#include <gtest/gtest.h>
#include <florid/protocols/version.hpp>

using florid::protocol::Version;
using florid::protocol::versions_compatible;
using florid::protocol::CURRENT_PROTOCOL_VERSION;

TEST(Version, EncodeDecode)
{
    Version v{1, 2, 3};
    EXPECT_EQ(v.major(), 1);
    EXPECT_EQ(v.minor(), 2);
    EXPECT_EQ(v.patch(), 3);
    EXPECT_EQ(v.raw, 0x1083);
}

TEST(Version, RawConstruction)
{
    Version v{0x1083};
    EXPECT_EQ(v.major(), 1);
    EXPECT_EQ(v.minor(), 2);
    EXPECT_EQ(v.patch(), 3);
}

TEST(Version, MaxValues)
{
    Version v{15, 63, 63};
    EXPECT_EQ(v.raw, 0xFFFF);
    EXPECT_EQ(v.major(), 15);
    EXPECT_EQ(v.minor(), 63);
    EXPECT_EQ(v.patch(), 63);
}

TEST(Version, Truncation)
{
    // 超过位宽的分量应被掩码截断
    Version v{16, 64, 64};
    EXPECT_EQ(v.major(), 0);
    EXPECT_EQ(v.minor(), 0);
    EXPECT_EQ(v.patch(), 0);
    EXPECT_EQ(v.raw, 0);
}

TEST(Version, CompatibleSameMajor)
{
    Version va{2, 0, 0};
    Version vb{2, 5, 9};
    EXPECT_TRUE(va.compatible_with(vb));
    EXPECT_TRUE(versions_compatible(va, vb));
    EXPECT_TRUE(versions_compatible(va.raw, vb.raw));
}

TEST(Version, IncompatibleDifferentMajor)
{
    Version va{2, 0, 0};
    Version vc{3, 0, 0};
    EXPECT_FALSE(va.compatible_with(vc));
    EXPECT_FALSE(versions_compatible(va, vc));
    EXPECT_FALSE(versions_compatible(va.raw, vc.raw));
}

TEST(Version, CurrentVersion)
{
    EXPECT_EQ(CURRENT_PROTOCOL_VERSION.major(), 1);
    EXPECT_EQ(CURRENT_PROTOCOL_VERSION.minor(), 0);
    EXPECT_EQ(CURRENT_PROTOCOL_VERSION.patch(), 0);
}

TEST(Version, Equality)
{
    Version v1{1, 2, 3};
    Version v2{1, 2, 3};
    Version v3{1, 2, 4};
    EXPECT_EQ(v1, v2);
    EXPECT_NE(v1, v3);
}
