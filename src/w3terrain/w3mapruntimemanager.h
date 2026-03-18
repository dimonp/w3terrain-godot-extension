#ifndef _W3MAPRUNTIME__H
#define _W3MAPRUNTIME__H

#include <optional>
#include <gsl/gsl>

#include "w3defs.h"
#include "w3math.h"

namespace w3terr {

class W3MapRuntimeManager {
public:
    struct CellPointRT {
        static constexpr uint8_t kEmptyTilesetId = 0xFF;
        static constexpr uint8_t kMaxGroundLayers = 4U;

        union {
            struct { // ground type
                uint32_t tileset_ids; // four packed tileset IDs
                uint32_t uv_indices; // four packed ground tileset texture indices
            };
            struct { // geo type
                uint16_t vertices_count;
                uint16_t indices_count;
                uint16_t tileset_id;
                uint16_t geoset_id;
            };
        };

        uint32_t packed_normal;
        uint8_t flags; // runtime cell flags

        enum Flags : uint8_t {
            INVICIBLE    = 0,
            GROUND       = 0b00001,
            GEO_CLIFF    = 0b00010,
            GEO_RAMP     = 0b00100,
            WATER        = 0b01000,
            RAMP_MIDDLE  = 0b10000
        };

        uint32_t get_ground_tileset_id(uint8_t layer_idx) const
        {
            return (tileset_ids >> (static_cast<uint32_t>(layer_idx) << 3U)) & 0b11111111U;
        }

        void set_ground_tileset_id(uint8_t layer_idx, uint32_t tileset_id = kEmptyTilesetId)
        {
            tileset_ids |= tileset_id << (static_cast<uint32_t>(layer_idx) << 3U);
        }

        W3UInt8Pair get_ground_tile_uv_indices(uint8_t layer_idx) const
        {
            return {
                ( uv_indices >> ((layer_idx & 3U) << 3U)) & 0b1111U,
                ((uv_indices >> ((layer_idx & 3U) << 3U)) >> 4U) & 0b1111U
            };
        }

        void set_ground_tile_uv_indices(uint8_t layer_idx, uint8_t tile_idx_u, uint8_t tile_idx_v)
        {
            uv_indices |= ((tile_idx_u & 15U) | ((tile_idx_v & 15U) << 4U)) << (static_cast<uint32_t>(layer_idx) << 3U);
        }

        bool check_flag(Flags flag) const
        {
            return (this->flags & flag) != 0;
        }
    };

    virtual ~W3MapRuntimeManager() = default;

    virtual const CellPointRT& get_cellpoint_rt(const Coord2D& coords) const = 0;

    virtual float get_cellpoint_layer_height(const Coord2D& coords) const = 0;
    virtual float get_cellpoint_ground_height(const Coord2D& coords) const = 0;
    virtual math::vector3 get_cellpoint_position(const Coord2D& coords) const = 0;
    virtual math::vector3 get_cellpoint_water_position(const Coord2D& coords) const = 0;
    virtual math::bbox3 get_cell_bbox(const Coord2D& coords) const = 0;
    virtual float get_cell_ground_height(const Coord2D& coords, float t_lerp, float s_lerp) const = 0;

    virtual bool test_cell_intersection(const Coord2D& coords, const math::line3& line) const = 0;
    virtual std::optional<math::vector3> get_cell_intersection_point(const Coord2D& coords, const math::line3& line) const = 0;

    virtual void update_all_cells_rt() = 0;
    virtual void update_cell_rt(const Coord2D& coords) = 0;
    virtual void update_area_rt(const Coord2D& coords, int32_t area_margin) = 0;

    virtual bool is_dirty() const = 0;
    virtual void set_dirty(bool flag) = 0;
};

}  // namespace w3terr

#endif // _W3MAPRUNTIME__H