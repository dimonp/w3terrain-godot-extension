#ifndef _W3MAP__H
#define _W3MAP__H

#include "w3defs.h"
#include "w3math.h"

namespace w3terr {

inline constexpr uint8_t kW3MapMapBaseHeightLayer   = 2;
inline constexpr int32_t kW3MapMapBaseHeight        = 0x2000;
inline constexpr int32_t kW3MapMapMaxHeight         = 0x4000;
inline constexpr uint8_t kW3MapMapMaxGroundLayers   = 4;

inline constexpr float kW3MapTile2DSize             = 128.0F;
inline constexpr float kW3MapTile2DHalfSize         = kW3MapTile2DSize / 2.0F;
inline constexpr float kW3MapTile2DInvSize          = 1.0F / kW3MapTile2DSize;
inline constexpr float kGroundTextureTileSize       = 64.0F;

inline float get_3d_coord_from_2d_x(int32_t idx) {
    return static_cast<float>(idx) * kW3MapTile2DSize;
}

inline float get_3d_coord_from_2d_z(int32_t idx) {
    return -static_cast<float>(idx) * kW3MapTile2DSize;
}

#pragma pack(1)
struct W3eCell {
    uint32_t ground_height:16   = kW3MapMapBaseHeight;
    uint32_t water_height:14    = kW3MapMapBaseHeight;
    uint32_t boundary_flag:2    = 0;
    uint8_t ground_tileset:4    = 0;
    uint8_t flags:4             = 0;
    uint8_t ground_variation:5  = 0;
    uint8_t geo_variation:3     = 0;
    uint8_t height_layer:4      = kW3MapMapBaseHeightLayer;
    uint8_t geo_tileset:4       = 0;

    enum Flags: uint8_t {
        RAMP        = 0b0001,
        BLIGHT      = 0b0010,
        WATER       = 0b0100,
        BOUNDARY    = 0b1000
    };

    float get_layer_height() const
    {
        return (static_cast<float>(height_layer) - kW3MapMapBaseHeightLayer) * kW3MapTile2DSize;
    }

    float get_ground_height() const
    {
        return static_cast<float>(static_cast<int32_t>(ground_height) - kW3MapMapBaseHeight) * 0.25F;
    }

    void set_ground_height(float height)
    {
        const auto ground_height = static_cast<uint16_t>((height / 0.25F) + kW3MapMapBaseHeight);
        if (ground_height <= kW3MapMapMaxHeight) {
            this->ground_height = ground_height;
        }
    }

    float get_water_height() const
    {
        return static_cast<float>(static_cast<int32_t>(water_height) - kW3MapMapBaseHeight - 358) * 0.25F;
    }

    void set_water_height(float height)
    {
        const auto water_height = static_cast<uint16_t>((height / 0.25F) + kW3MapMapBaseHeight + 358);
        if (water_height <= kW3MapMapMaxHeight) {
            this->water_height = water_height;
        }
    }

    float calc_total_height() const
    {
        return get_layer_height() + get_ground_height();
    }

    bool check_flag(Flags flag) const
    {
        return (this->flags & flag) != 0;
    }

    uint8_t set_flag(Flags flag)
    {
        return this->flags |= flag;
    }
};
#pragma pack()

class W3_API W3e {
public:
    const W3eCell& get_cellpoint(int32_t coord_2d_x, int32_t coord_2d_y) const;
    W3eCell& get_cellpoint(int32_t coord_2d_x, int32_t coord_2d_y);

    bool is_valid_cellpoint(int32_t coord_2d_x, int32_t coord_2d_y) const;
    bool is_valid_cell(int32_t coord_2d_x, int32_t coord_2d_y) const;

    uint32_t get_ground_tilesets_count() const;
    uint32_t get_geo_tilesets_count() const;
    int32_t get_map_2d_size_x() const;
    int32_t get_map_2d_size_y() const;
    float get_map_3d_offset_x() const;
    float get_map_3d_offset_z() const;

    bool create_empty(
        int32_t dim_2d_x, int32_t dim_2d_y,
        uint32_t ground_tilesets, uint32_t geo_tilesets,
        float map_3d_offset_x = 0.0F, float map_3d_offset_z = 0.0F
    );

    float get_cellpoint_layer_height(int32_t coord_2d_x, int32_t coord_2d_y) const;
    float get_cellpoint_ground_height(int32_t coord_2d_x, int32_t coord_2d_y) const;
    float get_cellpoint_water_height(int32_t coord_2d_x, int32_t coord_2d_y) const;

    void set_cellpoint_ground_height(int32_t coord_2d_x, int32_t coord_2d_y, float height);
    void set_cellpoint_water_height(int32_t coord_2d_x, int32_t coord_2d_y, float height);

    uint8_t get_cellpoint_layer(int32_t coord_2d_x, int32_t coord_2d_y) const;
    void set_cellpoint_layer(int32_t coord_2d_x, int32_t coord_2d_y, uint8_t layer);

    float get_cellpoint_total_height(int32_t coord_2d_x, int32_t coord_2d_y) const;

    static math::vector3 map_cellpoint_position_2d_to_3d(int32_t coord_2d_x, int32_t coord_2d_y) ;
    math::vector3 get_cellpoint_position(int32_t coord_2d_x, int32_t coord_2d_y) const;
    math::vector3 get_cellpoint_water_position(int32_t coord_2d_x, int32_t coord_2d_y) const;

    math::vector3 calc_cellpoint_normal(int32_t coord_2d_x, int32_t coord_2d_y) const;
    math::bbox3 calc_cell_bbox(int32_t coord_2d_x, int32_t coord_2d_y) const;
    math::bbox3 calc_cellpoints_bbox(int32_t from_2d_x, int32_t from_2d_y, int32_t dim) const;

protected:
    bool initialize();
    bool set_dimension_2d(int32_t dim_2d_x, int32_t dim_2d_y);

    W3Array<W3eCell> map_cells_;

    uint8_t     main_tileset_id_ = 0;
    uint32_t    custom_tileset_flag_ = 0;

    uint32_t    number_of_ground_tilesets_ = 0;
    uint32_t    number_of_geo_tilesets_ = 0;

    int32_t     map_size_x_ = 0;
    int32_t     map_size_y_ = 0;
    float       map_3d_offset_x_ = 0.0;
    float       map_3d_offset_z_ = 0.0;

    friend bool save_w3e(const char*, w3terr::W3e*);
};

inline
bool
W3e::is_valid_cellpoint(int32_t coord_2d_x, int32_t coord_2d_y) const
{
    return coord_2d_x >= 0 && coord_2d_y >= 0 &&
        coord_2d_x < map_size_x_ && coord_2d_y < map_size_y_;
}

inline
bool
W3e::is_valid_cell(int32_t coord_2d_x, int32_t coord_2d_y) const
{
    return coord_2d_x >= 0 && coord_2d_y >= 0 &&
        coord_2d_x < map_size_x_ - 1 && coord_2d_y < map_size_y_ - 1;
}

inline
const W3eCell&
W3e::get_cellpoint(int32_t coord_2d_x, int32_t coord_2d_y) const
{
    w3_assert(is_valid_cellpoint(coord_2d_x, coord_2d_y));
    return map_cells_[static_cast<size_t>(coord_2d_y * map_size_x_) + coord_2d_x];
}

inline
W3eCell&
W3e::get_cellpoint(int32_t coord_2d_x, int32_t coord_2d_y)
{
    w3_assert(is_valid_cellpoint(coord_2d_x, coord_2d_y));
    return map_cells_[static_cast<size_t>(coord_2d_y * map_size_x_) + coord_2d_x];
}

inline
uint32_t
W3e::get_ground_tilesets_count() const
{
    return number_of_ground_tilesets_;
}

inline
uint32_t
W3e::get_geo_tilesets_count() const
{
    return number_of_geo_tilesets_;
}

inline
int32_t
W3e::get_map_2d_size_x() const
{
    return map_size_x_;
}

inline
int32_t
W3e::get_map_2d_size_y() const
{
    return map_size_y_;
}

inline
float
W3e::get_map_3d_offset_x() const
{
    return map_3d_offset_x_;
}

inline
float
W3e::get_map_3d_offset_z() const
{
    return map_3d_offset_z_;
}

inline
math::vector3
W3e::map_cellpoint_position_2d_to_3d(int32_t coord_2d_x, int32_t coord_2d_y)
{
    math::vector3 pos;
    pos.x = get_3d_coord_from_2d_x(coord_2d_x);
    pos.z = get_3d_coord_from_2d_z(coord_2d_y); // 2D Y coordinate mapped to 3D Z axis
    return pos;
}

inline
float
W3e::get_cellpoint_layer_height(int32_t coord_2d_x, int32_t coord_2d_y) const
{
    return get_cellpoint(coord_2d_x, coord_2d_y).get_layer_height();
}

inline
float
W3e::get_cellpoint_ground_height(int32_t coord_2d_x, int32_t coord_2d_y) const
{
    return get_cellpoint(coord_2d_x, coord_2d_y).get_ground_height();
}

inline
float
W3e::get_cellpoint_water_height(int32_t coord_2d_x, int32_t coord_2d_y) const
{
    return get_cellpoint(coord_2d_x, coord_2d_y).get_water_height();
}

inline
float
W3e::get_cellpoint_total_height(int32_t coord_2d_x, int32_t coord_2d_y) const
{
    return get_cellpoint(coord_2d_x, coord_2d_y).calc_total_height();
}

inline
uint8_t
W3e::get_cellpoint_layer(int32_t coord_2d_x, int32_t coord_2d_y) const
{
    return get_cellpoint(coord_2d_x, coord_2d_y).height_layer;
}

inline
void
W3e::set_cellpoint_layer(int32_t coord_2d_x, int32_t coord_2d_y, uint8_t layer)
{
    get_cellpoint(coord_2d_x, coord_2d_y).height_layer = layer;
}

inline
void
W3e::set_cellpoint_ground_height(int32_t coord_2d_x, int32_t coord_2d_y, float height)
{
    get_cellpoint(coord_2d_x, coord_2d_y).set_ground_height(height);
}

inline
void
W3e::set_cellpoint_water_height(int32_t coord_2d_x, int32_t coord_2d_y, float height)
{
    get_cellpoint(coord_2d_x, coord_2d_y).set_water_height(height);
}

}  // namespace w3terr

#endif  /// _W3MAP__H

