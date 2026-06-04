#include "w3surfaceterrain.h"

#include "w3mapruntimemanager_impl.h"
#include "w3mapsectionmanager_impl.h"
#include "w3mapsectionrenderedcache.h"
#include "w3mapsection.h"
#include "w3mapnode.h"

namespace w3terr {

void
W3SurfaceTerrain::render_section_geo(uint32_t section_id)
{
    auto* rendered_sections_cache = get_rendered_sections_cache();
    auto* rendered_section = rendered_sections_cache->get_rendered(section_id);

    // if not rendered yet
    if (rendered_section != nullptr && rendered_section->surface_idx_geo >= 0) {
        return;
    }

    const auto* section_manager = get_section_manager();
    const W3MapSection& section = section_manager->get_section_by_id(section_id);

    auto geo_tileset_usage = section.get_geo_tileset_usage();

    bool has_any_active = std::ranges::any_of(
        geo_tileset_usage,
        [](const auto& bits) {
            return bits.any();
        });
    if (!has_any_active) {
        return;
    }

    int8_t surface_idx = begin_render(section_id);

    // for each geo tileset layer
    for(int64_t tileset_id = 0; tileset_id < geo_tileset_usage.size(); ++tileset_id ) {
        if (!geo_tileset_usage[tileset_id].any()) {
            continue;
        }

        float texture_index = static_cast<float>(tileset_id);
        surface_tool_->set_custom(0, godot::Color(texture_index, 0.0F, 0.0F));

        render_geo_cells(section_id, tileset_id);
    }

    rendered_section = end_render(section_id);
    rendered_section->surface_idx_geo = surface_idx;

    if (geo_material_asset_.is_valid()) {
        RS->mesh_surface_set_material(rendered_section->get_mesh_rid(), surface_idx, geo_material_asset_->get_rid());
    }
}

void
W3SurfaceTerrain::render_geo_cells(uint32_t section_id, size_t tileset_id) // NOLINT(readability-function-cognitive-complexity)
{
    const auto* assets = get_assets();
    const auto* section_manager = get_section_manager();
    const auto* runtime_manager = get_runtime_manager();

    const W3MapSection& section = section_manager->get_section_by_id(section_id);
    auto geo_tileset_usage = section.get_geo_tileset_usage();

    // for each cell in this section
    for(size_t cell_idx = 0; cell_idx < W3MapSection::kNumberOfCells; ++cell_idx) {
        if (!geo_tileset_usage[tileset_id][cell_idx]) { // Is there water in the cell?
            continue;
        }

        const auto section_origin = section_manager->calc_section_origin(section_id);
        const auto cell_coord = W3MapSection::calc_cell_coord_from_idx(section_origin, cell_idx);

        const W3MapRuntimeManager::CellPointRT& cell_rt00 = runtime_manager->get_cellpoint_rt(cell_coord);
        const W3MapRuntimeManager::CellPointRT& cell_rt10 = runtime_manager->get_cellpoint_rt({cell_coord.x + 1, cell_coord.y});
        const W3MapRuntimeManager::CellPointRT& cell_rt01 = runtime_manager->get_cellpoint_rt({cell_coord.x,     cell_coord.y + 1});
        const W3MapRuntimeManager::CellPointRT& cell_rt11 = runtime_manager->get_cellpoint_rt({cell_coord.x + 1, cell_coord.y + 1});

        const W3Mesh *geo_mesh_ptr = nullptr;
        if (cell_rt00.check_flag(W3MapRuntimeManagerImpl::CellPointRT::GEO_CLIFF)) {
            geo_mesh_ptr = assets->geo_asset_rt(cell_rt00.tileset_id).cliff_geoset_mesh.ptr();
        } else if (cell_rt00.check_flag(W3MapRuntimeManagerImpl::CellPointRT::GEO_RAMP)) {
            geo_mesh_ptr = assets->geo_asset_rt(cell_rt00.tileset_id).ramp_geoset_mesh.ptr();
        } else {
            w3_log_error("W3SurfaceTerrain::render_geo_cells: Unknown geo tileset: %d !", cell_rt00.tileset_id);
            continue;
        }

        if (geo_mesh_ptr == nullptr) {
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

        const auto& mesh_arrays = geo_mesh_ptr->surface_get_arrays(cell_rt00.geo_id);
        const godot::PackedVector3Array& src_vertices = mesh_arrays[W3Mesh::ARRAY_VERTEX];
        const godot::PackedVector3Array& src_normales = mesh_arrays[W3Mesh::ARRAY_NORMAL];
        const godot::PackedVector2Array& src_uvs = mesh_arrays[W3Mesh::ARRAY_TEX_UV];
        const godot::PackedInt32Array& src_indices = mesh_arrays[W3Mesh::ARRAY_INDEX];

        const int64_t src_vertices_size = src_vertices.size();
        const int64_t src_indices_size = src_indices.size();

        // put vertices to cache buffer
        for (int64_t src_vertex_idx = 0 ; src_vertex_idx < src_vertices_size; ++src_vertex_idx) {
            const float t_lerp = math::w3_clamp01(-src_vertices[src_vertex_idx].z * kW3MapTile2DInvSize);
            const float s_lerp = math::w3_clamp01(src_vertices[src_vertex_idx].x * kW3MapTile2DInvSize);

            // adjust inner vertices height
            const float delta_h = math::w3_lerp_bi(dh00, dh01, dh10, dh11, t_lerp, s_lerp);

            // cell edge stat
            const bool edge10 = std::abs(t_lerp) < 0.01F;
            const bool edge23 = std::abs(t_lerp - 1.0F) < 0.01F;
            const bool edge03 = std::abs(s_lerp) < 0.01F;
            const bool edge12 = std::abs(s_lerp - 1.0F) < 0.01F;

            // correct corner normals
            if (edge10 && edge03) {
                surface_tool_->set_normal(math::unpack_vector3_from_32bit(cell_rt00.packed_normal));
            } else if (edge10 && edge12) {
                surface_tool_->set_normal(math::unpack_vector3_from_32bit(cell_rt10.packed_normal));
            } else if (edge23 && edge12) {
                surface_tool_->set_normal(math::unpack_vector3_from_32bit(cell_rt11.packed_normal));
            } else if (edge23 && edge03) {
                surface_tool_->set_normal(math::unpack_vector3_from_32bit(cell_rt01.packed_normal));
            } else {
                math::vector3 normal = src_normales[src_vertex_idx];
                // correct edge normals
                if (edge10 || edge23) {
                    normal.z = 0;
                } else if (edge03 || edge12) {
                    normal.x = 0;
                }
                normal.normalize();

                surface_tool_->set_normal(normal);
            }

            surface_tool_->set_uv(src_uvs[src_vertex_idx]);
            surface_tool_->add_vertex({
                pos.x + src_vertices[src_vertex_idx].x,
                pos.y + src_vertices[src_vertex_idx].y + delta_h,
                pos.z + src_vertices[src_vertex_idx].z
            });
        }

        // put indices to cache buffer
        for (int64_t src_index_idx = 0 ; src_index_idx < src_indices_size; ++src_index_idx) {
            surface_tool_->add_index(static_cast<int32_t>(vertices_counter_ + src_indices[src_index_idx]));
        }

        vertices_counter_ += src_vertices_size;

#ifdef W3MAP_STATS_ENABLE
        stat_geo_tiles_rendered_++;
#endif
    }
}

void
W3SurfaceTerrain::render_geo_cells_normals(uint32_t section_id) // NOLINT(readability-function-cognitive-complexity)
{
    constexpr float kNormalScaleFactor = 10.0;

    const auto* assets = get_assets();
    const auto* section_manager = get_section_manager();
    const auto* runtime_manager = get_runtime_manager();

    const W3MapSection& section = section_manager->get_section_by_id(section_id);

    // for each cell in this section
    for(size_t cell_idx = 0; cell_idx < W3MapSection::kNumberOfCells; ++cell_idx) {
        const auto section_origin = section_manager->calc_section_origin(section_id);
        const auto cell_coord = W3MapSection::calc_cell_coord_from_idx(section_origin, cell_idx);

        const W3MapRuntimeManager::CellPointRT& cell_rt00 = runtime_manager->get_cellpoint_rt(cell_coord);
        const W3MapRuntimeManager::CellPointRT& cell_rt10 = runtime_manager->get_cellpoint_rt({cell_coord.x + 1, cell_coord.y});
        const W3MapRuntimeManager::CellPointRT& cell_rt01 = runtime_manager->get_cellpoint_rt({cell_coord.x,     cell_coord.y + 1});
        const W3MapRuntimeManager::CellPointRT& cell_rt11 = runtime_manager->get_cellpoint_rt({cell_coord.x + 1, cell_coord.y + 1});

        const W3Mesh *geo_mesh_ptr = nullptr;
        if (cell_rt00.check_flag(W3MapRuntimeManagerImpl::CellPointRT::GEO_CLIFF)) {
            geo_mesh_ptr = assets->geo_asset_rt(cell_rt00.tileset_id).cliff_geoset_mesh.ptr();
        } else if (cell_rt00.check_flag(W3MapRuntimeManagerImpl::CellPointRT::GEO_RAMP)) {
            geo_mesh_ptr = assets->geo_asset_rt(cell_rt00.tileset_id).ramp_geoset_mesh.ptr();
        } else {
            w3_log_error("W3SurfaceTerrain::render_geo_cells_normals: Unknown geo tileset: %d !", cell_rt00.tileset_id);
            continue;
        }

        if (geo_mesh_ptr == nullptr) {
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

        const auto& mesh_arrays = geo_mesh_ptr->surface_get_arrays(cell_rt00.geo_id);
        const godot::PackedVector3Array& src_vertices = mesh_arrays[W3Mesh::ARRAY_VERTEX];
        const godot::PackedVector3Array& src_normales = mesh_arrays[W3Mesh::ARRAY_NORMAL];

        const int64_t src_vertices_size = src_vertices.size();

        // put vertices to cache buffer
        for (int64_t src_vertex_idx = 0 ; src_vertex_idx < src_vertices_size; ++src_vertex_idx) {
            const float t_lerp = math::w3_clamp01(-src_vertices[src_vertex_idx].z * kW3MapTile2DInvSize);
            const float s_lerp = math::w3_clamp01(src_vertices[src_vertex_idx].x * kW3MapTile2DInvSize);

            // adjust inner vertices height
            const float delta_h = math::w3_lerp_bi(dh00, dh01, dh10, dh11, t_lerp, s_lerp);

            // cell edge stat
            const bool edge10 = std::abs(t_lerp) < 0.01F;
            const bool edge23 = std::abs(t_lerp - 1.0F) < 0.01F;
            const bool edge03 = std::abs(s_lerp) < 0.01F;
            const bool edge12 = std::abs(s_lerp - 1.0F) < 0.01F;

            // correct corner normals
            math::vector3 normal;
            if (edge10 && edge03) {
                normal = math::unpack_vector3_from_32bit(cell_rt00.packed_normal);
            } else if (edge10 && edge12) {
                normal = math::unpack_vector3_from_32bit(cell_rt10.packed_normal);
            } else if (edge23 && edge12) {
                normal = math::unpack_vector3_from_32bit(cell_rt11.packed_normal);
            } else if (edge23 && edge03) {
                normal = math::unpack_vector3_from_32bit(cell_rt01.packed_normal);
            } else {
                normal = src_normales[src_vertex_idx];
                // correct edge normals
                if (edge10 || edge23) {
                    normal.z = 0;
                } else if (edge03 || edge12) {
                    normal.x = 0;
                }
                normal.normalize();
            }

            math::vector3 vertex = {
                pos.x + src_vertices[src_vertex_idx].x,
                pos.y + src_vertices[src_vertex_idx].y + delta_h,
                pos.z + src_vertices[src_vertex_idx].z
            };

            surface_tool_->add_vertex(vertex);
            surface_tool_->add_vertex(vertex + normal * kNormalScaleFactor);
        }

        vertices_counter_ += src_vertices_size;
    }
}

}  // namespace w3terr