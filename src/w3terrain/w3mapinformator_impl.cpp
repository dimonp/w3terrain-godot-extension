#include "w3mapassets.h"
#include "w3mapinformator_impl.h"

namespace w3terr {

static inline
uint32_t
pack_ground_key(uint8_t ts0, uint8_t ts1, uint8_t ts2, uint8_t ts3, uint8_t var)
{
    return (ts0 & 15U)
        | ((ts1 & 15U) << 4U)
        | ((ts2 & 15U) << 8U)
        | ((ts3 & 15U) << 12U)
        | ((var  & 15U) << 16U);
}

static inline
uint32_t
pack_cliff_key(uint8_t layer0, uint8_t layer1, uint8_t layer2, uint8_t layer3, uint8_t var)
{
    return (layer0 & 3U)
        | ((layer1 & 3U) << 2U)
        | ((layer2 & 3U) << 4U)
        | ((layer3 & 3U) << 6U)
        | ((var  & 3U) << 8U);
}

static inline
uint32_t
pack_ramp_key(uint8_t layer0, uint8_t layer1, uint8_t layer2, uint8_t layer3, uint8_t var, uint8_t part)
{
    return (layer0 & 15U)
        | ((layer1 & 15U) << 4U)
        | ((layer2 & 15U) << 8U)
        | ((layer3 & 15U) << 12U)
        | ((var    & 15U) << 16U)
        | ((part   & 15U) << 20U);
}

inline
const auto*
W3MapInformatorImpl::map() const
{
    return map_asset_->get_w3e();
}

uint32_t
W3MapInformatorImpl::get_cell_ground_key(const Coord2D& coords) const
{
    const uint8_t t10 = get_cellpoint_ground_tileset_id({ coords.x + 1, coords.y     });
    const uint8_t t00 = get_cellpoint_ground_tileset_id({ coords.x,     coords.y     });
    const uint8_t t11 = get_cellpoint_ground_tileset_id({ coords.x + 1, coords.y + 1 });
    const uint8_t t01 = get_cellpoint_ground_tileset_id({ coords.x,     coords.y + 1 });
    const uint8_t variation = map()->get_cellpoint(coords.x, coords.y).ground_variation;
    return pack_ground_key(t10, t00, t11, t01, variation); // NOLINT(readability-suspicious-call-argument)
}

uint32_t
W3MapInformatorImpl::get_cell_cliff_key(const Coord2D& coords) const
{
    const uint8_t l00 = map()->get_cellpoint(coords.x,     coords.y    ).height_layer;
    const uint8_t l10 = map()->get_cellpoint(coords.x + 1, coords.y    ).height_layer;
    const uint8_t l11 = map()->get_cellpoint(coords.x + 1, coords.y + 1).height_layer;
    const uint8_t l01 = map()->get_cellpoint(coords.x,     coords.y + 1).height_layer;
    const uint8_t min_height_layer = math::w3_min4(l00, l10, l11, l01);

#ifdef _DEBUG
    // validate layer
    uint32_t max_layer = math::w3_min4(l00, l10, l11, l01);
    w3_assert(max_layer - min_height_layer < 3);
#endif
    const uint8_t variation = map()->get_cellpoint(coords.x, coords.y).geo_variation;
    return pack_cliff_key(
        l00 - min_height_layer,
        l10 - min_height_layer,
        l11 - min_height_layer,
        l01 - min_height_layer,
        variation
    );
}

uint32_t
W3MapInformatorImpl::get_cell_ramp_key(const Coord2D& coords, uint8_t partition) const
{
    const RampLayout::Id ramp_layout = get_cellpoint_ramp_layout_id(coords);
    switch (ramp_layout) {
    case RampLayout::Id::HIGH_WEST_EAST: {
            const uint8_t hlayer00 = map()->get_cellpoint(coords.x,     coords.y    ).height_layer;
            const uint8_t hlayer20 = map()->get_cellpoint(coords.x + 2, coords.y    ).height_layer;
            const uint8_t hlayer21 = map()->get_cellpoint(coords.x + 2, coords.y + 1).height_layer;
            const uint8_t hlayer01 = map()->get_cellpoint(coords.x,     coords.y + 1).height_layer;
            const uint8_t base_layer = math::w3_min4(hlayer00, hlayer20, hlayer21, hlayer01);
            return pack_ramp_key(
                hlayer00 - base_layer,
                hlayer20 - base_layer,
                hlayer21 - base_layer + 4,
                hlayer01 - base_layer + 4,
                0,
                partition
            );
        }
    case RampLayout::Id::LOW_WEST_EAST: {
            const uint8_t hlayer00 = map()->get_cellpoint(coords.x,     coords.y    ).height_layer;
            const uint8_t hlayer20 = map()->get_cellpoint(coords.x + 2, coords.y    ).height_layer;
            const uint8_t hlayer21 = map()->get_cellpoint(coords.x + 2, coords.y + 1).height_layer;
            const uint8_t hlayer01 = map()->get_cellpoint(coords.x,     coords.y + 1).height_layer;
            const uint8_t base_layer = math::w3_min4(hlayer00, hlayer20, hlayer21, hlayer01);
            return pack_ramp_key(
                hlayer00 - base_layer + 4,
                hlayer20 - base_layer + 4,
                hlayer21 - base_layer,
                hlayer01 - base_layer,
                0,
                partition
            );
        }
    case RampLayout::Id::RIGHT_SOUTH_NORTH: {
            const uint8_t hlayer00 = map()->get_cellpoint(coords.x,     coords.y    ).height_layer;
            const uint8_t hlayer10 = map()->get_cellpoint(coords.x + 1, coords.y    ).height_layer;
            const uint8_t hlayer12 = map()->get_cellpoint(coords.x + 1, coords.y + 2).height_layer;
            const uint8_t hlayer02 = map()->get_cellpoint(coords.x,     coords.y + 2).height_layer;
            const uint8_t base_layer = math::w3_min4(hlayer00, hlayer10, hlayer12, hlayer02);
            return pack_ramp_key(
                hlayer00 - base_layer,
                hlayer10 - base_layer + 4,
                hlayer12 - base_layer + 4,
                hlayer02 - base_layer,
                0,
                partition
            );
        }
    case RampLayout::Id::LEFT_SOUTH_NORTH: {
            const uint8_t hlayer00 = map()->get_cellpoint(coords.x,     coords.y    ).height_layer;
            const uint8_t hlayer10 = map()->get_cellpoint(coords.x + 1, coords.y    ).height_layer;
            const uint8_t hlayer12 = map()->get_cellpoint(coords.x + 1, coords.y + 2).height_layer;
            const uint8_t hlayer02 = map()->get_cellpoint(coords.x,     coords.y + 2).height_layer;
            const uint8_t base_layer = math::w3_min4(hlayer00, hlayer10, hlayer12, hlayer02);
            return pack_ramp_key(
                hlayer00 - base_layer + 4,
                hlayer10 - base_layer,
                hlayer12 - base_layer,
                hlayer02 - base_layer + 4,
                0,
                partition
            );
        }
    default:
        break; //  do nothing
    }

    w3_log_error("Unknown ramp layout.");
    Ensures(false); // should not be here
    return -1;
}

uint32_t
W3MapInformatorImpl::get_cell_ramp_tileset_id(const Coord2D& coords, RampLayout::Id ramp_layout) const
{
    switch (ramp_layout) {
    case RampLayout::Id::HIGH_WEST_EAST:
        return get_cellpoint_ramp_tileset_id({ coords.x + 2, coords.y + 1 });
    case RampLayout::Id::LOW_WEST_EAST:
        return get_cellpoint_ramp_tileset_id({ coords.x, coords.y });
    case RampLayout::Id::RIGHT_SOUTH_NORTH:
        return get_cellpoint_ramp_tileset_id({ coords.x + 1, coords.y });
    case RampLayout::Id::LEFT_SOUTH_NORTH:
        return get_cellpoint_ramp_tileset_id({ coords.x, coords.y });
    default:
        break; //  do nothing
    }

    w3_log_error("Unknown ramp layout.");
    Ensures(false); // should not be here
    return -1;
}

uint8_t
W3MapInformatorImpl::get_cellpoint_ramp_tileset_id(const Coord2D& coords) const
{
    w3_assert(map()->get_cellpoint(coords.x, coords.y).check_flag(W3eCell::RAMP));

    static constexpr std::array<Delta2D, 8> kRampNearPattern = {{
        {-1, -1}, {0, -1}, { 1, -1}, { 1, 0}, { 1,  1}, {0,  1}, {-1,  1}, {-1, 0}
    }};
    static constexpr std::array<Delta2D, 8> kRampFarPattern =  {{
        {-2, -2}, {0, -2}, { 2, -2}, { 2, 0}, { 2,  2}, {0,  2}, {-2,  2}, {-2, 0}
    }};

    const uint32_t base_layer_height = map()->get_cellpoint(coords.x, coords.y).height_layer;

    for (const auto& pattern : kRampNearPattern) {
        const int32_t loc_idx_2d_x = coords.x + pattern.dx;
        const int32_t loc_idx_2d_y = coords.y + pattern.dy;

        if (!map()->is_valid_cellpoint(loc_idx_2d_x, loc_idx_2d_y)) {
            continue;
        }

        const W3eCell &cell_point = map()->get_cellpoint(loc_idx_2d_x, loc_idx_2d_y);
        if (cell_point.check_flag(W3eCell::RAMP) && (cell_point.height_layer != base_layer_height)) {
            return map()->get_cellpoint(loc_idx_2d_x, loc_idx_2d_y).geo_tileset;
        }
    }

    for (const auto& pattern : kRampFarPattern) {
        const int32_t loc_idx_2d_x = coords.x + pattern.dx;
        const int32_t loc_idx_2d_y = coords.y + pattern.dy;

        if (loc_idx_2d_x + 2 < 0 || loc_idx_2d_y + 2 < 0 ||
            loc_idx_2d_x >= map()->get_map_2d_size_x() - 2 || loc_idx_2d_y >= map()->get_map_2d_size_y() - 2) {
            continue;
        }

        const W3eCell &cell_point = map()->get_cellpoint(loc_idx_2d_x, loc_idx_2d_y);
        if (cell_point.check_flag(W3eCell::RAMP) && (cell_point.height_layer != base_layer_height)) {
            return map()->get_cellpoint(loc_idx_2d_x, loc_idx_2d_y).geo_tileset;
        }
    }

    w3_log_error("Unknown ramp layout.");
    Ensures(false); // should not be here
    return -1;
}

uint8_t
W3MapInformatorImpl::get_cellpoint_ground_tileset_id(const Coord2D& coords) const // NOLINT(readability-function-cognitive-complexity)
{
    static constexpr std::array<Delta2D, 4> kTmNearPattern2 = {{
        {0, 0},{-1, 0}, {0, -1}, {-1, -1}
    }};

    const auto& geo_tilesets = map_asset_->geo_assets_rt();

    // return ramp ground id, if we have neighbours ramps
    for (const auto& pattern : kTmNearPattern2) {
        const int32_t loc_idx_2d_x = coords.x + pattern.dx;
        const int32_t loc_idx_2d_y = coords.y + pattern.dy;

        if (!map()->is_valid_cellpoint(loc_idx_2d_x, loc_idx_2d_y)) {
            break; // Ramp should not be here
        }

        const RampLayout::Id rpl0 = get_cellpoint_ramp_layout_id({ loc_idx_2d_x, loc_idx_2d_y });
        if (rpl0 != RampLayout::Id::UNKNOWN) {
            const uint32_t tileset_id = get_cell_ramp_tileset_id({ loc_idx_2d_x, loc_idx_2d_y }, rpl0);
            w3_assert(tileset_id < geo_tilesets.size());

            const auto geo_resource = geo_tilesets[tileset_id];
            return geo_resource.ground_tileset_id;
        }

        const RampLayout::Id rpl1 = get_cellpoint_ramp_layout_id({ loc_idx_2d_x - 1, loc_idx_2d_y });
        if (rpl1 == RampLayout::Id::HIGH_WEST_EAST || rpl1 == RampLayout::Id::LOW_WEST_EAST) {
            const uint32_t tileset_id = get_cell_ramp_tileset_id({ loc_idx_2d_x - 1, loc_idx_2d_y }, rpl1);
            w3_assert(tileset_id < geo_tilesets.size());

            const auto geo_resource = geo_tilesets[tileset_id];
            return geo_resource.ground_tileset_id;
        }

        const RampLayout::Id rpl2 = get_cellpoint_ramp_layout_id({ loc_idx_2d_x, loc_idx_2d_y - 1 });
        if (rpl2 == RampLayout::Id::RIGHT_SOUTH_NORTH || rpl2 == RampLayout::Id::LEFT_SOUTH_NORTH) {
            const uint32_t tileset_id = get_cell_ramp_tileset_id({ loc_idx_2d_x, loc_idx_2d_y - 1 }, rpl2);
            w3_assert(tileset_id < geo_tilesets.size());

            const auto geo_resource = geo_tilesets[tileset_id];
            return geo_resource.ground_tileset_id;
        }
    }

    if (map()->get_cellpoint(coords.x, coords.y).check_flag(W3eCell::RAMP)) {
        return map()->get_cellpoint(coords.x, coords.y).ground_tileset;
    }

    // return cliff ground id, if we have neighbours cliffs
    for (const auto& pattern : kTmNearPattern2) {
        const int32_t loc_idx_2d_x = coords.x + pattern.dx;
        const int32_t loc_idx_2d_y = coords.y + pattern.dy;

        if (!check_cell_is_cliff({ loc_idx_2d_x, loc_idx_2d_y })) {
            continue;
        }

        uint32_t tileset_id = map()->get_cellpoint(loc_idx_2d_x, loc_idx_2d_y).geo_tileset;
        if (tileset_id == 15) { // ???
            tileset_id = geo_tilesets.size() - 1;
        }

        const auto& geo_resource = geo_tilesets[tileset_id];
        return geo_resource.ground_tileset_id;
    }
    return map()->get_cellpoint(coords.x, coords.y).ground_tileset;
}

bool
W3MapInformatorImpl::check_cell_ramp_for_layout(const Coord2D& coords, const RampLayout& ramp_layout) const
{
    const auto is_delta_valid = [map_ptr = map(), &coords] (const Delta2D& delta) -> bool {
        return  coords.x >= 0 && coords.y >= 0 &&
                coords.x + delta.dx < map_ptr->get_map_2d_size_x() &&
                coords.y + delta.dy < map_ptr->get_map_2d_size_y();
    };

    const auto is_ramp = [map_ptr = map(), &coords] (const Delta2D& delta) -> bool {
        return map_ptr->get_cellpoint(
            coords.x + delta.dx,
            coords.y + delta.dy).check_flag(W3eCell::RAMP);
    };

    const auto layer = [map_ptr = map(), &coords] (const Delta2D& delta) -> uint8_t {
        return map_ptr->get_cellpoint(
            coords.x + delta.dx,
            coords.y + delta.dy).height_layer;
    };

    // check if a pattern is inside the map
    for(const auto& delta: ramp_layout.ramp) {
        if (!is_delta_valid(delta)) { return false; }
    }
    for(const auto& delta: ramp_layout.not_ramp) {
        if (!is_delta_valid(delta)) { return false; }
    }

    // check if we have ramp where should not be
    for(const auto& delta: ramp_layout.not_ramp) {
        if (is_ramp(delta)) { return false; }
    }

    // check if we dont have ramp where should be
    for(const auto& delta: ramp_layout.ramp) {
        if (!is_ramp(delta)) { return false; }
    }

    // check the forward direction of elevation
    if (layer(ramp_layout.ramp[0]) == layer(ramp_layout.ramp[1]) &&
        layer(ramp_layout.ramp[1]) < layer(ramp_layout.ramp[2])) {
        return true;
    }

    // check the backward direction of elevation
    return layer(ramp_layout.ramp[2]) == layer(ramp_layout.ramp[1]) &&
        layer(ramp_layout.ramp[1]) < layer(ramp_layout.ramp[0]);
}

W3MapInformatorImpl::RampLayout::Id
W3MapInformatorImpl::get_cellpoint_ramp_layout_id(const Coord2D& coords) const
{
    // ramp layout pattern: high west-east
    //
    //  + + +
    //  o - -
    static constexpr RampLayout kPatternHwe = { RampLayout::Id::HIGH_WEST_EAST,
                                                {{ {0, 1}, {1, 1}, {2, 1} }},
                                                {{ {0, 0}, {1, 0}, {2, 0} }}
                                              };

    // ramp layout pattern: low west-east
    //
    //  - - -
    //  * + +
    static constexpr RampLayout kPatternLwe = { RampLayout::Id::LOW_WEST_EAST,
                                                {{ {0, 0}, {1, 0}, {2, 0} }},
                                                {{ {0, 1}, {1, 1}, {2, 1} }}
                                              };

    // ramp layout pattern: right south-north
    //  - +
    //  - +
    //  o +
    static constexpr RampLayout kPatternRsn = { RampLayout::Id::RIGHT_SOUTH_NORTH,
                                                {{ {1, 0}, {1, 1}, {1, 2} }},
                                                {{ {0, 0}, {0, 1}, {0, 2} }}
                                              };

    // ramp layout pattern: left south-north
    //  + -
    //  + -
    //  * -
    static constexpr RampLayout kPatternLsn = { RampLayout::Id::LEFT_SOUTH_NORTH,
                                                {{ {0, 0}, {0, 1}, {0, 2} }},
                                                {{ {1, 0}, {1, 1}, {1, 2} }}
                                              };

    static constexpr std::array<RampLayout, 4> kPatterns = { kPatternHwe, kPatternLwe, kPatternRsn, kPatternLsn };
    for (const auto& pattern : kPatterns ) {
        if (check_cell_ramp_for_layout(coords, pattern)) {
            return pattern.ramp_layout_id;
        }
    }
    return RampLayout::Id::UNKNOWN;
}

uint8_t
W3MapInformatorImpl::get_cellpoint_ramp_flag(const Coord2D& coords) const
{
    // near pattern:
    //   +
    //  +*+
    //   +
    static constexpr std::array<Delta2D, 4> kTmNearPattern0 = {{
        {0, -1}, {1, 0}, {0, 1}, {-1, 0}
    }};

    if (map()->get_cellpoint(coords.x, coords.y).check_flag(W3eCell::RAMP)) {
        const uint8_t base_layer_height = map()->get_cellpoint(coords.x, coords.y).height_layer;

        for (const auto& pattern : kTmNearPattern0) {
            const int32_t loc_idx_2d_x = coords.x + pattern.dx;
            const int32_t loc_idx_2d_y = coords.y + pattern.dy;
            if (!map()->is_valid_cellpoint(loc_idx_2d_x, loc_idx_2d_y)) {
                continue;
            }

            const W3eCell &cell_point = map()->get_cellpoint(loc_idx_2d_x, loc_idx_2d_y);
            if (cell_point.check_flag(W3eCell::RAMP) && (cell_point.height_layer > base_layer_height)) {
                return W3CPInfo::RAMP_MIDDLE;
            }
        }
        return W3CPInfo::RAMP;
    }
    return 0;
}

uint8_t
W3MapInformatorImpl::get_cellpoint_water_flag(const Coord2D& coords) const
{
    // near pattern:
    //   *+
    //   ++
    static constexpr std::array<Delta2D, 4> kTmNearPattern1 = {{
        {0, 0}, {1, 0}, {1, 1}, {0, 1}
    }};

    // check neighbour water tiles
    for (const auto& pattern : kTmNearPattern1) {
        const int32_t loc_idx_2d_x = coords.x + pattern.dx;
        const int32_t loc_idx_2d_y = coords.y + pattern.dy;
        if (!map()->is_valid_cellpoint(loc_idx_2d_x, loc_idx_2d_y)) {
            continue;
        }

        const W3eCell &cell_point = map()->get_cellpoint(loc_idx_2d_x, loc_idx_2d_y);
        if (cell_point.check_flag(W3eCell::WATER) && (cell_point.calc_total_height() < cell_point.get_water_height())) {
            return W3CPInfo::WATER;
        }
    }
    return 0;
}

bool
W3MapInformatorImpl::check_cell_is_cliff(const Coord2D& coords) const
{
    if (!map()->is_valid_cell(coords.x, coords.y)) {
        return false;
    }

    const W3eCell &cell_point0 = map()->get_cellpoint(coords.x,     coords.y);
    const W3eCell &cell_point1 = map()->get_cellpoint(coords.x + 1, coords.y);
    const W3eCell &cell_point2 = map()->get_cellpoint(coords.x + 1, coords.y + 1);
    const W3eCell &cell_point3 = map()->get_cellpoint(coords.x,     coords.y + 1);

    if (cell_point0.check_flag(W3eCell::RAMP) &&
        cell_point1.check_flag(W3eCell::RAMP) &&
        cell_point2.check_flag(W3eCell::RAMP) &&
        cell_point3.check_flag(W3eCell::RAMP)) {
        return false;
    }
    return  cell_point0.height_layer != cell_point1.height_layer ||
            cell_point0.height_layer != cell_point2.height_layer ||
            cell_point0.height_layer != cell_point3.height_layer;
}

}  // namespace w3terr