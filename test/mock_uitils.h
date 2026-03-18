#ifndef _MOCK_UTILS__H
#define _MOCK_UTILS__H

#include <fstream>
#include <gtest/gtest.h>

#include "w3terrain/w3mapsection.h"
#include "w3terrain/w3math.h"
#include "w3terrain/w3e.h"

namespace w3terr {

//NOLINTBEGIN(cppcoreguidelines-pro-type-reinterpret-cast)
inline
bool save_w3e(const char* filename, w3terr::W3e* w3e)
{
    std::ofstream out(filename, std::ios::out | std::ios::binary | std::ios::trunc);
    if (!out.is_open()) {
        return false;
    }

    static const uint32_t kMagik = 0x21453357U; // "W3E!"
    out.write(reinterpret_cast<const char*>(&kMagik), sizeof(kMagik));

    static const uint32_t kVersion = 11;
    out.write(reinterpret_cast<const char*>(&kVersion), sizeof(kVersion));

    out.write(reinterpret_cast<const char*>(&w3e->main_tileset_id_), sizeof(w3e->main_tileset_id_));
    out.write(reinterpret_cast<const char*>(&w3e->custom_tileset_flag_), sizeof(w3e->custom_tileset_flag_));

    out.write(reinterpret_cast<const char*>(&w3e->number_of_ground_tilesets_), sizeof(w3e->number_of_ground_tilesets_));
    for(size_t i = 0; i < w3e->number_of_ground_tilesets_; ++i) {
        static const uint32_t kTileset = 0;
        out.write(reinterpret_cast<const char*>(&kTileset), sizeof(kTileset));
    }

    out.write(reinterpret_cast<const char*>(&w3e->number_of_geo_tilesets_), sizeof(w3e->number_of_geo_tilesets_));
    for(size_t i = 0; i < w3e->number_of_geo_tilesets_; ++i) {
        static const uint32_t kTileset = 0;
        out.write(reinterpret_cast<const char*>(&kTileset), sizeof(kTileset));
    }

    out.write(reinterpret_cast<const char*>(&w3e->map_size_x_), sizeof(w3e->map_size_x_));
    out.write(reinterpret_cast<const char*>(&w3e->map_size_y_), sizeof(w3e->map_size_y_));

    out.write(reinterpret_cast<const char*>(&w3e->map_3d_offset_x_), sizeof(w3e->map_3d_offset_x_));
    out.write(reinterpret_cast<const char*>(&w3e->map_3d_offset_z_), sizeof(w3e->map_3d_offset_z_));

    for(int32_t i = 0; i < w3e->map_size_y_; ++i) {
        for(int32_t j = 0; j < w3e->map_size_x_; ++j) {
            const w3terr::W3eCell &cell_point = w3e->get_cellpoint(j, i);
            out.write(reinterpret_cast<const char*>(&cell_point), sizeof(cell_point));
        }
    }
    return true;
}
//NOLINTEND(cppcoreguidelines-pro-type-reinterpret-cast)

}  // namespace w3terr

namespace w3terr::test {


using w3terr::math::bbox3;

inline
auto create_section_array4()
{
    auto section00 = w3terr::W3MapSection(nullptr);
    section00.initialize(
        2, 1,
        { 0, 0 },
        bbox3 { {-512.0F, 0.0F, 0.0F}, {0.0F, 32.0F, 512.0F} });

    auto section01 = w3terr::W3MapSection(nullptr);
    section01.initialize(
        2, 1,
        { 4, 0 },
        bbox3 { {-512.0F, -32.0F, -512.0F}, {0.0F, 0.0F, 0.0F} });

    auto section10 = w3terr::W3MapSection(nullptr);
    section10.initialize(
        2, 1,
        { 0, 4 },
        bbox3 { {0.0F, -32.0F, 0.0F}, {512.0F, 32.0F, 512.0F} });

    auto section11 = w3terr::W3MapSection(nullptr);
    section11.initialize(
        2, 1,
        { 4, 4 },
        bbox3 { {0.0F, 0.0F, -512.0F}, {512.0F, 0.0F, 0.0F} });

    return std::vector<w3terr::W3MapSection> { section00, section01, section10, section11 };
}

inline
auto create_test_w3e_5x5(int low_layer_x, int low_layer_y)
{
    w3terr::W3e w3e;
    w3e.create_empty(
        5, 5,
        2, 2,
        -128.0F, -128.0F
    );
    const auto base_layer = w3e.get_cellpoint_layer(low_layer_x, low_layer_y);
    w3e.set_cellpoint_layer(low_layer_x, low_layer_y, base_layer - 1);
    w3e.get_cellpoint(low_layer_x, low_layer_y).set_flag(w3terr::W3eCell::WATER);

    w3e.get_cellpoint(0, 1).ground_variation = 1;
    w3e.get_cellpoint(0, 2).ground_variation = 2;
    w3e.get_cellpoint(0, 3).ground_variation = 3;
    w3e.get_cellpoint(0, 4).ground_variation = 4;

    w3e.get_cellpoint(1, 0).ground_variation = 5;
    w3e.get_cellpoint(2, 0).ground_variation = 6;
    w3e.get_cellpoint(3, 0).ground_variation = 7;
    w3e.get_cellpoint(4, 0).ground_variation = 8;
    return w3e;
}

inline
auto create_test_w3e_9x9(const int cliff_x = 3, const int cliff_y = 3)
{
    w3terr::W3e w3e;
    w3e.create_empty(
        9, 9,
        2, 2,
        -1024.0F, -1024.0F
    );

    const auto base_layer = w3e.get_cellpoint_layer(0, 0);
    for(int cy = cliff_y; cy < w3e.get_map_2d_size_y(); ++cy) {
        for(int cx = cliff_x; cx < w3e.get_map_2d_size_x(); ++cx) {
            w3e.set_cellpoint_layer(cx, cy, base_layer - 1);
            w3e.get_cellpoint(cx, cy).set_flag(w3terr::W3eCell::WATER);
        }
    }

    w3e.get_cellpoint(0, 1).ground_variation = 1;
    w3e.get_cellpoint(0, 2).ground_variation = 2;
    w3e.get_cellpoint(0, 3).ground_variation = 3;
    w3e.get_cellpoint(0, 4).ground_variation = 4;
    w3e.get_cellpoint(0, 5).ground_variation = 5;
    w3e.get_cellpoint(0, 6).ground_variation = 6;
    w3e.get_cellpoint(0, 7).ground_variation = 7;
    w3e.get_cellpoint(0, 8).ground_variation = 8;

    w3e.get_cellpoint(1, 0).ground_variation = 9;
    w3e.get_cellpoint(2, 0).ground_variation = 10;
    w3e.get_cellpoint(3, 0).ground_variation = 11;
    w3e.get_cellpoint(4, 0).ground_variation = 12;
    w3e.get_cellpoint(5, 0).ground_variation = 13;
    w3e.get_cellpoint(6, 0).ground_variation = 14;
    w3e.get_cellpoint(7, 0).ground_variation = 15;
    return w3e;
}


inline
auto create_test_w3e_9x9_ramp_v(
    const int cliff_y = 4,
    const int ramp_h_pos = 3,
    const int ramp_h_size = 3
)
{
    w3terr::W3e w3e;
    w3e.create_empty(
        9, 9,
        2, 2,
        -1024.0F, -1024.0F
    );

    // generate horizontal cliff
    const auto base_layer = w3e.get_cellpoint_layer(0, 0);
    for(int cy = cliff_y; cy < w3e.get_map_2d_size_y(); ++cy) {
        for(int cx = 0; cx < w3e.get_map_2d_size_x(); ++cx) {
            w3e.set_cellpoint_layer(cx, cy, base_layer - 1);
            w3e.get_cellpoint(cx, cy).set_flag(w3terr::W3eCell::WATER);
        }
    }

    // generate vertical ramp
    for (int i = 0; i < ramp_h_size; ++i) {
        w3e.get_cellpoint(ramp_h_pos + i, cliff_y - 1).set_flag(w3terr::W3eCell::RAMP);
        w3e.get_cellpoint(ramp_h_pos + i, cliff_y    ).set_flag(w3terr::W3eCell::RAMP);
        w3e.get_cellpoint(ramp_h_pos + i, cliff_y + 1).set_flag(w3terr::W3eCell::RAMP);
    }
    return w3e;
}

inline
auto create_test_w3e_9x9_ramp_h(
    const int cliff_x = 4,
    const int ramp_v_pos = 3,
    const int ramp_v_size = 3
)
{
    w3terr::W3e w3e;
    w3e.create_empty(
        9, 9,
        2, 2,
        -1024.0F, -1024.0F
    );

    // generate vertical cliff
    const auto base_layer = w3e.get_cellpoint_layer(0, 0);
    for(int cy = 0; cy < w3e.get_map_2d_size_y(); ++cy) {
        for(int cx = cliff_x; cx < w3e.get_map_2d_size_x(); ++cx) {
            w3e.set_cellpoint_layer(cx, cy, base_layer - 1);
            w3e.get_cellpoint(cx, cy).set_flag(w3terr::W3eCell::WATER);
        }
    }

    // generate horizontal ramp
    for (int i = 0; i < ramp_v_size; ++i) {
        w3e.get_cellpoint(cliff_x - 1, ramp_v_pos + i).set_flag(w3terr::W3eCell::RAMP);
        w3e.get_cellpoint(cliff_x    , ramp_v_pos + i).set_flag(w3terr::W3eCell::RAMP);
        w3e.get_cellpoint(cliff_x + 1, ramp_v_pos + i).set_flag(w3terr::W3eCell::RAMP);
    }
    return w3e;
}

inline
auto create_test_w3e_5x9(
    const int cliff_low_x = 4,
    const int cliff_high_x = 0
)
{
    w3terr::W3e w3e;
    w3e.create_empty(
        5, 9,
        2, 2,
        -512.0F, -1024.0F
    );

    // generate vertical low cliff
    const auto base_layer = w3e.get_cellpoint_layer(0, 0);
    for(int cy = 0; cy < w3e.get_map_2d_size_y(); ++cy) {
        for(int cx = std::max(0, cliff_low_x); cx < w3e.get_map_2d_size_x(); ++cx) {
            w3e.set_cellpoint_layer(cx, cy, base_layer - 1);
            w3e.get_cellpoint(cx, cy).set_flag(w3terr::W3eCell::WATER);
        }
    }

    // generate vertical high cliff
    for(int cy = 0; cy < w3e.get_map_2d_size_y(); ++cy) {
        for(int cx = 0; cx <= std::min(cliff_high_x, w3e.get_map_2d_size_x()-1); ++cx) {
            w3e.set_cellpoint_layer(cx, cy, base_layer + 1);
            w3e.get_cellpoint(cx, cy).set_flag(w3terr::W3eCell::WATER);
        }
    }

    save_w3e("test_w3e_5x9.w3e", &w3e);
    return w3e;
}

}  // namespace w3terr::test

#endif // _MOCK_UTILS__H