#include "w3mapsection.h"
#include "w3mapruntimemanager.h"

namespace w3terr {

W3MapSection::W3MapSection(uint8_t ground_tilesets_size, uint8_t geo_tilesets_size)
    : ground_tilesets_size_(ground_tilesets_size)
    , geo_tilesets_size_(geo_tilesets_size)
    , ground_tileset_to_layer_map_(new uint32_t[ground_tilesets_size])
    , tilesets_usage_(new TilesetUsage[ground_tilesets_size + geo_tilesets_size + 1])
{
}

W3MapSection::~W3MapSection() noexcept
{
    delete [] tilesets_usage_;
    delete [] ground_tileset_to_layer_map_;
}

W3MapSection::W3MapSection(W3MapSection&& rhs) noexcept
    : ground_tilesets_size_(std::exchange(rhs.ground_tilesets_size_, 0))
    , geo_tilesets_size_(std::exchange(rhs.geo_tilesets_size_, 0))
    , ground_tileset_to_layer_map_(std::exchange(rhs.ground_tileset_to_layer_map_, nullptr))
    , tilesets_usage_(std::exchange(rhs.tilesets_usage_, nullptr))
{
}

W3MapSection&
W3MapSection::operator=(W3MapSection&& rhs) noexcept
{
    ground_tilesets_size_ = std::exchange(rhs.ground_tilesets_size_, 0);
    geo_tilesets_size_  = std::exchange(rhs.geo_tilesets_size_, 0);
    ground_tileset_to_layer_map_ = std::exchange(rhs.ground_tileset_to_layer_map_, nullptr);
    tilesets_usage_ = std::exchange(rhs.tilesets_usage_, nullptr);
    return *this;
}

void
W3MapSection::update_cell(const Coord2D& cell_coords, const size_t cell_idx, W3MapRuntimeManager* runtime)
{
    const W3MapRuntimeManager::CellPointRT& cell_rt = runtime->get_cellpoint_rt(cell_coords);

    // mark cell as unusable for all geoset ground meshes
    for(auto & usage : std::span(ground_usage_ptr(), ground_tilesets_size_)) {
         usage.set(cell_idx, false);
    }

    // mark cell as unusable for all geo meshes
    for(auto & usage : std::span(geo_usage_ptr(), geo_tilesets_size_)) {
         usage.set(cell_idx, false);
    }

    // NOLINTBEGIN(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    if (cell_rt.check_flag(W3MapRuntimeManager::CellPointRT::GROUND)) {  // ground cellpoint
        TilesetUsage* ground_usage = ground_usage_ptr();
        for(size_t layer_idx = 0; layer_idx < W3MapRuntimeManager::CellPointRT::kMaxGroundLayers; ++layer_idx) {

            size_t tileset_id = cell_rt.get_ground_tileset_id(layer_idx);
            if (tileset_id == W3MapRuntimeManager::CellPointRT::kEmptyTilesetId) {
                continue;
            }

            // init cells
            auto& usage = ground_usage[tileset_id];
            usage.set(cell_idx, true);
            ground_tileset_to_layer_map_[tileset_id] |= layer_idx << (cell_idx << 1U);
        }
    } else if (cell_rt.check_flag(W3MapRuntimeManager::CellPointRT::GEO_CLIFF)) { // geo cellpoint (cliffs)
        TilesetUsage* geo_usage = geo_usage_ptr();
        const size_t tileset_id = cell_rt.tileset_id;
        auto& usage = geo_usage[tileset_id];
        usage |= (1U << cell_idx);
    } else if (cell_rt.check_flag(W3MapRuntimeManager::CellPointRT::GEO_RAMP)) { // geo cellpoint (ramps)
        TilesetUsage* geo_usage = geo_usage_ptr();
        const size_t tileset_id = cell_rt.tileset_id;
        auto& usage = geo_usage[tileset_id];
        usage.set(cell_idx, true);
    }
    // NOLINTEND(cppcoreguidelines-pro-bounds-pointer-arithmetic)

    // update water info
    TilesetUsage* water_usage = water_usage_ptr();
    water_usage->set(cell_idx, cell_rt.check_flag(W3MapRuntimeManager::CellPointRT::WATER));
}

void
W3MapSection::update_all_cells(const Coord2D& section_origin, W3MapRuntimeManager* runtime)
{
    std::ranges::fill(std::span(ground_tileset_to_layer_map_, ground_tilesets_size_), 0);
    std::ranges::fill(std::span(ground_usage_ptr(), ground_tilesets_size_), 0);
    std::ranges::fill(std::span(geo_usage_ptr(), geo_tilesets_size_), 0);
    *water_usage_ptr() = 0;

    for(uint32_t cell_idx = 0; cell_idx < kNumberOfCells; ++cell_idx) {
        const auto cell_coords = calc_cell_coord_from_idx(section_origin, cell_idx);
        update_cell(cell_coords, cell_idx, runtime);
    }
}

bool
W3MapSection::refresh(const Coord2D& section_origin, W3MapRuntimeManager* runtime)
{
    if (is_dirty()) {
        update_all_cells(section_origin, runtime);
        set_dirty(false);
        return true;
    }
    return false;
}

}  // namespace w3terr