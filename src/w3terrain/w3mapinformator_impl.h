#ifndef _W3MAPINFORMATOR_IMPL__H
#define _W3MAPINFORMATOR_IMPL__H

#include <gsl/gsl>

#include "w3mapinformator.h"
#include "w3defs.h"

namespace w3terr {

class W3MapAssets;

class W3_API W3MapInformatorImpl final : public W3MapInformator {
public:
    explicit W3MapInformatorImpl(const W3MapAssets* assets): map_asset_(assets) {}

    W3CPInfo collect_cellpoint_info(const Coord2D& coords) const override;

private:
    struct RampLayout {
        enum Id: uint8_t {
            UNKNOWN             = 0,
            HIGH_WEST_EAST      = 1,
            LOW_WEST_EAST       = 2,
            RIGHT_SOUTH_NORTH   = 3,
            LEFT_SOUTH_NORTH    = 4
        };

        Id ramp_layout_id;
        std::array<Delta2D, 3> ramp;
        std::array<Delta2D, 3> not_ramp;
    };

    uint32_t get_cell_ground_key(const Coord2D& coords) const;
    uint32_t get_cell_cliff_key(const Coord2D& coords) const;
    uint32_t get_cell_ramp_key(const Coord2D& coords, uint8_t partition) const;
    uint32_t get_cell_ramp_tileset_id(const Coord2D& coords, RampLayout::Id ramp_layout) const;
    bool check_cell_is_cliff(const Coord2D& coords) const;

    bool check_cell_ramp_for_layout(const Coord2D& coords, const RampLayout& ramp_layout) const;
    RampLayout::Id get_cellpoint_ramp_layout_id(const Coord2D& coords) const;

    uint8_t get_cellpoint_ground_tileset_id(const Coord2D& coords) const;
    uint8_t get_cellpoint_ramp_tileset_id(const Coord2D& coords) const;
    uint8_t get_cellpoint_ramp_flag(const Coord2D& coords) const;
    uint8_t get_cellpoint_water_flag(const Coord2D& coords) const;

    bool collect_cellpoint_ramp_info(const Coord2D& coords, W3CPInfo& info) const;
    bool collect_cellpoint_ground_info(const Coord2D& coords, W3CPInfo& info) const;
    bool collect_cellpoint_cliff_info(const Coord2D& coords, W3CPInfo& info) const;

    const auto* map() const;

    gsl::not_null<const W3MapAssets*> map_asset_;
};

}  // namespace w3terr

#endif // _W3MAPINFORMATOR_IMPL__H
