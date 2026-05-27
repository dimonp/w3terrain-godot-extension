#include "w3surfaceterrain.h"

#include "w3mapruntimemanager_impl.h"
#include "w3mapsectionmanager_impl.h"
#include "w3mapsectionrenderedcache.h"
#include "w3mapsection.h"
#include "w3mapnode.h"

namespace w3terr {

void
W3SurfaceTerrain::render_section_ground(uint32_t section_id)
{
    auto* rendered_sections_cache = get_rendered_sections_cache();
    auto* rendered_section = rendered_sections_cache->get_rendered(section_id);

    if (rendered_section != nullptr && rendered_section->surface_idx_ground >= 0) {
        return;
    }

    const auto* section_manager = get_section_manager();
    const W3MapSection& section = section_manager->get_section_by_id(section_id);

    auto ground_tileset_usage = section.get_ground_tileset_usage();

    bool has_any_active = std::ranges::any_of(
        ground_tileset_usage,
        [](const auto& bits) {
            return bits.any();
        });
    if (!has_any_active) {
        return;
    }

    int8_t surface_idx = begin_render(section_id);

    // for each ground tileset layer
    for(int64_t tileset_id = 0; tileset_id < ground_tileset_usage.size(); ++tileset_id ) {
        if (!ground_tileset_usage[tileset_id].any()) {
            continue;
        }

        float texture_index = static_cast<float>(tileset_id);
        surface_tool_->set_custom(0, godot::Color(texture_index, 0.0F, 0.0F));

        render_ground_cells(section_id, tileset_id);
    }

    rendered_section = end_render(section_id);
    rendered_section->surface_idx_ground = surface_idx;

    if (ground_material_asset_.is_valid()) {
        RS->mesh_surface_set_material(rendered_section->get_mesh_rid(), surface_idx, ground_material_asset_->get_rid());
    }
}

void
W3SurfaceTerrain::render_ground_cells(uint32_t section_id, size_t tileset_id)
{
    const auto* assets = get_assets();
    const auto* section_manager = get_section_manager();
    const auto* runtime_manager = get_runtime_manager();

    const W3MapSection& section = section_manager->get_section_by_id(section_id);
    const W3MapAssets::GroundAsset &ground_tileset_rt = assets->ground_asset_rt(tileset_id);

    uint32_t layer = section.get_ground_tileset_to_layer_map()[tileset_id];
    auto ground_tileset_usage = section.get_ground_tileset_usage();

    // for each cell in this section
    for(size_t cell_idx = 0; cell_idx < W3MapSection::kNumberOfCells; ++cell_idx) {
        if (!ground_tileset_usage[tileset_id][cell_idx]) { // Is there water in the cell?
            layer >>= 2U;
            continue;
        }

        const auto section_origin = section_manager->calc_section_origin(section_id);
        const auto cell_coord = W3MapSection::calc_cell_coord_from_idx(section_origin, cell_idx);

        const W3MapRuntimeManager::CellPointRT& cell_rt00 = runtime_manager->get_cellpoint_rt(cell_coord);
        const W3MapRuntimeManager::CellPointRT& cell_rt10 = runtime_manager->get_cellpoint_rt({ cell_coord.x + 1, cell_coord.y });
        const W3MapRuntimeManager::CellPointRT& cell_rt01 = runtime_manager->get_cellpoint_rt({ cell_coord.x,     cell_coord.y + 1 });
        const W3MapRuntimeManager::CellPointRT& cell_rt11 = runtime_manager->get_cellpoint_rt({ cell_coord.x + 1, cell_coord.y + 1 });

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
        surface_tool_->set_normal(math::unpack_vector3_from_32bit(cell_rt00.packed_normal));
        surface_tool_->set_uv({ uv0.x, uv1.y });
        surface_tool_->add_vertex(runtime_manager->get_cellpoint_position(cell_coord));

        // vertex 10
        surface_tool_->set_normal(math::unpack_vector3_from_32bit(cell_rt10.packed_normal));
        surface_tool_->set_uv(uv1);
        surface_tool_->add_vertex(runtime_manager->get_cellpoint_position({ cell_coord.x + 1, cell_coord.y }));

        // vertex 11
        surface_tool_->set_normal(math::unpack_vector3_from_32bit(cell_rt11.packed_normal));
        surface_tool_->set_uv({ uv1.x, uv0.y });
        surface_tool_->add_vertex(runtime_manager->get_cellpoint_position({ cell_coord.x + 1, cell_coord.y + 1 }));

        // vertex 01
        surface_tool_->set_normal(math::unpack_vector3_from_32bit(cell_rt01.packed_normal));
        surface_tool_->set_uv(uv0);
        surface_tool_->add_vertex(runtime_manager->get_cellpoint_position({ cell_coord.x, cell_coord.y + 1 }));

        // cell indices counterclockwise order
        surface_tool_->add_index(static_cast<int32_t>(vertices_counter_));
        surface_tool_->add_index(static_cast<int32_t>(vertices_counter_ + 2));
        surface_tool_->add_index(static_cast<int32_t>(vertices_counter_ + 1));
        surface_tool_->add_index(static_cast<int32_t>(vertices_counter_));
        surface_tool_->add_index(static_cast<int32_t>(vertices_counter_ + 3));
        surface_tool_->add_index(static_cast<int32_t>(vertices_counter_ + 2));

        vertices_counter_ += 4;

#ifdef W3MAP_STATS_ENABLE
        stat_ground_tiles_rendered_++;
#endif
        layer >>= 2U;
    }
}

}  // namespace w3terr