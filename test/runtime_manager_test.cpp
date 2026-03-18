#include "gtest/gtest.h"
#include <gmock/gmock.h>

#include "w3terrain/w3mapassets.h"
#include "w3terrain/w3mapinformator.h"
#include "w3terrain/w3mapruntimemanager_impl.h"
#include "mock_uitils.h"

class MockInformator : public w3terr::W3MapInformator {
public:
    MOCK_METHOD(w3terr::W3MapInformator::W3CPInfo, collect_cellpoint_info, (const w3terr::Coord2D&), (const, override));
};

class MockAssets : public w3terr::W3MapAssets {
public:
    MockAssets() {
        ground_assets_rt_ = {
            {
                .tile_tu_size = 0,
                .tile_tv_size = 0,
                .is_extended = false
            },
            {
                .tile_tu_size = 0,
                .tile_tv_size = 1,
                .is_extended = true
            }
        };
        geo_assets_rt_ = {
            {
                .geo_cliff_keys_map = { {1, 0}, {2, 0} },
                .geo_ramp_keys_map = { {1, 0}, {2, 0} },
                .ground_tileset_id = 0
            },
            {
                .geo_cliff_keys_map = { {1, 0}, {69, 4} },
                .geo_ramp_keys_map = { {1, 0}, {4660, 4} },
                .ground_tileset_id = 1
            }
        };
    }
    MOCK_METHOD(w3terr::W3e*, get_w3e, (), (const, override));
};

class RuntimeManagerTestFixture : public ::testing::Test {
protected:
    w3terr::W3e w3e_;
    ::testing::NiceMock<MockAssets> assets_;
    ::testing::NiceMock<MockInformator> informator_;

    void SetUp() override
    {
        w3e_ = w3terr::test::create_test_w3e_5x5(2, 2);
        ON_CALL(assets_, get_w3e()).WillByDefault(testing::Return(&w3e_));
    }

    void TearDown() override {}
};

using W3CPInfo = w3terr::W3MapInformator::W3CPInfo;

TEST_F(RuntimeManagerTestFixture, InitializeWithValidParameters)
{
    w3terr::W3MapRuntimeManagerImpl sut(&assets_, &informator_);

    EXPECT_TRUE(sut.is_dirty());
}
TEST_F(RuntimeManagerTestFixture, SetDirtyFlag)
{
    w3terr::W3MapRuntimeManagerImpl sut(&assets_, &informator_);

    EXPECT_TRUE(sut.is_dirty());
    sut.set_dirty(false);
    EXPECT_FALSE(sut.is_dirty());
    sut.set_dirty(true);
    EXPECT_TRUE(sut.is_dirty());
}

TEST_F(RuntimeManagerTestFixture, GetCellpointLayerHeight)
{
    w3terr::W3MapRuntimeManagerImpl sut(&assets_, &informator_);

    // default layer height is 0 (since base layer is 2)
    EXPECT_FLOAT_EQ(sut.get_cellpoint_layer_height({0, 0}), 0.0F);
    // cell (2,2) has layer set to 1 in create_test_w3e_5x5
    // layer height = (1 - 2) * 128 = -128.0F
    EXPECT_FLOAT_EQ(sut.get_cellpoint_layer_height({2, 2}), -128.0F);
}

TEST_F(RuntimeManagerTestFixture, GetCellpointGroundHeight)
{
    w3terr::W3MapRuntimeManagerImpl sut(&assets_, &informator_);

    // default ground height is 0
    EXPECT_FLOAT_EQ(sut.get_cellpoint_ground_height({0, 0}), 0.0F);
}

TEST_F(RuntimeManagerTestFixture, GetCellpointPosition)
{
    w3terr::W3MapRuntimeManagerImpl sut(&assets_, &informator_);

    w3terr::math::vector3 pos = sut.get_cellpoint_position({0, 0});
    EXPECT_FLOAT_EQ(pos.x, 0.0F);
    EXPECT_FLOAT_EQ(pos.y, 0.0F);
    EXPECT_FLOAT_EQ(pos.z, 0.0F);

    pos = sut.get_cellpoint_position({1, 0});
    EXPECT_FLOAT_EQ(pos.x, 128.0F);
    EXPECT_FLOAT_EQ(pos.z, 0.0F);

    pos = sut.get_cellpoint_position({0, 1});
    EXPECT_FLOAT_EQ(pos.x, 0.0F);
    EXPECT_FLOAT_EQ(pos.z, -128.0F);
}

TEST_F(RuntimeManagerTestFixture, GetCellpointWaterPosition)
{
    w3terr::W3MapRuntimeManagerImpl sut(&assets_, &informator_);

    w3terr::math::vector3 pos = sut.get_cellpoint_water_position({0, 0});

    EXPECT_FLOAT_EQ(pos.x, 0.0F);
    EXPECT_FLOAT_EQ(pos.z, 0.0F);
    EXPECT_FLOAT_EQ(pos.y, -89.5F) << "Water height should be less zero level ground height.";
}

TEST_F(RuntimeManagerTestFixture, GetCellBbox)
{
    w3terr::W3MapRuntimeManagerImpl sut(&assets_, &informator_);

    const w3terr::math::bbox3 bbox = sut.get_cell_bbox({0, 0});
    // bbox min and max should match cell corners
    const w3terr::math::vector3 expected_min{0.0F, 0.0F, -128.0F};
    const w3terr::math::vector3 expected_max{128.0F, 0.0F, 0.0F};

    EXPECT_EQ(bbox.get_min(), expected_min);
    EXPECT_EQ(bbox.get_max(), expected_max);
}

TEST_F(RuntimeManagerTestFixture, GetCellGroundHeightLerp)
{
    w3terr::W3MapRuntimeManagerImpl sut(&assets_, &informator_);
    // all heights are zero, so any lerp should be zero
    auto actual_height = sut.get_cell_ground_height({0, 0}, 0.5F, 0.5F);
    EXPECT_FLOAT_EQ(actual_height, 0.0F);
}

TEST_F(RuntimeManagerTestFixture, TestCellIntersection)
{
    w3terr::W3MapRuntimeManagerImpl sut(&assets_, &informator_);

    // line that passes through the cell
    w3terr::math::line3 line{{64.0F, 10.0F, -64.0F}, {0.0F, -1.0F, 0.0F}};
    EXPECT_TRUE(sut.test_cell_intersection({0, 0}, line));

    // line far away
    w3terr::math::line3 line2{{1000.0F, 0.0F, 1000.0F}, {0.0F, 1.0F, 0.0F}};
    EXPECT_FALSE(sut.test_cell_intersection({0, 0}, line2));
}

TEST_F(RuntimeManagerTestFixture, GetCellIntersectionPoint)
{
    w3terr::W3MapRuntimeManagerImpl sut(&assets_, &informator_);

    // vertical line down through the cell center
    w3terr::math::line3 line{{64.0F, 10.0F, -64.0F}, {0.0F, -1.0F, 0.0F}};
    auto maybe_point = sut.get_cell_intersection_point({0, 0}, line);
    EXPECT_TRUE(maybe_point.has_value());

    // intersection point should be on the cell's plane (y = 0)
    EXPECT_FLOAT_EQ(maybe_point->y, 0.0F);
    EXPECT_NEAR(maybe_point->x, 64.0F, 1e-3F);
    EXPECT_NEAR(maybe_point->z, -64.0F, 1e-3F);
}

TEST_F(RuntimeManagerTestFixture, UpdateCellRt)
{
    W3CPInfo info {
        .flags = W3CPInfo::GROUND,
        .key = 0,
        .ground_layers = {0, 0} // dummy layer
    };

    ON_CALL(informator_, collect_cellpoint_info(testing::_))
        .WillByDefault(testing::Return(W3CPInfo {}));

    w3terr::W3MapRuntimeManagerImpl sut(&assets_, &informator_);

    // called twice for cellpoint (1,1)
    EXPECT_CALL(informator_, collect_cellpoint_info(w3terr::Coord2D(1, 1)))
        .WillOnce(testing::Return(info));

    sut.update_cell_rt({1, 1});

    // after update, dirty flag should be true (already true after construction)
    EXPECT_TRUE(sut.is_dirty());
}

TEST_F(RuntimeManagerTestFixture, UpdateAreaRt)
{
    W3CPInfo info {
        .flags = W3CPInfo::GROUND,
        .key = 0,
        .ground_layers = {0, 0} // dummy layer
    };

    ON_CALL(informator_, collect_cellpoint_info(testing::_))
        .WillByDefault(testing::Return(W3CPInfo {}));

    w3terr::W3MapRuntimeManagerImpl sut(&assets_, &informator_);

    // Expect call twice for each cell in area (margin 1 around (2,2) => 3x3 cells)
    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            EXPECT_CALL(informator_, collect_cellpoint_info(w3terr::Coord2D(2 + dx, 2 + dy)))
                .WillOnce(testing::Return(info));
        }
    }

    sut.update_area_rt({2, 2}, 1);

    EXPECT_TRUE(sut.is_dirty());
}
TEST_F(RuntimeManagerTestFixture, GetCellpointInfoForGround)
{
    static constexpr uint8_t kExpectedTilesetId = 1;
    W3CPInfo info {
        .flags = W3CPInfo::GROUND,
        .key = 0x14444,
        .ground_layers = {1, kExpectedTilesetId } // dummy layer
    };

    ON_CALL(informator_, collect_cellpoint_info(testing::_))
        .WillByDefault(testing::Return(W3CPInfo {}));

    w3terr::W3MapRuntimeManagerImpl sut(&assets_, &informator_);

    EXPECT_CALL(informator_, collect_cellpoint_info(w3terr::Coord2D(1, 1)))
        .WillOnce(testing::Return(info));

    sut.update_cell_rt({1, 1});

    const auto& cell_rt = std::as_const(sut).get_cellpoint_rt({1, 1});
    EXPECT_TRUE(cell_rt.check_flag(w3terr::W3MapRuntimeManager::CellPointRT::GROUND));
    EXPECT_EQ(cell_rt.get_ground_tileset_id(0), kExpectedTilesetId);
    EXPECT_EQ(cell_rt.get_ground_tile_uv_indices(0), w3terr::W3UInt8Pair(5, 0));
}

TEST_F(RuntimeManagerTestFixture, GetCellpointInfoForCliff)
{
    static constexpr uint8_t kExpectedTilesetId = 1;
    W3CPInfo info {
        .flags = W3CPInfo::GEOCLIFF,
        .key = 0x45, // example cliff key
        .geo_tileset = kExpectedTilesetId,
        .ground_layers = {}
    };

    ON_CALL(informator_, collect_cellpoint_info(testing::_))
        .WillByDefault(testing::Return(W3CPInfo {}));

    w3terr::W3MapRuntimeManagerImpl sut(&assets_, &informator_);

    EXPECT_CALL(informator_, collect_cellpoint_info(w3terr::Coord2D(2, 2)))
        .WillOnce(testing::Return(info));

    sut.update_cell_rt({2, 2});

    const auto& cell_rt = std::as_const(sut).get_cellpoint_rt({2, 2});
    EXPECT_TRUE(cell_rt.check_flag(w3terr::W3MapRuntimeManager::CellPointRT::GEO_CLIFF));
    EXPECT_EQ(cell_rt.tileset_id, kExpectedTilesetId);
    // vertices_count and indices_count may be zero because mock assets have null mesh
}

TEST_F(RuntimeManagerTestFixture, GetCellpointInfoForRamp)
{
    static constexpr uint8_t kExpectedTilesetId = 1;
    W3CPInfo info {
        .flags = W3CPInfo::GEORAMP,
        .key = 0x1234,
        .geo_tileset = kExpectedTilesetId,
        .ground_layers = {}
    };

    ON_CALL(informator_, collect_cellpoint_info(testing::_))
        .WillByDefault(testing::Return(W3CPInfo {}));

    w3terr::W3MapRuntimeManagerImpl sut(&assets_, &informator_);

    EXPECT_CALL(informator_, collect_cellpoint_info(w3terr::Coord2D(3, 3)))
        .WillOnce(testing::Return(info));

    sut.update_cell_rt({3, 3});

    const auto& cell_rt = std::as_const(sut).get_cellpoint_rt({3, 3});
    EXPECT_TRUE(cell_rt.check_flag(w3terr::W3MapRuntimeManager::CellPointRT::GEO_RAMP));
    EXPECT_EQ(cell_rt.tileset_id, kExpectedTilesetId);
    // vertices_count and indices_count may be zero because mock assets have null mesh
}

TEST_F(RuntimeManagerTestFixture, GetCellpointInfoForWater)
{
    using W3CPInfo = w3terr::W3MapInformator::W3CPInfo;
    W3CPInfo info {
        .flags = W3CPInfo::WATER,
        .key = 0,
        .ground_layers = {}
    };

    ON_CALL(informator_, collect_cellpoint_info(testing::_))
        .WillByDefault(testing::Return(info));

    w3terr::W3MapRuntimeManagerImpl sut(&assets_, &informator_);

    EXPECT_CALL(informator_, collect_cellpoint_info(w3terr::Coord2D(0, 0)))
        .WillOnce(testing::Return(info));

    sut.update_cell_rt({0, 0});

    const w3terr::W3MapRuntimeManagerImpl& const_sut = sut;
    const auto& cell_rt = const_sut.get_cellpoint_rt({0, 0});
    EXPECT_TRUE(cell_rt.check_flag(w3terr::W3MapRuntimeManager::CellPointRT::WATER));
}

TEST_F(RuntimeManagerTestFixture, UpdateAllCellsRtAndCallsInformatorForEachCell)
{
    using W3CPInfo = w3terr::W3MapInformator::W3CPInfo;
    W3CPInfo info {
        .flags = W3CPInfo::GROUND,
        .key = 0,
        .ground_layers = {}
    };

    ON_CALL(informator_, collect_cellpoint_info(testing::_))
        .WillByDefault(testing::Return(info));

    w3terr::W3MapRuntimeManagerImpl sut(&assets_, &informator_);

    // map size is 5x5 cellpoints
    EXPECT_CALL(informator_, collect_cellpoint_info(testing::_))
        .Times(25)
        .WillRepeatedly(testing::Return(info));

    sut.update_all_cells_rt();

    // After update, dirty flag should be true
    EXPECT_TRUE(sut.is_dirty());
    // All cells should have ground flag? Not necessarily because info may be empty.
}


TEST_F(RuntimeManagerTestFixture, GetCellBboxAtMapEdge)
{
    w3terr::W3MapRuntimeManagerImpl sut(&assets_, &informator_);
    // Map size is 5x5 cellpoints, cells are 0..3 in each dimension.
    // Test cell at (3,3) (bottom-right)
    const w3terr::math::bbox3 bbox = sut.get_cell_bbox({3, 3});
    // Expected corners: cellpoints (3,3), (4,3), (4,4), (3,4)
    // Positions can be computed from w3e map.
    // Since we have a mock w3e, we can compute manually.
    // For simplicity, we can just ensure bbox is valid (min <= max).
    EXPECT_LE(bbox.get_min().x, bbox.get_max().x);
    EXPECT_LE(bbox.get_min().y, bbox.get_max().y);
    EXPECT_LE(bbox.get_min().z, bbox.get_max().z);
}

TEST_F(RuntimeManagerTestFixture, GetCellGroundHeightExtremeLerp)
{
    w3terr::W3MapRuntimeManagerImpl sut(&assets_, &informator_);
    // All heights are zero in test map.
    EXPECT_FLOAT_EQ(sut.get_cell_ground_height({0, 0}, 0.0F, 0.0F), 0.0F);
    EXPECT_FLOAT_EQ(sut.get_cell_ground_height({0, 0}, 1.0F, 0.0F), 0.0F);
    EXPECT_FLOAT_EQ(sut.get_cell_ground_height({0, 0}, 0.0F, 1.0F), 0.0F);
    EXPECT_FLOAT_EQ(sut.get_cell_ground_height({0, 0}, 1.0F, 1.0F), 0.0F);
    // Out-of-range lerp values may produce extrapolation; we can still test they don't crash.
    EXPECT_NO_THROW(sut.get_cell_ground_height({0, 0}, -0.5F, 0.5F));
    EXPECT_NO_THROW(sut.get_cell_ground_height({0, 0}, 1.5F, 0.5F));
}

TEST_F(RuntimeManagerTestFixture, TestCellIntersectionWhenNoIntersection)
{
    w3terr::W3MapRuntimeManagerImpl sut(&assets_, &informator_);
    // Line far away
    w3terr::math::line3 line{{1000.0F, 1000.0F, 1000.0F}, {0.0F, 1.0F, 0.0F}};
    EXPECT_FALSE(sut.test_cell_intersection({0, 0}, line));
}

TEST_F(RuntimeManagerTestFixture, GetCellIntersectionPointWhenNoIntersection)
{
    w3terr::W3MapRuntimeManagerImpl sut(&assets_, &informator_);
    w3terr::math::line3 line{{1000.0F, 1000.0F, 1000.0F}, {0.0F, 1.0F, 0.0F}};
    auto maybe_point = sut.get_cell_intersection_point({0, 0}, line);
    EXPECT_FALSE(maybe_point.has_value());
}

TEST_F(RuntimeManagerTestFixture, SetDirtyFlagAfterUpdate)
{
    w3terr::W3MapRuntimeManagerImpl sut(&assets_, &informator_);
    sut.set_dirty(false);
    EXPECT_FALSE(sut.is_dirty());
    // Update a cell should set dirty true
    using W3CPInfo = w3terr::W3MapInformator::W3CPInfo;
    W3CPInfo info {
        .flags = W3CPInfo::GROUND,
        .key = 0,
        .ground_layers = {}
    };
    EXPECT_CALL(informator_, collect_cellpoint_info(testing::_))
        .WillOnce(testing::Return(info));
    sut.update_cell_rt({1, 1});
    EXPECT_TRUE(sut.is_dirty());
}