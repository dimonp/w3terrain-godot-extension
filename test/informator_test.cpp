#include "gtest/gtest.h"
#include <gmock/gmock.h>

#include <algorithm>
#include <cmath>

#include "w3terrain/w3mapassets.h"
#include "w3terrain/w3mapinformator_impl.h"
#include "mock_uitils.h"

class MockAssets : public w3terr::W3MapAssets {
public:
    MockAssets() 
    {
        ground_assets_rt_.resize(2);
        geo_assets_rt_.resize(2);
    }

    MOCK_METHOD(w3terr::W3e*, get_w3e, (), (const, override));
};

class InformatorTestFixture : public ::testing::Test {
protected:
    static constexpr int kCliffCellX = 3;
    static constexpr int kCliffCellY = 3;

    w3terr::W3e w3e_5x5_;
    w3terr::W3e w3e_9x9_;
    w3terr::W3e w3e_5x9_;
    ::testing::NiceMock<MockAssets> assets_;

    void SetUp() override
    {
        w3e_5x5_ = w3terr::test::create_test_w3e_5x5(kCliffCellX, kCliffCellY);
        w3e_9x9_ = w3terr::test::create_test_w3e_9x9(kCliffCellX, kCliffCellY);
        w3e_5x9_ = w3terr::test::create_test_w3e_5x9();
    }

    void TearDown() override {}
};

using W3CPInfo = w3terr::W3MapInformator::W3CPInfo;

TEST_F(InformatorTestFixture, CheckGroundAndCliffCells)
{
    ON_CALL(assets_, get_w3e()).WillByDefault(testing::Return(&w3e_5x5_));

    const std::vector<w3terr::Coord2D> cliff_coords = {
        { kCliffCellX - 1,   kCliffCellY - 1 },
        { kCliffCellX - 1,   kCliffCellY     },
        { kCliffCellX,       kCliffCellY - 1 },
        { kCliffCellX,       kCliffCellY     }
    };

    w3terr::W3MapInformatorImpl sut(&assets_);

    for(int cy = 0; cy < 4; cy++) {
        for(int cx = 0; cx < 4; cx++) {
            const W3CPInfo info = sut.collect_cellpoint_info(w3terr::Coord2D(cx, cy));
            if (std::ranges::find(cliff_coords, w3terr::Coord2D(cx, cy)) != cliff_coords.end()) {
                EXPECT_TRUE(info.check_flag(W3CPInfo::GEOCLIFF));
            } else {
                EXPECT_TRUE(info.check_flag(W3CPInfo::GROUND));
            }
        }
    }
}

TEST_F(InformatorTestFixture, CheckBoundaryCells)
{
    ON_CALL(assets_, get_w3e()).WillByDefault(testing::Return(&w3e_5x5_));

    w3terr::W3MapInformatorImpl sut(&assets_);

    // map size is 5x5, boundary cellpoints are x=4 or y=4
    for (int cy = 0; cy < 5; ++cy) {
        for (int cx = 0; cx < 5; ++cx) {
            if (cx == 4 || cy == 4) {
                const W3CPInfo info = sut.collect_cellpoint_info(w3terr::Coord2D(cx, cy));
                // Should have no ground, cliff, or ramp flags
                EXPECT_FALSE(info.check_flag(W3CPInfo::GROUND));
                EXPECT_FALSE(info.check_flag(W3CPInfo::GEOCLIFF));
                EXPECT_FALSE(info.check_flag(W3CPInfo::GEORAMP));
                // Should have INVISIBLE? Actually boundary cells are not processed, they return early.
                // The flags should be zero (except maybe WATER? not set)
                EXPECT_EQ(info.flags, 0U);
            }
        }
    }
}

TEST_F(InformatorTestFixture, CheckWaterFlag)
{
    ON_CALL(assets_, get_w3e()).WillByDefault(testing::Return(&w3e_5x5_));

    const std::vector<w3terr::Coord2D> water_coords = {
        { kCliffCellX - 1,   kCliffCellY - 1 },
        { kCliffCellX - 1,   kCliffCellY     },
        { kCliffCellX,       kCliffCellY - 1 },
        { kCliffCellX,       kCliffCellY     }
    };

    w3terr::W3MapInformatorImpl sut(&assets_);

    for(const auto& coord : water_coords) {
        const W3CPInfo info = sut.collect_cellpoint_info(coord);
        EXPECT_TRUE(info.check_flag(W3CPInfo::WATER));
    }
}

TEST_F(InformatorTestFixture, CheckGroundLayers)
{
    ON_CALL(assets_, get_w3e()).WillByDefault(testing::Return(&w3e_5x5_));

    w3terr::W3MapInformatorImpl sut(&assets_);

    // Pick a non-boundary, non-cliff, non-ramp cellpoint
    const W3CPInfo info = sut.collect_cellpoint_info({1, 1});
    EXPECT_TRUE(info.check_flag(W3CPInfo::GROUND));
    // ground_layers_array should be populated
    // Each layer should have a mask and tileset_id
    for (const auto& layer : info.ground_layers) {
        EXPECT_NE(layer.mask, 0U);
        // tileset_id should be within ground tilesets count (2)
        EXPECT_LT(layer.tileset_id, 2U);
    }
}

TEST_F(InformatorTestFixture, CheckNormal)
{
    ON_CALL(assets_, get_w3e()).WillByDefault(testing::Return(&w3e_5x5_));

    w3terr::W3MapInformatorImpl sut(&assets_);

    const W3CPInfo info = sut.collect_cellpoint_info({1, 1});
    // Normal should be non-zero (default flat terrain normal is (0,1,0))
    // Actually normal is calculated from height differences; we can just ensure it's not NaN.
    EXPECT_FALSE(std::isnan(info.normal.x));
    EXPECT_FALSE(std::isnan(info.normal.y));
    EXPECT_FALSE(std::isnan(info.normal.z));
}

TEST_F(InformatorTestFixture, CheckGeoCliffTilesetMapping)
{
    ON_CALL(assets_, get_w3e()).WillByDefault(testing::Return(&w3e_5x5_));

    w3terr::W3MapInformatorImpl sut(&assets_);

    // Cliff cell (1,1) should have geo_tileset not 15 (since geo tilesets count is 2)
    const W3CPInfo info = sut.collect_cellpoint_info({kCliffCellX - 1, kCliffCellY - 1});
    EXPECT_TRUE(info.check_flag(W3CPInfo::GEOCLIFF));
    EXPECT_LT(info.geo_tileset, 2U);
}

TEST_F(InformatorTestFixture, CheckGroundKey)
{
    ON_CALL(assets_, get_w3e()).WillByDefault(testing::Return(&w3e_5x5_));

    w3terr::W3MapInformatorImpl sut(&assets_);

    // Cell (0,1) has ground_variation = 1, ground_tileset = 0
    const W3CPInfo info = sut.collect_cellpoint_info({0, 1});
    EXPECT_TRUE(info.check_flag(W3CPInfo::GROUND));
    // key = pack_ground_key(t10=0, t00=0, t11=0, t01=0, variation=1)
    // pack_ground_key places variation in bits 16-19
    const uint32_t expected_key = 1U << 16U;
    EXPECT_EQ(info.key, expected_key);
}

TEST_F(InformatorTestFixture, CheckCliffKey)
{
    ON_CALL(assets_, get_w3e()).WillByDefault(testing::Return(&w3e_5x5_));

    w3terr::W3MapInformatorImpl sut(&assets_);

    // Cliff cell (2,2) (since kCliffCellX=3, kCliffCellY=3)
    const W3CPInfo info = sut.collect_cellpoint_info({kCliffCellX - 1, kCliffCellY - 1});
    EXPECT_TRUE(info.check_flag(W3CPInfo::GEOCLIFF));
    // key = pack_cliff_key(layer differences 1,1,0,1, variation=0) = 0x45
    const uint32_t expected_key = (1U << 0U) | (1U << 2U) | (1U << 6U); // 0x45
    EXPECT_EQ(info.key, expected_key);
}

TEST_F(InformatorTestFixture, CheckNoWaterFlagOnDryLand)
{
    ON_CALL(assets_, get_w3e()).WillByDefault(testing::Return(&w3e_5x5_));

    w3terr::W3MapInformatorImpl sut(&assets_);

    // Cell (0,0) has no water
    const W3CPInfo info = sut.collect_cellpoint_info({0, 0});
    EXPECT_FALSE(info.check_flag(W3CPInfo::WATER));
}

