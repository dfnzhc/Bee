#include <gtest/gtest.h>

#include "Tensor/Tensor.hpp"

TEST(TensorComponentTests, ReturnsComponentName)
{
    EXPECT_EQ(bee::tensor_name(), "Tensor");
}
