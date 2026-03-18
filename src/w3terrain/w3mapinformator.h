#ifndef _W3MAPINFORMATOR__H
#define _W3MAPINFORMATOR__H

#include <gsl/gsl>

#include "w3defs.h"
#include "w3e.h"

namespace w3terr {

class W3MapAssetsImpl;

class W3_API W3MapInformator {
public:
    virtual ~W3MapInformator() = default;

    struct W3LayerInfo {
        uint8_t mask: 4; // < 16
        uint8_t tileset_id;

        bool operator > (const W3LayerInfo &other) const
        {
            return tileset_id > other.tileset_id;
        }

        static void sort_four_layers_by_tileset_id(
            W3LayerInfo& li0, W3LayerInfo& li1, W3LayerInfo& li2, W3LayerInfo& li3
        ) {
            if (li0 > li1) { std::swap(li0, li1); }
            if (li2 > li3) { std::swap(li2, li3); }
            if (li0 > li2) { std::swap(li0, li2); }
            if (li1 > li3) { std::swap(li1, li3); }
            if (li1 > li2) { std::swap(li1, li2); }
        }
    };

    struct W3CPInfo {
        enum : uint8_t {
            INVISIBLE   = 0,
            GROUND      = (1U << 0U),
            GEOCLIFF    = (1U << 1U),
            GEORAMP     = (1U << 2U),
            RAMP_MIDDLE = (1U << 3U),
            RAMP        = (1U << 4U),
            WATER       = (1U << 5U),
            BOUNDARY    = (1U << 6U),
            BLIGHT      = (1U << 7U)
        };

        bool check_flag(uint32_t flag) const { return (flags & flag) != 0; }

        uint32_t flags;
        uint32_t key;
        uint32_t geo_tileset;
        uint16_t height_layer;
        uint16_t ground_height;
        uint16_t water_height;
        math::vector3 normal;

        std::array<W3LayerInfo, kW3MapMapMaxGroundLayers> ground_layers;
    };

    virtual W3CPInfo collect_cellpoint_info(const Coord2D& coords) const = 0;
};

}  // namespace w3terr

#endif // _W3MAPINFORMATOR__H
