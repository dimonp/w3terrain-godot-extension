#include <godot_cpp/classes/performance.hpp>

#include "w3mapcollector_impl.h"
#include "w3mapruntimemanager_impl.h"
#include "w3mapsectionmanager_impl.h"
#include "w3mapsection.h"
#include "w3mapnode.h"
#include "w3surfaceground.h"

namespace w3terr {

void
W3SurfaceGround::_bind_methods()
{
    godot::ClassDB::bind_method(godot::D_METHOD("get_ground_material"), &W3SurfaceGround::get_ground_material);
    godot::ClassDB::bind_method(godot::D_METHOD("set_ground_material", "ground_material"), &W3SurfaceGround::set_ground_material);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::OBJECT, "ground_material",
            godot::PROPERTY_HINT_RESOURCE_TYPE,
            "Material"),
        "set_ground_material", "get_ground_material");

    godot::ClassDB::bind_method(godot::D_METHOD("get_geo_material"), &W3SurfaceGround::get_geo_materials);
    godot::ClassDB::bind_method(godot::D_METHOD("set_geo_material", "geo_material"), &W3SurfaceGround::set_geo_materials);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::OBJECT, "geo_material",
            godot::PROPERTY_HINT_RESOURCE_TYPE,
            "ShaderMaterial"),
        "set_geo_material", "get_geo_material");

    godot::ClassDB::bind_method(godot::D_METHOD("get_debug_material"), &W3SurfaceGround::get_debug_material);
    godot::ClassDB::bind_method(godot::D_METHOD("set_debug_material", "debug_material"), &W3SurfaceGround::set_debug_material);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::OBJECT, "debug_material",
            godot::PROPERTY_HINT_RESOURCE_TYPE,
            "Material"),
        "set_debug_material", "get_debug_material");

    godot::ClassDB::bind_method(godot::D_METHOD("get_render_normals"), &W3SurfaceGround::get_render_normals);
    godot::ClassDB::bind_method(godot::D_METHOD("set_render_normals", "p_camera"), &W3SurfaceGround::set_render_normals);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "render_normals"),
        "set_render_normals", "get_render_normals"
    );
}

void
W3SurfaceGround::_notification(int p_what)
{
    // avoid to save dinamicaly created resources to scene file
    switch (p_what) {
    case NOTIFICATION_EDITOR_PRE_SAVE: {
        ground_material_asset_->set_shader_parameter("texture_array", nullptr);
        geo_material_asset_->set_shader_parameter("texture_array", nullptr);
        break;
    }
    case NOTIFICATION_EDITOR_POST_SAVE: {
        ground_material_asset_->set_shader_parameter("texture_array", ground_textures_array_);
        geo_material_asset_->set_shader_parameter("texture_array", geo_textures_array_);
        break;
    }
    default:
        break;
    }
}

void
W3SurfaceGround::_enter_tree() {
    const auto callable_ground_assets_changed = callable_mp(this, &W3SurfaceGround::on_ground_assets_changed);
    if (!get_map_node()->is_connected(W3MapNode::kSignalGroundAssetsChanged, callable_ground_assets_changed)) {
        get_map_node()->connect(W3MapNode::kSignalGroundAssetsChanged, callable_ground_assets_changed);
    }

    const auto callable_geo_assets_changed = callable_mp(this, &W3SurfaceGround::on_geo_assets_changed);
    if (!get_map_node()->is_connected(W3MapNode::kSignalGeoAssetsChanged, callable_geo_assets_changed)) {
        get_map_node()->connect(W3MapNode::kSignalGeoAssetsChanged, callable_geo_assets_changed);
    }

#ifdef W3MAP_STATS_ENABLE
    godot::Performance *perf = godot::Performance::get_singleton();
    const auto stat_ground_tiles_precached_callable = callable_mp(this, &W3SurfaceGround::get_stat_ground_tiles_precached);
    perf->add_custom_monitor(kStatGroundTilesPrecachedId, stat_ground_tiles_precached_callable);
    const auto stat_geo_tiles_precached_callable = callable_mp(this, &W3SurfaceGround::get_stat_geo_tiles_precached);
    perf->add_custom_monitor(kStatGeoTilesPrecachedId, stat_geo_tiles_precached_callable);
#endif
}

void
W3SurfaceGround::_exit_tree()
{
#ifdef W3MAP_STATS_ENABLE
    godot::Performance *perf = godot::Performance::get_singleton();
    if (perf->has_custom_monitor(kStatGroundTilesPrecachedId)) {
        perf->remove_custom_monitor(kStatGroundTilesPrecachedId);
    }
    if (perf->has_custom_monitor(kStatGeoTilesPrecachedId)) {
        perf->remove_custom_monitor(kStatGeoTilesPrecachedId);
    }
#endif
}

void
W3SurfaceGround::_process(double  /*delta*/)
{
    if (get_map_node() == nullptr) {
        return;
    }

    if (ground_assets_dirty_flag_) {
        load_ground_materials();
        reset_rendered();
        ground_assets_dirty_flag_ = false;
    }

    if (geo_assets_dirty_flag_) {
        load_geo_materials();
        reset_rendered();
        geo_assets_dirty_flag_ = false;
    }

    const auto* assets = get_assets();
    // release all early rendered meshes in the GPU only if the maximum count is reached
    // or runtime data is outdated
    const size_t total_tilesets = static_cast<size_t>(assets->ground_assets_size_rt()) + assets->geo_assets_size_rt();
    if (is_mesh_dirty() || mesh_->get_surface_count() + total_tilesets > kMaxGPUMeshes) {
        reset_rendered();
    }

    const auto* collector = get_collector();
    if (collector == nullptr) {
        return;
    }

    const W3Array<uint32_t>& visible_sections = collector->get_visible_sections();
    if (visible_sections.empty()) {
        return;
    }

    render(visible_sections);
}

void
W3SurfaceGround::reset_rendered()
{
    if (mesh_.is_valid()) {
        mesh_->clear_surfaces();
    }
    section_rendered_flags_.clear();
}

void
W3SurfaceGround::render(const W3Array<uint32_t>& sections)
{
    not_rendered_sections_.clear();
    // filter already rendered sections
    std::ranges::copy_if (sections,
        std::back_inserter(not_rendered_sections_),
        [&rendered_flags = section_rendered_flags_](uint32_t section_id) {
            return !rendered_flags[section_id];
        });

    if (not_rendered_sections_.empty()) {
        return;
    }

    // mark all sections as rendered
    for(const uint32_t section_id : not_rendered_sections_) {
        section_rendered_flags_[section_id] = true;
    }

    render_geos(not_rendered_sections_, false);
    render_grounds(not_rendered_sections_, false);

    if (render_normals_) {
        render_geos(not_rendered_sections_, true);
        render_grounds(not_rendered_sections_, true);
    }
}

void
W3SurfaceGround::render_grounds(const W3Array<uint32_t>& sections, bool render_as_normals)
{
    const auto* assets = get_assets();
    // for each ground tilset layer
    for(int64_t tileset_id = 0; tileset_id < assets->ground_assets_size_rt(); ++tileset_id ) {
        const W3MapAssets::GroundAsset& ground_type = assets->ground_asset_rt(tileset_id);

        begin_render(render_as_normals);
        if (!render_as_normals) {
            surface_tool_->set_material(ground_material_asset_);
        } else {
            surface_tool_->set_material(debug_material_);
        }

        float texture_index = static_cast<float>(tileset_id);
        surface_tool_->set_custom(0, godot::Color(texture_index, 0.0F, 0.0F));

        for(const uint32_t section_id : sections) {
            render_ground_cells(section_id, tileset_id, render_as_normals);
        }
        end_render();
    }
}

void
W3SurfaceGround::render_geos(const W3Array<uint32_t>& sections, bool render_as_normals)
{
    const auto* assets = get_assets();
    // for each geo tilset layer
    for(int64_t tileset_id = 0; tileset_id < assets->geo_assets_size_rt(); ++tileset_id ) {
        const W3MapAssets::GeoAsset& geo_type = assets->geo_asset_rt(tileset_id);

        begin_render(render_as_normals);
        if (!render_as_normals) {
            surface_tool_->set_material(geo_material_asset_);
        } else {
            surface_tool_->set_material(debug_material_);
        }

        float texture_index = static_cast<float>(tileset_id);
        surface_tool_->set_custom(0, godot::Color(texture_index, 0.0F, 0.0F));

        for(const uint32_t section_id : sections) {
            render_geo_cells(section_id, tileset_id, render_as_normals);
        }
        end_render();
    }
}

void
W3SurfaceGround::render_ground_cells(uint32_t section_id, size_t tileset_id, bool render_as_normals)
{
    const auto* section_manager = get_section_manager();
    const W3MapSection& section = section_manager->get_section_by_id(section_id);

    if (tileset_id >= section.get_ground_tilesets_size() || section.get_ground_vertices_count(tileset_id) == 0) {
        return;
    }

    const W3MapSection::CachedMesh &cached_mesh = section.get_cached_ground_mesh(tileset_id);
    if (!cached_mesh.is_used()) { // this tileset is not used in this section
        return;
    }

    // already cached ?
    auto [vertices, indices] = cached_mesh.get_cached_mesh_data<CachedVertex, uint16_t>();
    if (vertices.empty()) {

#ifdef W3MAP_STATS_ENABLE
        stat_ground_tiles_precached_ = 0;
#endif

        std::tie(vertices, indices) = precache_ground_cells(section_id, tileset_id);
    }

    if (!render_as_normals) {
        render_cached_mesh(vertices, indices);
    } else {
        render_cached_mesh_normals(vertices);
    }
}

void
W3SurfaceGround::render_geo_cells(uint32_t section_id, size_t tileset_id, bool render_as_normals)
{
    const auto* section_manager = get_section_manager();
    const W3MapSection& section = section_manager->get_section_by_id(section_id);

    if (tileset_id >= section.get_geo_tilesets_size() || section.get_geo_vertices_count(tileset_id) == 0) {
        return;
    }

    const W3MapSection::CachedMesh &cached_mesh = section.get_cached_geo_mesh(tileset_id);
    if (!cached_mesh.is_used()) { // this tileset is not used in this section
        return;
    }

    auto [vertices, indices] = cached_mesh.get_cached_mesh_data<CachedVertex, uint16_t>();
    if (vertices.empty()) {

#ifdef W3MAP_STATS_ENABLE
        stat_geo_tiles_precached_ = 0;
#endif

        std::tie(vertices, indices) = precache_geo_cells(section_id, tileset_id);
    }

    if (!render_as_normals) {
        render_cached_mesh(vertices, indices);
    } else {
        render_cached_mesh_normals(vertices);
    }
}

/*
    Generate and precache a mesh for sections with given ground tileset
*/
W3Pair<W3SurfaceGround::VertexSpan, W3SurfaceGround::IndexSpan>
W3SurfaceGround::precache_ground_cells(uint32_t section_id, size_t tileset_id) const
{
    const auto* assets = get_assets();
    const auto* section_manager = get_section_manager();

    const W3MapSection& section = section_manager->get_section_by_id(section_id);
    const W3MapSection::CachedMesh &cached_mesh = section.get_cached_ground_mesh(tileset_id);
    const W3MapAssets::GroundAsset &ground_tileset_rt = assets->ground_asset_rt(tileset_id);

    // alloc lru cache memory
    auto [vertices, indices] = cached_mesh.allocate_mesh_data<W3SurfaceGround::CachedVertex, uint16_t>();
    if (vertices.empty()) {
        w3_log_error("Can't allocate ground vertices cache.");
        return {};
    }

    uint16_t dest_vertex_idx = 0;
    uint16_t dest_index_idx = 0;
    uint32_t layer = section.map_ground_tileset_to_layer(tileset_id);

    const auto* runtime_manager = get_runtime_manager();
    // for each cell in this section
    for(size_t cell_idx = 0; cell_idx < W3MapSection::kNumberOfCells; ++cell_idx) {
        if (!cached_mesh.is_used_by(cell_idx)) { // Does the cell use this tile set?
            layer >>= 2U;
            continue;
        }

        w3_assert(dest_vertex_idx < cached_mesh.vertices_count());
        w3_assert(dest_index_idx < cached_mesh.indices_count());

        const Coord2D cell_coord = section.calc_cell_coord_from_idx(cell_idx);
        const W3MapRuntimeManagerImpl::CellPointRT& cell_rt00 = runtime_manager->get_cellpoint_rt(cell_coord);
        const W3MapRuntimeManagerImpl::CellPointRT& cell_rt10 = runtime_manager->get_cellpoint_rt({ cell_coord.x + 1, cell_coord.y });
        const W3MapRuntimeManagerImpl::CellPointRT& cell_rt11 = runtime_manager->get_cellpoint_rt({ cell_coord.x + 1, cell_coord.y + 1 });
        const W3MapRuntimeManagerImpl::CellPointRT& cell_rt01 = runtime_manager->get_cellpoint_rt({ cell_coord.x,     cell_coord.y + 1 });

        // calc cell uv
        const auto [tile_idx_u, tile_idx_v] = cell_rt00.get_ground_tile_uv_indices(layer);
        const math::vector2 uv0(
            ground_tileset_rt.tile_tu_size * static_cast<float>(tile_idx_u),
            ground_tileset_rt.tile_tv_size * static_cast<float>(tile_idx_v)
        );
        const math::vector2 uv1 = {
            uv0.x + ground_tileset_rt.tile_tu_size,
            uv0.y + ground_tileset_rt.tile_tv_size
        };

        // vertex 00
        vertices[dest_vertex_idx].pos = runtime_manager->get_cellpoint_position(cell_coord);
        vertices[dest_vertex_idx].norm = math::unpack_vector3_from_32bit(cell_rt00.packed_normal);
        vertices[dest_vertex_idx].uv.x = uv0.x;
        vertices[dest_vertex_idx].uv.y = uv1.y;
        ++dest_vertex_idx;

        // vertex 10
        vertices[dest_vertex_idx].pos = runtime_manager->get_cellpoint_position({ cell_coord.x + 1, cell_coord.y });
        vertices[dest_vertex_idx].norm = math::unpack_vector3_from_32bit(cell_rt10.packed_normal);
        vertices[dest_vertex_idx].uv = uv1;
        ++dest_vertex_idx;

        // vertex 11
        vertices[dest_vertex_idx].pos = runtime_manager->get_cellpoint_position({ cell_coord.x + 1, cell_coord.y + 1 });
        vertices[dest_vertex_idx].norm = math::unpack_vector3_from_32bit(cell_rt11.packed_normal);
        vertices[dest_vertex_idx].uv.x = uv1.x;
        vertices[dest_vertex_idx].uv.y = uv0.y;
        ++dest_vertex_idx;

        // vertex 01
        vertices[dest_vertex_idx].pos = runtime_manager->get_cellpoint_position({ cell_coord.x, cell_coord.y + 1 });
        vertices[dest_vertex_idx].norm = math::unpack_vector3_from_32bit(cell_rt01.packed_normal);
        vertices[dest_vertex_idx].uv = uv0;
        ++dest_vertex_idx;

        // cell indices counterclockwise order
        indices[dest_index_idx++] = dest_vertex_idx - 4; // 0
        indices[dest_index_idx++] = dest_vertex_idx - 2; // 2
        indices[dest_index_idx++] = dest_vertex_idx - 3; // 1
        indices[dest_index_idx++] = dest_vertex_idx - 4; // 0
        indices[dest_index_idx++] = dest_vertex_idx - 1; // 3
        indices[dest_index_idx++] = dest_vertex_idx - 2; // 2

#ifdef W3MAP_STATS_ENABLE
        stat_ground_tiles_precached_++;
#endif
        layer >>= 2U;
    }
    return { vertices, indices };
}

/*
    Generate and precache a mesh for sections with given geo tileset
*/
W3Pair<W3SurfaceGround::VertexSpan, W3SurfaceGround::IndexSpan>
W3SurfaceGround::precache_geo_cells(uint32_t section_id, size_t tileset_id) const // NOLINT(readability-function-cognitive-complexity)
{
    const auto* section_manager = get_section_manager();
    const auto* runtime_manager = get_runtime_manager();

    const W3MapSection& section = section_manager->get_section_by_id(section_id);
    const W3MapSection::CachedMesh &cached_mesh = section.get_cached_geo_mesh(tileset_id);

    // alloc lru cache memory
    auto [vertices, indices] = cached_mesh.allocate_mesh_data<CachedVertex, uint16_t>();
    if (vertices.empty()) {
        w3_log_error("Can't allocate geo vertices cache.");
        return {};
    }

    size_t dest_vertex_idx = 0;
    size_t dest_index_idx = 0;
    size_t dest_index_offeset = 0;

    const auto* assets = get_assets();
    // for each cell in this section
    for(size_t cell_idx = 0; cell_idx < W3MapSection::kNumberOfCells; ++cell_idx) {
        if (!cached_mesh.is_used_by(cell_idx)) { // // Does the cell use this tile set?
            continue;
        }

        w3_assert(dest_vertex_idx < cached_mesh.vertices_count());
        w3_assert(dest_index_idx < cached_mesh.indices_count());

        const Coord2D cell_coord = section.calc_cell_coord_from_idx(cell_idx);
        const W3MapRuntimeManagerImpl::CellPointRT& cell_rt00 = runtime_manager->get_cellpoint_rt(cell_coord);
        const W3MapRuntimeManagerImpl::CellPointRT& cell_rt10 = runtime_manager->get_cellpoint_rt({cell_coord.x + 1, cell_coord.y});
        const W3MapRuntimeManagerImpl::CellPointRT& cell_rt11 = runtime_manager->get_cellpoint_rt({cell_coord.x + 1, cell_coord.y + 1});
        const W3MapRuntimeManagerImpl::CellPointRT& cell_rt01 = runtime_manager->get_cellpoint_rt({cell_coord.x,     cell_coord.y + 1});

        const W3Mesh *gro_mesh_ptr = nullptr;
        if (cell_rt00.check_flag(W3MapRuntimeManagerImpl::CellPointRT::GEO_CLIFF)) {
            gro_mesh_ptr = assets->geo_asset_rt(cell_rt00.tileset_id).cliff_geoset_mesh.ptr();
        } else if (cell_rt00.check_flag(W3MapRuntimeManagerImpl::CellPointRT::GEO_RAMP)) {
            gro_mesh_ptr = assets->geo_asset_rt(cell_rt00.tileset_id).ramp_geoset_mesh.ptr();
        } else {
            w3_log_error("nTileMapSection::PrecacheGeoCells: Unknown geo tileset: %d !", cell_rt00.tileset_id);
            continue;
        }

        if (gro_mesh_ptr == nullptr) {
            continue;
        }

        math::vector3 pos = runtime_manager->get_cellpoint_position(cell_coord);
        const float lh00 = runtime_manager->get_cellpoint_layer_height(cell_coord);
        const float lh10 = runtime_manager->get_cellpoint_layer_height({ cell_coord.x + 1, cell_coord.y     });
        const float lh11 = runtime_manager->get_cellpoint_layer_height({ cell_coord.x + 1, cell_coord.y + 1 });
        const float lh01 = runtime_manager->get_cellpoint_layer_height({ cell_coord.x,     cell_coord.y + 1 });
        pos.y = math::w3_min4(lh00, lh10, lh11, lh01);

        const float dh00 = runtime_manager->get_cellpoint_ground_height(cell_coord);
        const float dh10 = runtime_manager->get_cellpoint_ground_height({ cell_coord.x + 1, cell_coord.y     });
        const float dh11 = runtime_manager->get_cellpoint_ground_height({ cell_coord.x + 1, cell_coord.y + 1 });
        const float dh01 = runtime_manager->get_cellpoint_ground_height({ cell_coord.x,     cell_coord.y + 1 });

        const auto& mesh_arrays = gro_mesh_ptr->surface_get_arrays(cell_rt00.geoset_id);
        const godot::PackedVector3Array& src_vertices = mesh_arrays[W3Mesh::ARRAY_VERTEX];
        const godot::PackedVector3Array& src_normales = mesh_arrays[W3Mesh::ARRAY_NORMAL];
        const godot::PackedVector2Array& src_uvs = mesh_arrays[W3Mesh::ARRAY_TEX_UV];
        const godot::PackedInt32Array& src_indices = mesh_arrays[W3Mesh::ARRAY_INDEX];

        const size_t src_vertices_size = src_vertices.size();
        const size_t src_indices_size = src_indices.size();

        // put vertices to cache buffer
        for (int64_t src_vertex_idx = 0 ; src_vertex_idx < src_vertices_size; ++src_vertex_idx) {
            const float t_lerp = math::w3_clamp01(-src_vertices[src_vertex_idx].z * kW3MapTile2DInvSize);
            const float s_lerp = math::w3_clamp01(src_vertices[src_vertex_idx].x * kW3MapTile2DInvSize);

            // adjust inner vertices height
            const float delta_h = math::w3_lerp_bi(dh00, dh01, dh10, dh11, t_lerp, s_lerp);

            vertices[dest_vertex_idx].pos = {
                pos.x + src_vertices[src_vertex_idx].x,
                pos.y + src_vertices[src_vertex_idx].y + delta_h,
                pos.z + src_vertices[src_vertex_idx].z
            };

            // cell edge stat
            const bool edge10 = std::abs(t_lerp) < 0.01F;
            const bool edge23 = std::abs(t_lerp - 1.0F) < 0.01F;
            const bool edge03 = std::abs(s_lerp) < 0.01F;
            const bool edge12 = std::abs(s_lerp - 1.0F) < 0.01F;

            // correct corner normals
            if (edge10 && edge03) {
                vertices[dest_vertex_idx].norm = math::unpack_vector3_from_32bit(cell_rt00.packed_normal);
            } else if (edge10 && edge12) {
                vertices[dest_vertex_idx].norm = math::unpack_vector3_from_32bit(cell_rt10.packed_normal);
            } else if (edge23 && edge12) {
                vertices[dest_vertex_idx].norm = math::unpack_vector3_from_32bit(cell_rt11.packed_normal);
            } else if (edge23 && edge03) {
                vertices[dest_vertex_idx].norm = math::unpack_vector3_from_32bit(cell_rt01.packed_normal);
            } else {
                vertices[dest_vertex_idx].norm = src_normales[src_vertex_idx];

                // correct edge normals
                if (edge10 || edge23) {
                    vertices[dest_vertex_idx].norm.z = 0;
                } else if (edge03 || edge12) {
                    vertices[dest_vertex_idx].norm.x = 0;
                }

                vertices[dest_vertex_idx].norm.normalize();
            }

            vertices[dest_vertex_idx].uv = src_uvs[src_vertex_idx];
            dest_vertex_idx++;
        }

        // put indices to cache buffer
        for (int64_t src_index_idx = 0 ; src_index_idx < src_indices_size; ++src_index_idx) {
            indices[dest_index_idx++] = dest_index_offeset + src_indices[src_index_idx];
        }

        dest_index_offeset += src_vertices_size;

#ifdef W3MAP_STATS_ENABLE
        stat_geo_tiles_precached_++;
#endif
    }
    return { vertices, indices };
}

/*
    Generate and precache a mesh for sections with given geo tileset
*/
W3Pair<W3SurfaceGround::VertexSpan, W3SurfaceGround::IndexSpan>
W3SurfaceGround::precache_geo_cells_fake(uint32_t section_id, size_t tileset_id) const // NOLINT(readability-function-cognitive-complexity)
{
    const auto* section_manager = get_section_manager();
    const auto* runtime_manager = get_runtime_manager();

    const W3MapSection& section = section_manager->get_section_by_id(section_id);
    const W3MapSection::CachedMesh &cached_mesh = section.get_cached_geo_mesh(tileset_id);

    // alloc lru cache memory
    auto [vertices, indices] = cached_mesh.allocate_mesh_data<CachedVertex, uint16_t>();
    if (vertices.empty()) {
        w3_log_error("Can't allocate geo vertices cache.");
        return {};
    }

    size_t dest_vertex_idx = 0;
    size_t dest_index_idx = 0;
    size_t dest_index_offeset = 0;

    const auto* assets = get_assets();
    // for each cell in this section
    for(size_t cell_idx = 0; cell_idx < W3MapSection::kNumberOfCells; ++cell_idx) {
        if (!cached_mesh.is_used_by(cell_idx)) { // // Does the cell use this tile set?
            continue;
        }

        w3_assert(dest_vertex_idx < cached_mesh.vertices_count());
        w3_assert(dest_index_idx < cached_mesh.indices_count());

        const Coord2D cell_coord = section.calc_cell_coord_from_idx(cell_idx);
        const W3MapRuntimeManagerImpl::CellPointRT& cell_rt00 = runtime_manager->get_cellpoint_rt(cell_coord);
        const W3MapRuntimeManagerImpl::CellPointRT& cell_rt10 = runtime_manager->get_cellpoint_rt({cell_coord.x + 1, cell_coord.y});
        const W3MapRuntimeManagerImpl::CellPointRT& cell_rt11 = runtime_manager->get_cellpoint_rt({cell_coord.x + 1, cell_coord.y + 1});
        const W3MapRuntimeManagerImpl::CellPointRT& cell_rt01 = runtime_manager->get_cellpoint_rt({cell_coord.x,     cell_coord.y + 1});

        const W3Mesh *gro_mesh_ptr = nullptr;
        if (cell_rt00.check_flag(W3MapRuntimeManagerImpl::CellPointRT::GEO_CLIFF)) {
            gro_mesh_ptr = assets->geo_asset_rt(cell_rt00.tileset_id).cliff_geoset_mesh.ptr();

        } else if (cell_rt00.check_flag(W3MapRuntimeManagerImpl::CellPointRT::GEO_RAMP)) {

            gro_mesh_ptr = assets->geo_asset_rt(cell_rt00.tileset_id).ramp_geoset_mesh.ptr();
        } else {
            w3_log_error("nTileMapSection::PrecacheGeoCells: Unknown geo tileset: %d !", cell_rt00.tileset_id);
            continue;
        }

        if (gro_mesh_ptr == nullptr) {
            continue;
        }

        math::vector3 pos = runtime_manager->get_cellpoint_position(cell_coord);
        const float lh00 = runtime_manager->get_cellpoint_layer_height(cell_coord);
        const float lh10 = runtime_manager->get_cellpoint_layer_height({ cell_coord.x + 1, cell_coord.y     });
        const float lh11 = runtime_manager->get_cellpoint_layer_height({ cell_coord.x + 1, cell_coord.y + 1 });
        const float lh01 = runtime_manager->get_cellpoint_layer_height({ cell_coord.x,     cell_coord.y + 1 });
        pos.y = math::w3_min4(lh00, lh10, lh11, lh01);

        const float dh00 = runtime_manager->get_cellpoint_ground_height(cell_coord);
        const float dh10 = runtime_manager->get_cellpoint_ground_height({ cell_coord.x + 1, cell_coord.y     });
        const float dh11 = runtime_manager->get_cellpoint_ground_height({ cell_coord.x + 1, cell_coord.y + 1 });
        const float dh01 = runtime_manager->get_cellpoint_ground_height({ cell_coord.x,     cell_coord.y + 1 });

        const auto& mesh_arrays = gro_mesh_ptr->surface_get_arrays(cell_rt00.geoset_id);
        const godot::PackedVector3Array& src_vertices = mesh_arrays[W3Mesh::ARRAY_VERTEX];
        const godot::PackedVector3Array& src_normales = mesh_arrays[W3Mesh::ARRAY_NORMAL];
        const godot::PackedVector2Array& src_uvs = mesh_arrays[W3Mesh::ARRAY_TEX_UV];
        const godot::PackedInt32Array& src_indices = mesh_arrays[W3Mesh::ARRAY_INDEX];

        const size_t src_vertices_size = src_vertices.size();
        const size_t src_indices_size = src_indices.size();

        // put vertices to cache buffer
        for (int64_t src_vertex_idx = 0 ; src_vertex_idx < src_vertices_size; ++src_vertex_idx) {
            const float t_lerp = math::w3_clamp01(-src_vertices[src_vertex_idx].z * kW3MapTile2DInvSize);
            const float s_lerp = math::w3_clamp01(src_vertices[src_vertex_idx].x * kW3MapTile2DInvSize);

            // adjust inner vertices height
            const float delta_h = math::w3_lerp_bi(dh00, dh01, dh10, dh11, t_lerp, s_lerp);

            vertices[dest_vertex_idx].pos = {
                pos.x + src_vertices[src_vertex_idx].x,
                pos.y + src_vertices[src_vertex_idx].y + delta_h,
                pos.z + src_vertices[src_vertex_idx].z
            };

            // cell edge stat
            const bool edge10 = std::abs(t_lerp) < 0.01F;
            const bool edge23 = std::abs(t_lerp - 1.0F) < 0.01F;
            const bool edge03 = std::abs(s_lerp) < 0.01F;
            const bool edge12 = std::abs(s_lerp - 1.0F) < 0.01F;

            // correct corner normals
            if (edge10 && edge03) {
                vertices[dest_vertex_idx].norm = math::unpack_vector3_from_32bit(cell_rt00.packed_normal);
            } else if (edge10 && edge12) {
                vertices[dest_vertex_idx].norm = math::unpack_vector3_from_32bit(cell_rt10.packed_normal);
            } else if (edge23 && edge12) {
                vertices[dest_vertex_idx].norm = math::unpack_vector3_from_32bit(cell_rt11.packed_normal);
            } else if (edge23 && edge03) {
                vertices[dest_vertex_idx].norm = math::unpack_vector3_from_32bit(cell_rt01.packed_normal);
            } else {
                vertices[dest_vertex_idx].norm = src_normales[src_vertex_idx];

                // correct edge normals
                if (edge10 || edge23) {
                    vertices[dest_vertex_idx].norm.z = 0;
                } else if (edge03 || edge12) {
                    vertices[dest_vertex_idx].norm.x = 0;
                }

                vertices[dest_vertex_idx].norm.normalize();
            }

            vertices[dest_vertex_idx].uv = src_uvs[src_vertex_idx];
            dest_vertex_idx++;
        }

        // put indices to cache buffer
        for (int64_t src_index_idx = 0 ; src_index_idx < src_indices_size; ++src_index_idx) {
            indices[dest_index_idx++] = dest_index_offeset + src_indices[src_index_idx];
        }

        dest_index_offeset += src_vertices_size;

#ifdef W3MAP_STATS_ENABLE
            stat_geo_tiles_precached_++;
#endif
    }
    return { vertices, indices };
}

void
W3SurfaceGround::render_cached_mesh(VertexSpan vertices, IndexSpan indices)
{
    for(const auto& vertex : vertices) {
        surface_tool_->set_uv(vertex.uv);
        surface_tool_->set_normal(vertex.norm);
        surface_tool_->add_vertex(vertex.pos);
    }

    for(const auto index : indices) {
        surface_tool_->add_index(static_cast<int32_t>(index + vertices_counter_));
    }
    vertices_counter_ += static_cast<int64_t>(vertices.size());
}

void
W3SurfaceGround::render_cached_mesh_normals(VertexSpan vertices)
{
    constexpr float kNormalScaleFactor = 10.0;
    for(const auto& vertex : vertices) {
        surface_tool_->add_vertex(vertex.pos);
        surface_tool_->add_vertex(vertex.pos + vertex.norm * kNormalScaleFactor);
    }
    vertices_counter_ += static_cast<int64_t>(vertices.size());
}

}  // namespace w3terr