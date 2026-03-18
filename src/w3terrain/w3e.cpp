#include "w3e.h"

#include <cstddef>
#include <gsl/gsl>

namespace w3terr {

bool
W3e::initialize()
{
    Expects(map_cells_.empty());
    if (map_size_x_ > 0 && map_size_y_ > 0) {
        map_cells_.assign(static_cast<size_t>(map_size_x_) * map_size_y_, {});

        for(int32_t i = 0; i < map_size_y_; ++i) {
            for(int32_t j = 0; j < map_size_x_; ++j) {
                W3eCell &cell_point = get_cellpoint(j,i);
                cell_point.ground_tileset = 0;
                cell_point.height_layer = kW3MapMapBaseHeightLayer;
                cell_point.ground_height = kW3MapMapBaseHeight;
            }
        }
        return true;
    }
    w3_log_error("nTileMap::CreateEmpty: Failed to create empty tilemap.");
    return false;
}

bool
W3e::set_dimension_2d(int32_t dim_2d_x, int32_t dim_2d_y)
{
    if (dim_2d_x > 4 && dim_2d_y > 4 &&
        (dim_2d_x - 1) % 4 == 0 && (dim_2d_y - 1) % 4 == 0) {
        map_size_x_ = dim_2d_x;
        map_size_y_ = dim_2d_y;
        return true;
    }
    return false;
}

bool
W3e::create_empty(
    int32_t dim_2d_x, int32_t dim_2d_y,
    uint32_t ground_tilesets, uint32_t geo_tilesets,
    float map_3d_offset_x, float map_3d_offset_z
) {
    map_3d_offset_x_ = map_3d_offset_x;
    map_3d_offset_z_ = map_3d_offset_z;
    number_of_ground_tilesets_ = ground_tilesets;
    number_of_geo_tilesets_ = geo_tilesets;
    map_cells_.clear();
    return set_dimension_2d(dim_2d_x, dim_2d_y) && initialize();
}

math::vector3
W3e::get_cellpoint_position(int32_t coord_2d_x, int32_t coord_2d_y) const
{
    math::vector3 pos = map_cellpoint_position_2d_to_3d(coord_2d_x, coord_2d_y);
    pos.y = get_cellpoint_total_height(coord_2d_x, coord_2d_y);
    return pos;
}

math::vector3
W3e::get_cellpoint_water_position(int32_t coord_2d_x, int32_t coord_2d_y) const
{
    math::vector3 pos = map_cellpoint_position_2d_to_3d(coord_2d_x, coord_2d_y);
    pos.y = get_cellpoint_water_height(coord_2d_x, coord_2d_y);
    return pos;
}

math::bbox3
W3e::calc_cell_bbox(int32_t coord_2d_x, int32_t coord_2d_y) const
{
    math::bbox3 bbox;
    bbox.begin_extend();

    math::vector3 pos;
    pos = get_cellpoint_position(coord_2d_x, coord_2d_y);
    bbox.extend(pos);

    pos = get_cellpoint_position(coord_2d_x + 1, coord_2d_y);
    bbox.extend(pos);

    pos = get_cellpoint_position(coord_2d_x + 1, coord_2d_y + 1);
    bbox.extend(pos);

    pos = get_cellpoint_position(coord_2d_x,     coord_2d_y + 1);
    bbox.extend(pos);

    const W3eCell& cell = get_cellpoint(coord_2d_x, coord_2d_y);
    if (cell.check_flag(W3eCell::WATER)) {
        const math::vector3 pos = get_cellpoint_water_position(coord_2d_x, coord_2d_y);
        bbox.extend(pos);
    }
    return bbox;
}

math::bbox3
W3e::calc_cellpoints_bbox(int32_t from_2d_x, int32_t from_2d_y, int32_t dim) const
{
    math::bbox3 bbox;
    bbox.begin_extend();

    int32_t to_x = from_2d_x + dim;
    int32_t to_y = from_2d_y + dim;

    for(int32_t cell_y = from_2d_y; cell_y <= to_y; ++cell_y) {
        for(int32_t cell_x = from_2d_x; cell_x <= to_x; ++cell_x) {
            if (!is_valid_cellpoint(cell_x, cell_y)) {
                continue;
            }
            const math::vector3 pos = get_cellpoint_position(cell_x, cell_y);
            bbox.extend(pos);
        }
    }
    return bbox;
}


//------------------------------------------------------------------------------
/**
 *   @brief Calculate cellpoint normal.
 *   Uses Sobel filter.  Two kernels, for x and z:
 *
 *   X                   Z
 *   -1   0   1           1   2   1
 *   -2   0   2           0   0   0
 *   -1   0   1          -1  -2  -1
 *
 */
math::vector3
W3e::calc_cellpoint_normal(const int32_t coord_2d_x, const int32_t coord_2d_y) const
{
    const int32_t y_dec = coord_2d_y + ((coord_2d_y != 0) ? -1 : 0);
    const int32_t y_inc = coord_2d_y + ((coord_2d_y != get_map_2d_size_y() - 1) ? 1 : 0);

    const int32_t x_dec = coord_2d_x + ((coord_2d_x != 0) ? -1 : 0);
    const int32_t x_inc = coord_2d_x + ((coord_2d_x != get_map_2d_size_x() - 1) ? 1 : 0);

    //  0  -X       +X
    // -Z  y00 y10 y20
    //     y01 y11 y21
    // +Z  y02 y12 y22

    const float y00 = get_cellpoint(x_dec, y_dec).get_ground_height();
    const float y01 = get_cellpoint(x_dec, coord_2d_y).get_ground_height();
    const float y02 = get_cellpoint(x_dec, y_inc).get_ground_height();
    const float y10 = get_cellpoint(coord_2d_x, y_dec).get_ground_height();
    const float y12 = get_cellpoint(coord_2d_x, y_inc).get_ground_height();
    const float y20 = get_cellpoint(x_inc, y_dec).get_ground_height();
    const float y21 = get_cellpoint(x_inc, coord_2d_y).get_ground_height();
    const float y22 = get_cellpoint(x_inc, y_inc).get_ground_height();

    // Calculate gradients
    float j_grad = -y00 - (2.0F * y01) - y02 + y20 + (2.0F * y21) + y22;
    j_grad /= 8.0F * kW3MapTile2DSize;

    float i_grad = -y00 - (2.0F * y10) - y20 + y02 + (2.0F * y12) + y22;
    i_grad /= 8.0F * kW3MapTile2DSize;

    float mag = (j_grad * j_grad) + (i_grad * i_grad) + 1.0F;
    mag = 1.0F / std::sqrt(mag);

    math::vector3 normal = { -j_grad * mag, mag, i_grad * mag };
    normal.normalize();
    return normal;
}

}  // namespace w3terr

