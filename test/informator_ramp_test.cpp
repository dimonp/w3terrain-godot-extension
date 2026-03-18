#include "gtest/gtest.h"
#include <gmock/gmock.h>

#include "w3terrain/w3mapassets.h"
#include "w3terrain/w3mapinformator_impl.h"
#include "mock_uitils.h"

class MockAssets : public w3terr::W3MapAssets {
public:
    MockAssets() {
        ground_assets_rt_.resize(2);
        geo_assets_rt_.resize(2);
    }
    MOCK_METHOD(w3terr::W3e*, get_w3e, (), (const, override));
};

enum RC : uint8_t  {
    G,  // ground
    R,  // ramp
    MR, // middle ramp + ramp
    MG  // middle ramp + ground
};

inline constexpr std::array<std::array<RC, 4>, 4> kRampCheckPattern = {{
    {RC::G, RC::G, RC::G, RC::G},
    {RC::R, RC::G, RC::G, RC::R},
    {RC::R, RC::MG, RC::MG, RC::MR},
    {RC::G, RC::G, RC::G, RC::G},
}};

using W3CPInfo = w3terr::W3MapInformator::W3CPInfo;

static void
check_ramp_flags(const W3CPInfo& info, const RC check, const w3terr::Coord2D& coord) // NOLINT(readability-function-cognitive-complexity)
{
    switch (check) { // NOLINT
    case RC::G:
        EXPECT_TRUE(info.check_flag(W3CPInfo::GROUND)) << "Cell (" << coord.x << "," << coord.y << ") should be ground";
        EXPECT_FALSE(info.check_flag(W3CPInfo::GEORAMP));
        EXPECT_FALSE(info.check_flag(W3CPInfo::GEOCLIFF));
        EXPECT_FALSE(info.check_flag(W3CPInfo::RAMP_MIDDLE));
        break;
    case RC::R:
        EXPECT_TRUE(info.check_flag(W3CPInfo::GEORAMP)) << "Cell (" << coord.x << "," << coord.y << ") should be ramp";
        EXPECT_FALSE(info.check_flag(W3CPInfo::RAMP_MIDDLE));
        EXPECT_FALSE(info.check_flag(W3CPInfo::GEOCLIFF));
        EXPECT_FALSE(info.check_flag(W3CPInfo::GROUND));
        break;
    case RC::MG:
        EXPECT_TRUE(info.check_flag(W3CPInfo::RAMP_MIDDLE)) << "Cell (" << coord.x << "," << coord.y << ") should be ramp middle";
        EXPECT_TRUE(info.check_flag(W3CPInfo::GROUND));
        EXPECT_FALSE(info.check_flag(W3CPInfo::GEORAMP));
        EXPECT_FALSE(info.check_flag(W3CPInfo::GEOCLIFF));
        break;
    case RC::MR:
        EXPECT_TRUE(info.check_flag(W3CPInfo::RAMP_MIDDLE)) << "Cell (" << coord.x << "," << coord.y << ") should be ramp middle";
        EXPECT_TRUE(info.check_flag(W3CPInfo::GEORAMP));
        EXPECT_FALSE(info.check_flag(W3CPInfo::GROUND));
        EXPECT_FALSE(info.check_flag(W3CPInfo::GEOCLIFF));
        break;
    default:
        break;
    }
}


class InformatorRampTestFixture : public ::testing::Test {
protected:
    static constexpr int kCliffCellX = 3;
    static constexpr int kCliffCellY = 3;

    w3terr::W3e w3e_9x9_ramp_v_;
    w3terr::W3e w3e_9x9_ramp_h_;
    ::testing::NiceMock<MockAssets> assets_;

    void SetUp() override
    {
        w3e_9x9_ramp_v_ = w3terr::test::create_test_w3e_9x9_ramp_v(kCliffCellY);
        w3e_9x9_ramp_h_ = w3terr::test::create_test_w3e_9x9_ramp_h(kCliffCellX);
    }

    void TearDown() override {}
};

TEST_F(InformatorRampTestFixture, CheckVerticalRampLayout)
{
    ON_CALL(assets_, get_w3e()).WillByDefault(testing::Return(&w3e_9x9_ramp_v_));

    w3terr::W3MapInformatorImpl sut(&assets_);

    static constexpr int kStartPosX = 2;
    static constexpr int kStartPosY = 1;
    for (int py = 0; py < 4; ++py) {
        for (int px = 0; px < 4; ++px) {
            const auto coord = w3terr::Coord2D(px + kStartPosX, py + kStartPosY);
            const W3CPInfo info = sut.collect_cellpoint_info(coord);
            check_ramp_flags(info, kRampCheckPattern[py][px], coord); // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
        }
    }
}

TEST_F(InformatorRampTestFixture, CheckHorizontalRampLayout)
{
    ON_CALL(assets_, get_w3e()).WillByDefault(testing::Return(&w3e_9x9_ramp_h_));

    w3terr::W3MapInformatorImpl sut(&assets_);

    static constexpr int kStartPosX = 1;
    static constexpr int kStartPosY = 2;
    for (int py = 0; py < 4; ++py) {
        for (int px = 0; px < 4; ++px) {
            const auto coord = w3terr::Coord2D(px + kStartPosX, py + kStartPosY);
            const W3CPInfo info = sut.collect_cellpoint_info(coord);
            check_ramp_flags(info, kRampCheckPattern[px][py], coord); // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
        }
    }
}

TEST_F(InformatorRampTestFixture, CheckRampMiddleFlag)
{
    ON_CALL(assets_, get_w3e()).WillByDefault(testing::Return(&w3e_9x9_ramp_v_));

    w3terr::W3MapInformatorImpl sut(&assets_);

    // According to vertical ramp pattern, cell (3,3) should be middle ramp + ground
    const W3CPInfo info = sut.collect_cellpoint_info({3, 3});
    EXPECT_TRUE(info.check_flag(W3CPInfo::RAMP_MIDDLE));
    EXPECT_TRUE(info.check_flag(W3CPInfo::GROUND));
    EXPECT_FALSE(info.check_flag(W3CPInfo::GEORAMP));
    EXPECT_FALSE(info.check_flag(W3CPInfo::GEOCLIFF));
}
