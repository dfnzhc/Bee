#include <gtest/gtest.h>

#include "DataStructure/DataStructure.hpp"

TEST(DataStructureComponentTests, ReturnsComponentName)
{
    EXPECT_EQ(bee::data_structure_name(), "DataStructure");
}

TEST(DataStructureComponentTests, LinksAndUsesBaseComponent)
{
    EXPECT_EQ(bee::data_structure_base_name(), "Base");
}
