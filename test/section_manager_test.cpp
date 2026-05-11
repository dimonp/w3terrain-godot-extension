#include "gtest/gtest.h"
#include <gmock/gmock.h>

#include "w3terrain/w3mapsection.h"
#include "w3terrain/w3mapassets.h"
#include "w3terrain/w3mapruntimemanager.h"
#include "w3terrain/w3mapsectionmanager_impl.h"
#include "mock_uitils.h"

namespace {

class MockRuntimeManager : public w3terr::W3MapRuntimeManager {
public:
    MOCK_METHOD(const CellPointRT&, get_cellpoint_rt, (const w3terr::Coord2D&), (const, override));
    MOCK_METHOD(float, get_cellpoint_layer_height, (const w3terr::Coord2D&), (const, override));
    MOCK_METHOD(float, get_cellpoint_ground_height, (const w3terr::Coord2D&), (const, override));
    MOCK_METHOD(w3terr::math::vector3, get_cellpoint_position, (const w3terr::Coord2D&), (const, override));
    MOCK_METHOD(w3terr::math::vector3, get_cellpoint_water_position, (const w3terr::Coord2D&), (const, override));
    MOCK_METHOD(w3terr::math::bbox3, get_cell_bbox, (const w3terr::Coord2D&), (const, override));
    MOCK_METHOD(float, get_cell_ground_height, (const w3terr::Coord2D&, float, float), (const, override));
    MOCK_METHOD(bool, test_cell_intersection, (const w3terr::Coord2D&, const w3terr::math::line3&), (const, override));
    MOCK_METHOD(std::optional<w3terr::math::vector3>, get_cell_intersection_point, (const w3terr::Coord2D&, const w3terr::math::line3&), (const, override));

    MOCK_METHOD(void, update_cell_rt, (const w3terr::Coord2D&), (override));
    MOCK_METHOD(void, update_area_rt, (const w3terr::Coord2D&, int32_t), (override));
    MOCK_METHOD(bool, is_dirty, (), (const, override));
    MOCK_METHOD(void, set_dirty, (bool), (override));
};

class MockAssets : public w3terr::W3MapAssets {
public:
    MOCK_METHOD(w3terr::W3e*, get_w3e, (), (const, override));
};

class SectionManagerTestFixture : public ::testing::Test {
protected:
    w3terr::W3e w3e_5x5_;
    w3terr::W3e w3e_9x9_;
    ::testing::NiceMock<MockAssets> assets_;
    MockRuntimeManager runtime_manager_;

    void SetUp() override
    {
        w3e_5x5_ = w3terr::test::create_test_w3e_5x5(2, 2);
        w3e_9x9_ = w3terr::test::create_test_w3e_9x9();

        ON_CALL(assets_, get_w3e()).WillByDefault(testing::Return(&w3e_5x5_));
    }

    void TearDown() override {}
};

}  // namespace


TEST_F(SectionManagerTestFixture, InitializeWithValidParameters)
{
    w3terr::W3MapSectionManagerImpl sut(&assets_, &runtime_manager_);


    EXPECT_EQ(sut.begin(), w3terr::W3MapSectionManager::SectionIdIterator(1));
    EXPECT_EQ(sut.end(), w3terr::W3MapSectionManager::SectionIdIterator(2));

    EXPECT_TRUE(sut.is_valid_section_id(1));
    EXPECT_FALSE(sut.is_valid_section_id(0));
    EXPECT_FALSE(sut.is_valid_section_id(10));

    const auto& section = sut.get_section_by_id(1);
    // EXPECT_EQ(section.get_geo_tilesets_size(), 2);
    // EXPECT_EQ(section.get_ground_tilesets_size(), 2);
}

TEST_F(SectionManagerTestFixture, SetDirtyAllMarksAllSectionsDirty)
{
    w3terr::W3MapSectionManagerImpl sut(&assets_, &runtime_manager_);
    // All sections are dirty by default after initialization
    for (const auto section_id : sut) {
        EXPECT_TRUE(sut.get_section_by_id(section_id).is_dirty());
    }
    // Call set_dirty_all (should keep them dirty)
    sut.set_dirty_all();
    for (const auto section_id : sut) {
        EXPECT_TRUE(sut.get_section_by_id(section_id).is_dirty());
    }
}

TEST_F(SectionManagerTestFixture, InvalidateSectionsAtCellpointCallsRuntimeUpdateArea)
{
    w3terr::W3MapSectionManagerImpl sut(&assets_, &runtime_manager_);
    const w3terr::Coord2D coords{2, 2};
    EXPECT_CALL(runtime_manager_, update_area_rt(coords, 2)).Times(1);
    sut.invalidate_sections_at_cellpoint(coords);
}

TEST_F(SectionManagerTestFixture, MultipleSectionsIteration)
{
    // Use a larger map to have multiple sections
    w3terr::W3e w3e_large = w3terr::test::create_test_w3e_9x9();
    ON_CALL(assets_, get_w3e()).WillByDefault(testing::Return(&w3e_large));

    w3terr::W3MapSectionManagerImpl sut(&assets_, &runtime_manager_);

    // 9x9 map with section dimension 4 => 2x2 sections
    EXPECT_EQ(sut.begin(), w3terr::W3MapSectionManager::SectionIdIterator(1));
    EXPECT_EQ(sut.end(), w3terr::W3MapSectionManager::SectionIdIterator(5)); // 4 sections
}

TEST_F(SectionManagerTestFixture, UpdateAllSectionsReinitializesSections)
{
    w3terr::W3MapSectionManagerImpl sut(&assets_, &runtime_manager_);
    // Initially 1 section (5x5 map)
    EXPECT_EQ(sut.begin(), w3terr::W3MapSectionManager::SectionIdIterator(1));
    EXPECT_EQ(sut.end(), w3terr::W3MapSectionManager::SectionIdIterator(2));

    // Switch to a larger map
    ON_CALL(assets_, get_w3e()).WillByDefault(testing::Return(&w3e_9x9_));
    sut.initialize_all_sections();

    // Now should have 4 sections
    EXPECT_EQ(sut.begin(), w3terr::W3MapSectionManager::SectionIdIterator(1));
    EXPECT_EQ(sut.end(), w3terr::W3MapSectionManager::SectionIdIterator(5));
}