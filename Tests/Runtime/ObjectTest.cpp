/**
 * @File ObjectTest.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2024/11/22
 * @Brief This file is part of Bee.
 */

#include <gtest/gtest.h>
#include <Core/Object.hpp>
#include "Utility/Assert.hpp"

using namespace bee;

class ObjectTest : public ::testing::Test
{
protected:
    Object obj;
};

TEST_F(ObjectTest, DefaultRefCount)
{
    EXPECT_EQ(obj.refCount(), 0);
}

TEST_F(ObjectTest, IncRef)
{
    obj.incRef();
    EXPECT_EQ(obj.refCount(), 1);
}

TEST_F(ObjectTest, DecRef)
{
    obj.incRef();
    obj.decRef(false);
    EXPECT_EQ(obj.refCount(), 0);
}

class ObjectRefTest : public ::testing::Test
{
protected:

    Object* raw_obj = new Object();
    ref<Object> ref_obj = raw_obj;
};

TEST_F(ObjectRefTest, DefaultCtor)
{
    ref<Object> empty;
    EXPECT_EQ(empty.get(), nullptr);
}

TEST_F(ObjectRefTest, PtrCtor)
{
    EXPECT_EQ(ref_obj.get(), raw_obj);
    EXPECT_EQ(raw_obj->refCount(), 1);
}

TEST_F(ObjectRefTest, CopyCtor)
{
    ref<Object> copy(ref_obj);
    EXPECT_EQ(copy.get(), raw_obj);
    EXPECT_EQ(raw_obj->refCount(), 2);
}

TEST_F(ObjectRefTest, MoveCtor)
{
    ref<Object> moved(std::move(ref_obj));
    EXPECT_EQ(moved.get(), raw_obj);
    EXPECT_EQ(ref_obj.get(), nullptr);
    EXPECT_EQ(raw_obj->refCount(), 1);
}

TEST_F(ObjectRefTest, CopyAssignment)
{
    ref<Object> copy;
    copy = ref_obj;
    EXPECT_EQ(copy.get(), raw_obj);
    EXPECT_EQ(raw_obj->refCount(), 2);
}

TEST_F(ObjectRefTest, MoveAssignment)
{
    ref<Object> moved;
    moved = std::move(ref_obj);
    EXPECT_EQ(moved.get(), raw_obj);
    EXPECT_EQ(ref_obj.get(), nullptr);
    EXPECT_EQ(raw_obj->refCount(), 1);
}

TEST_F(ObjectRefTest, Reset)
{
    auto* new_obj = new Object();
    ref<Object> new_ref(new_obj);
    ref_obj.reset(new_obj);
    EXPECT_EQ(ref_obj.get(), new_ref.get());
    EXPECT_EQ(new_obj->refCount(), 2);
}

TEST_F(ObjectRefTest, Equality)
{
    ref<Object> ref1 = raw_obj;
    ref<Object> ref2 = raw_obj;
    EXPECT_TRUE(ref1 == ref2);
}

TEST_F(ObjectRefTest, Inequality)
{
    auto* another_obj = new Object();
    ref<Object> ref1    = raw_obj;
    ref<Object> ref2    = another_obj;
    EXPECT_TRUE(ref1 != ref2);
}

TEST_F(ObjectRefTest, Dereference)
{
    EXPECT_EQ(&(*ref_obj), raw_obj);
}

TEST_F(ObjectRefTest, ArrowOperator)
{
    EXPECT_EQ(ref_obj->className(), "Object");
}

TEST_F(ObjectRefTest, BoolConversion)
{
    ref<Object> empty;
    EXPECT_FALSE(empty);
    ref<Object> non_empty = raw_obj;
    EXPECT_TRUE(non_empty);
}

TEST_F(ObjectRefTest, Swap)
{
    auto* another_obj = new Object();
    ref<Object> ref1    = raw_obj;
    ref<Object> ref2    = another_obj;
    ref1.swap(ref2);
    EXPECT_EQ(ref1.get(), another_obj);
    EXPECT_EQ(ref2.get(), raw_obj);
    
    Object::dumpAliveObjects();
}

TEST_F(ObjectRefTest, Destructor)
{
    auto* temp_obj = new Object();
    EXPECT_EQ(temp_obj->refCount(), 0);
    {
        ref<Object> temp_ref = temp_obj;
        EXPECT_EQ(temp_obj->refCount(), 1);
    }
    EXPECT_EQ(temp_obj->refCount(), 0);
}