#include "w3surfaceterrain.h"

#include <godot_cpp/classes/performance.hpp>

#include "w3mapruntimemanager_impl.h"
#include "w3mapsectionmanager_impl.h"
#include "w3mapsection.h"
#include "w3mapnode.h"

namespace w3terr {

void
W3SurfaceTerrain::render_grounds(const W3Array<uint32_t>& sections, bool render_as_normals)
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
W3SurfaceTerrain::render_ground_cells(uint32_t section_id, size_t tileset_id, bool render_as_normals)
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

/*
    Generate and precache a mesh for sections with given ground tileset
*/
W3Pair<W3SurfaceTerrain::VertexSpan, W3SurfaceTerrain::IndexSpan>
W3SurfaceTerrain::precache_ground_cells(uint32_t section_id, size_t tileset_id) const
{
    const auto* assets = get_assets();
    const auto* section_manager = get_section_manager();

    const W3MapSection& section = section_manager->get_section_by_id(section_id);
    const W3MapSection::CachedMesh &cached_mesh = section.get_cached_ground_mesh(tileset_id);
    const W3MapAssets::GroundAsset &ground_tileset_rt = assets->ground_asset_rt(tileset_id);

    // alloc lru cache memory
    auto [vertices, indices] = cached_mesh.allocate_mesh_data<W3SurfaceTerrain::CachedVertex, uint16_t>();
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

        const auto section_origin = section_manager->calc_section_origin(section_id);
        const auto cell_coord = W3MapSection::calc_cell_coord_from_idx(section_origin, cell_idx);

        const W3MapRuntimeManager::CellPointRT& cell_rt00 = runtime_manager->get_cellpoint_rt(cell_coord);
        const W3MapRuntimeManager::CellPointRT& cell_rt10 = runtime_manager->get_cellpoint_rt({ cell_coord.x + 1, cell_coord.y });
        const W3MapRuntimeManager::CellPointRT& cell_rt11 = runtime_manager->get_cellpoint_rt({ cell_coord.x + 1, cell_coord.y + 1 });
        const W3MapRuntimeManager::CellPointRT& cell_rt01 = runtime_manager->get_cellpoint_rt({ cell_coord.x,     cell_coord.y + 1 });

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

}  // namespace w3terr