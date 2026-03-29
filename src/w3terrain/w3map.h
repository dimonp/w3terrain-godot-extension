#ifndef _W3MAP__H
#define _W3MAP__H

#include <cstdint>

namespace w3terr {

inline constexpr int32_t kSectionDimension = 4;

inline constexpr float kW3MapTile2DSize             = 128.0F;
inline constexpr float kW3MapTile2DHalfSize         = kW3MapTile2DSize / 2.0F;
inline constexpr float kW3MapTile2DInvSize          = 1.0F / kW3MapTile2DSize;
inline constexpr float kGroundTextureTileSize       = 64.0F;

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

