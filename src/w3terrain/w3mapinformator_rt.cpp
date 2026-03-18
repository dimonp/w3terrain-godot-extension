#include <ranges>

#include "w3mapassets.h"
#include "w3mapinformator_impl.h"

namespace w3terr {

/**
 * Extracts a 4-bit ground tileset id from a packed 32-bit key.
 * Each 4-bit nibble represents a layer (up to 8 layers supported by the shift,
 * but masked to 4 at now).
 */
[[nodiscard]]
static inline
uint8_t
extract_ground_tileset_id(const uint32_t packed_ids, const uint8_t layer_idx)
{
    return (packed_ids >> ((layer_idx & 3U) << 2U)) & 15U;
}

/**
 * Calculates a 4-bit mask where each bit 'i' is set if the ground tileset
 * is present at layer 'i'.
 */
 [[nodiscard]]
 static inline
 uint8_t
 calc_ground_layer_tilesets_mask(const uint32_t ground_key, const uint8_t layer_idx)
 {
    // Get the target ground tileset we are looking for
    const uint32_t target_tileset_id = extract_ground_tileset_id(ground_key, layer_idx);

    uint32_t mask = 0;
    // Check layer 0 (bits 0-3)
    mask |= static_cast<uint32_t>(target_tileset_id == (ground_key & 15U));
    // Check layer 1 (bits 4-7)
    mask |= static_cast<uint32_t>(target_tileset_id == ((ground_key >> 4U) & 15U)) << 1U;
    // Check layer 2 (bits 8-11)
    mask |= static_cast<uint32_t>(target_tileset_id == ((ground_key >> 8U) & 15U)) << 2U;
    // Check layer 3 (bits 12-15)
    mask |= static_cast<uint32_t>(target_tileset_id == ((ground_key >> 12U) & 15U)) << 3U;
    return mask;
}

inline
const auto*
W3MapInformatorImpl::map() const
{
    return map_asset_->get_w3e();
}

bool
W3MapInformatorImpl::collect_cellpoint_ramp_info(const Coord2D& coords, W3CPInfo& info) const
{
    if (const RampLayout::Id rl0 = get_cellpoint_ramp_layout_id(coords);
                rl0 != RampLayout::Id::UNKNOWN) {
        info.flags |= W3CPInfo::GEORAMP;
        info.key = get_cell_ramp_key(coords, 0);
        info.geo_tileset = get_cell_ramp_tileset_id(coords, rl0);

    } else if (const RampLayout::Id rl1 = get_cellpoint_ramp_layout_id({ coords.x - 1, coords.y});
                rl1 == RampLayout::Id::HIGH_WEST_EAST || rl1 == RampLayout::Id::LOW_WEST_EAST) {
        info.flags |= W3CPInfo::GEORAMP;
        info.key = get_cell_ramp_key({ coords.x - 1, coords.y }, 1);
        info.geo_tileset = get_cell_ramp_tileset_id({ coords.x -1, coords.y }, rl1);

    } else if (const RampLayout::Id rl2 = get_cellpoint_ramp_layout_id({ coords.x, coords.y - 1 });
                rl2 == RampLayout::Id::RIGHT_SOUTH_NORTH || rl2 == RampLayout::Id::LEFT_SOUTH_NORTH) {
        info.flags |= W3CPInfo::GEORAMP;
        info.key = get_cell_ramp_key({ coords.x, coords.y - 1 }, 1);
        info.geo_tileset = get_cell_ramp_tileset_id({ coords.x, coords.y - 1 }, rl2);
    } else {
        return false;
    }
    return true;
}

bool
W3MapInformatorImpl::collect_cellpoint_ground_info(const Coord2D& coords, W3CPInfo& info) const
{
    bool is_ground = !check_cell_is_cliff(coords)
        || (info.flags & W3CPInfo::RAMP_MIDDLE) != 0
        || (info.flags & W3CPInfo::RAMP) != 0 &&
            (get_cellpoint_ramp_flag({ coords.x + 1, coords.y     }) == W3CPInfo::RAMP_MIDDLE ||
             get_cellpoint_ramp_flag({ coords.x,     coords.y + 1 }) == W3CPInfo::RAMP_MIDDLE);

    if (is_ground) {
        info.flags |= W3CPInfo::GROUND;
        info.key = get_cell_ground_key(coords);
    } else {
        return false;
    }

    for (auto [layer_idx, layer] : std::views::enumerate(info.ground_layers)) {
        layer = {
            .mask = calc_ground_layer_tilesets_mask(info.key, layer_idx),
            .tileset_id = extract_ground_tileset_id(info.key, layer_idx)
        };
    }

    // sort ground layers by tileset id
    W3LayerInfo::sort_four_layers_by_tileset_id(
        info.ground_layers[0],
        info.ground_layers[1],
        info.ground_layers[2],
        info.ground_layers[3]);

    return true;
}

bool
W3MapInformatorImpl::collect_cellpoint_cliff_info(const Coord2D& coords, W3CPInfo& info) const
{
    info.geo_tileset = map()->get_cellpoint(coords.x, coords.y).geo_tileset;
    info.flags |= W3CPInfo::GEOCLIFF;
    info.key = get_cell_cliff_key(coords);
    return true;
}

W3MapInformatorImpl::W3CPInfo
W3MapInformatorImpl::collect_cellpoint_info(const Coord2D& coords) const
{
    W3CPInfo cellpoint_info = { 0 };
    cellpoint_info.normal = map()->calc_cellpoint_normal(coords.x, coords.y);
    cellpoint_info.flags = get_cellpoint_ramp_flag(coords);
    cellpoint_info.flags |= get_cellpoint_water_flag(coords);

    const W3eCell& map_cell_point = map()->get_cellpoint(coords.x, coords.y);
    cellpoint_info.height_layer = map_cell_point.height_layer;
    cellpoint_info.ground_height = map_cell_point.ground_height;
    cellpoint_info.water_height = map_cell_point.water_height;

    if (coords.y == map()->get_map_2d_size_y() - 1 || coords.x == map()->get_map_2d_size_x() - 1) {
        return cellpoint_info;
    }

    collect_cellpoint_ramp_info(coords, cellpoint_info) ||
    collect_cellpoint_ground_info(coords, cellpoint_info) ||
    collect_cellpoint_cliff_info(coords, cellpoint_info);

    if (cellpoint_info.geo_tileset == 15) {
        cellpoint_info.geo_tileset = map()->get_geo_tilesets_count() - 1;
    }
    return cellpoint_info;
}

}  // namespace w3terr