/**
 * @File Check.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/11/25
 * @Brief This file is part of Bee.
 */

#pragma once

#include <Bee/Bee.hpp>
#include <cuda_runtime.h>

using namespace bee;

struct TestStatus
{
    int failureCount;
};

__device__ inline void NotifyFailure(TestStatus* status, const char* file, int line, const char* msg)
{
    if (status)
    {
        atomicAdd(&status->failureCount, 1);
    }
    printf("-> [FAILURE] %s:%d: %s\n", file, line, msg);
}

#define BEE_TEST_START                                                              \
    TestStatus h_status = {0};                                                      \
    TestStatus* d_status;                                                           \
    cudaMalloc(&d_status, sizeof(TestStatus));                                      \
    cudaMemcpy(d_status, &h_status, sizeof(TestStatus), cudaMemcpyHostToDevice)

#define BEE_TEST_END                                                                \
    cudaMemcpy(&h_status, d_status, sizeof(TestStatus), cudaMemcpyDeviceToHost);    \
    if (h_status.failureCount > 0) {                                                \
        printf("[%s] with %d errors.\n", __FUNCTION__ , h_status.failureCount);     \
    } else {                                                                        \
        printf("[%s] PASSED.\n", __FUNCTION__ );                                    \
    }                                                                               \
    cudaFree(d_status)

#define BEE_DEFINE_TEST(testFuncName)                       \
    void testFuncName() {                                   \
        BEE_TEST_START;                                     \
        BEE_CONCAT(testFuncName, Kernal)<<<1,1>>>(d_status);\
        cudaDeviceSynchronize();                            \
        BEE_TEST_END;                                       \
    }

#define BEE_TEST_KERNAL(kernalName) __global__ void kernalName(TestStatus* testStatus)

// -----------------------------------------------------------
// 核心比较宏
// -----------------------------------------------------------
#define BEE_TEST_OP(a, b, op, op_str, is_assert)                                                            \
    do {                                                                                                    \
        auto val_a = (a);                                                                                   \
        auto val_b = (b);                                                                                   \
        if (BEE_UNLIKELY(!(val_a op val_b))) {                                                              \
            NotifyFailure(testStatus, __FILE__, __LINE__, "Check failed: " #a " " op_str " " #b);           \
            printf("    Expected: "); bee::detail::DevicePrintVal(val_a);                                   \
            printf("\n    Actual:   "); bee::detail::DevicePrintVal(val_b); printf("\n");                   \
            if (is_assert) {                                                                                \
                return;                                                                                     \
            }                                                                                               \
        }                                                                                                   \
    } while(0)

#define EXPECT_EQ(a, b) BEE_TEST_OP(a, b, ==, "==", false)
#define EXPECT_NE(a, b) BEE_TEST_OP(a, b, !=, "!=", false)
#define EXPECT_GT(a, b) BEE_TEST_OP(a, b, > , ">" , false)
#define EXPECT_GE(a, b) BEE_TEST_OP(a, b, >=, ">=", false)
#define EXPECT_LT(a, b) BEE_TEST_OP(a, b, < , "<" , false)
#define EXPECT_LE(a, b) BEE_TEST_OP(a, b, <=, "<=", false)

#define ASSERT_EQ(a, b) BEE_TEST_OP(a, b, ==, "==", true)
#define ASSERT_NE(a, b) BEE_TEST_OP(a, b, !=, "!=", true)
#define ASSERT_GT(a, b) BEE_TEST_OP(a, b, > , ">" , true)
#define ASSERT_GE(a, b) BEE_TEST_OP(a, b, >=, ">=", true)
#define ASSERT_LT(a, b) BEE_TEST_OP(a, b, < , "<" , true)
#define ASSERT_LE(a, b) BEE_TEST_OP(a, b, <=, "<=", true)

#define EXPECT_TRUE(x)                                                              \
    do {                                                                            \
        if (BEE_UNLIKELY(!(x))) {                                                   \
            NotifyFailure(testStatus, __FILE__, __LINE__, "EXPECT_TRUE failed");    \
            printf("    Result FALSE: %s\n", #x);                                   \
        }                                                                           \
    } while(0)

#define EXPECT_FALSE(x)                                                             \
    do {                                                                            \
        if (BEE_UNLIKELY((x))) {                                                    \
            NotifyFailure(testStatus, __FILE__, __LINE__, "EXPECT_FALSE failed");   \
            printf("    Result TRUE: %s\n", #x);                                    \
        }                                                                           \
    } while(0)

// -----------------------------------------------------------
// 浮点近似比较
// -----------------------------------------------------------
#define EXPECT_NEAR(a, b, tol)                                                          \
    do {                                                                                \
        double va = (double)(a); double vb = (double)(b); double t = (double)(tol);     \
        double diff = fabs(va - vb);                                                    \
        if (BEE_UNLIKELY(diff > t)) {                                                   \
            NotifyFailure(testStatus, __FILE__, __LINE__, "EXPECT_NEAR failed");        \
            printf("    Diff: %f > Tol: %f\n", diff, t);                                \
        }                                                                               \
    } while(0)

#define EXPECT_FLOAT_EQ(a, b) EXPECT_NEAR(a, b, 1e-6)
#define EXPECT_DOUBLE_EQ(a, b) EXPECT_NEAR(a, b, 1e-12)
