#include <godot_cpp/classes/performance.hpp>

#include "w3mapruntimemanager_impl.h"
#include "w3mapsectionmanager_impl.h"
#include "w3mapsection.h"
#include "w3mapnode.h"
#include "w3surfaceterrain.h"

namespace w3terr {


void
W3SurfaceTerrain::render_geos(const W3Array<uint32_t>& sections, bool render_as_normals)
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
W3SurfaceTerrain::render_geo_cells(uint32_t section_id, size_t tileset_id, bool render_as_normals)
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
    Generate and precache a mesh for sections with given geo tileset
*/
W3Pair<W3SurfaceTerrain::VertexSpan, W3SurfaceTerrain::IndexSpan>
W3SurfaceTerrain::precache_geo_cells(uint32_t section_id, size_t tileset_id) const // NOLINT(readability-function-cognitive-complexity)
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

        const auto section_origin = section_manager->calc_section_origin(section_id);
        const auto cell_coord = W3MapSection::calc_cell_coord_from_idx(section_origin, cell_idx);

        const W3MapRuntimeManager::CellPointRT& cell_rt00 = runtime_manager->get_cellpoint_rt(cell_coord);
        const W3MapRuntimeManager::CellPointRT& cell_rt10 = runtime_manager->get_cellpoint_rt({cell_coord.x + 1, cell_coord.y});
        const W3MapRuntimeManager::CellPointRT& cell_rt11 = runtime_manager->get_cellpoint_rt({cell_coord.x + 1, cell_coord.y + 1});
        const W3MapRuntimeManager::CellPointRT& cell_rt01 = runtime_manager->get_cellpoint_rt({cell_coord.x,     cell_coord.y + 1});

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

}  // namespace w3terr