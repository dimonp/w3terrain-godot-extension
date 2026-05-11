#include "w3mapsection.h"
#include "w3mapruntimemanager.h"

namespace w3terr {

void
W3MapSection::initialize(size_t ground_tilesets_size, size_t geo_tilesets_size)
{
    ground_tileset_to_layer_map.assign(ground_tilesets_size, {});
    ground_tileset_usage.assign(ground_tilesets_size, {});
    geo_tileset_usage.assign(geo_tilesets_size, {});
    water_usage = {};
}

void
W3MapSection::free_cached_data()
{
    rendered_mesh.free();
}

void
W3MapSection::update_cell(const Coord2D& cell_coords, const size_t cell_idx)
{
    const W3MapRuntimeManager::CellPointRT& cell_rt = runtime_manager_->get_cellpoint_rt(cell_coords);

    // mark cell as unusable for all geoset ground meshes
    for(auto & usage : ground_tileset_usage) {
         usage.set(cell_idx, false);
    }

    // // mark cell as unusable for all geo meshes
    for(auto & usage : geo_tileset_usage) {
         usage.set(cell_idx, false);
    }

    water_usage.set(cell_idx, false);

    if (cell_rt.check_flag(W3MapRuntimeManager::CellPointRT::GROUND)) {  // ground cellpoint
        for(size_t layer_idx = 0; layer_idx < W3MapRuntimeManager::CellPointRT::kMaxGroundLayers; ++layer_idx) {

            size_t tileset_id = cell_rt.get_ground_tileset_id(layer_idx);
            if (tileset_id == W3MapRuntimeManager::CellPointRT::kEmptyTilesetId) {
                continue;
            }

            // init cells
            auto& usage = ground_tileset_usage[tileset_id];
            usage.set(cell_idx, true);
            ground_tileset_to_layer_map[tileset_id] |= layer_idx << (cell_idx << 1U);
        }
    } else if (cell_rt.check_flag(W3MapRuntimeManager::CellPointRT::GEO_CLIFF)) { // geo cellpoint (cliffs)
        const size_t tileset_id = cell_rt.tileset_id;
        auto& usage = geo_tileset_usage[tileset_id];
        usage |= (1U << cell_idx);
    } else if (cell_rt.check_flag(W3MapRuntimeManager::CellPointRT::GEO_RAMP)) { // geo cellpoint (ramps)
        const size_t tileset_id = cell_rt.tileset_id;
        auto& usage = geo_tileset_usage[tileset_id];
        usage.set(cell_idx, true);
    }

    // update water info
    if (cell_rt.check_flag(W3MapRuntimeManager::CellPointRT::WATER)) {
        water_usage.set(cell_idx, true);
    }
}

void
W3MapSection::update_all_cells(const Coord2D& section_origin)
{
    std::ranges::fill(ground_tileset_to_layer_map, 0);
    std::ranges::fill(ground_tileset_usage, 0);
    std::ranges::fill(geo_tileset_usage, 0);
    water_usage = 0;

    for(uint32_t cell_idx = 0; cell_idx < kNumberOfCells; ++cell_idx) {
        const auto cell_coords = calc_cell_coord_from_idx(section_origin, cell_idx);
        update_cell(cell_coords, cell_idx);
    }
}

bool
W3MapSection::refresh(const Coord2D& section_origin)
{
    if (is_dirty()) {
        free_cached_data();
        update_all_cells(section_origin);
        set_dirty(false);
        return true;
    }
    return false;
}

}  // namespace w3terr