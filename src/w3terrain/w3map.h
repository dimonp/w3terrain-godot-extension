#ifndef _W3MAP__H
#define _W3MAP__H

#include <cstdint>

namespace w3terr {

inline constexpr std::size_t kSectionDimension = 4;

inline constexpr float kW3MapTile2DSize             = 128.0F;
inline constexpr float kW3MapTile2DHalfSize         = kW3MapTile2DSize / 2.0F;
inline constexpr float kW3MapTile2DInvSize          = 1.0F / kW3MapTile2DSize;
inline constexpr float kGroundTextureTileSize       = 64.0F;

inline
std::size_t
calc_map_array_size(uint32_t map_size_x, uint32_t map_size_y)
{
    const std::size_t array_size = static_cast<std::size_t>(
        map_size_x + kSectionDimension - 1) * (map_size_y + kSectionDimension - 1 // + 1 padding cells
    );
    return array_size;
}

inline
std::size_t
calc_map_array_index(uint32_t coord_x, uint32_t coord_y, uint32_t map_size_x)
{
    // Iterate cellpoints in cache-friendly order
    // cells:    0123456..n      0123456..n     0123456..n
    // row 0: .. section 00 .... section 01 ... section 02 ...
    // row 1: .. section 10 .... section 11 ... section 12 ...
    // row 2: .. section 20 .... section 21 ... section 22 ...
    // row 3: .. section 30 .... section 31 ... section 32 ...

    const std::size_t num_sections_x = map_size_x / kSectionDimension;
    const uint32_t sec_x = coord_x / kSectionDimension;
    const uint32_t sec_y = coord_y / kSectionDimension;
    const uint32_t loc_x = coord_x % kSectionDimension;
    const uint32_t loc_y = coord_y % kSectionDimension;

    const std::size_t full_row_size = ((num_sections_x * kSectionDimension) + 1) * kSectionDimension;
    // Global offset to the start of the current horizontal band of sections
    const std::size_t global_offset = sec_y * full_row_size;

    // Offset to a specific section within a row
    const std::size_t sec_in_row_offset = sec_x * kSectionDimension * kSectionDimension;

    // Local offset within a section
    const int current_sec_w = (sec_x < num_sections_x) ? kSectionDimension : 1;
    const std::size_t local_offset = static_cast<std::size_t>(loc_y * current_sec_w) + loc_x;

    return global_offset + sec_in_row_offset + local_offset;
}

inline
float
get_3d_coord_from_2d_x(int32_t idx)
{
    return static_cast<float>(idx) * kW3MapTile2DSize;
}

inline
float
get_3d_coord_from_2d_z(int32_t idx)
{
    return -static_cast<float>(idx) * kW3MapTile2DSize;
}

}  // namespace w3terr

#endif  /// _W3MAP__H

