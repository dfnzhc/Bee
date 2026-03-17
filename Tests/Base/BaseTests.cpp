#include <gtest/gtest.h>

#include "Base/Base.hpp"

TEST(BaseComponentTests, ReturnsComponentName) {
    EXPECT_EQ(bee::base::name(), "Base");
}
