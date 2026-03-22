/**
 * @File NameofTests.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/3/17
 * @Brief This file is part of Bee.
 */

#include <gtest/gtest.h>

#include <array>
#include <string>
#include <string_view>
#include <type_traits>

#include "Base/Nameof.hpp"

namespace bee::nameof_test_samples
{

struct Vec3
{
    float x;
    float y;
    float z;
};

template <typename T>
struct Box
{
    T value;
};

struct MoveSpeed
{
    float value;
};

struct FancyName
{
    int value;
};

namespace nested
{

struct Inner
{
    int value;
};

} // namespace nested

enum class Color
{
    Red,
    Green,
    Blue
};

enum class Action
{
    Idle   = 0,
    Move   = 1,
    Attack = 2
};

enum class AutoAction
{
    Waiting = 0,
    Running = 1,
    Done    = 2
};

enum class AutoCheckedAction
{
    Stand = 0,
    Walk  = 1
};

enum class Permission
{
    None    = 0,
    Read    = 1,
    Write   = 2,
    Execute = 4
};

enum class AutoPermission
{
    None    = 0,
    Read    = 1,
    Write   = 2,
    Execute = 4
};

} // namespace bee::nameof_test_samples

namespace bee::Customize
{

template <>
struct DisplayName<bee::nameof_test_samples::MoveSpeed>
{
    static constexpr std::string_view value = "Move Speed";
};

template <>
struct TypeName<bee::nameof_test_samples::nested::Inner>
{
    static constexpr std::string_view value = "Gameplay::InnerNode";
};

template <>
struct TypeName<bee::nameof_test_samples::FancyName>
{
    static constexpr std::string_view value = "Fancy_Internal_Name";
};

template <>
struct DisplayName<bee::nameof_test_samples::FancyName>
{
    static constexpr std::string_view value = "Fancy Display";
};

template <>
struct EnumEntries<bee::nameof_test_samples::Action>
{
    static constexpr std::array<EnumEntry<bee::nameof_test_samples::Action>, 3> entries = {
        EnumEntry<bee::nameof_test_samples::Action>{bee::nameof_test_samples::Action::Idle, "idle", "Idle"},
        EnumEntry<bee::nameof_test_samples::Action>{bee::nameof_test_samples::Action::Move, "move", "Move Action"},
        EnumEntry<bee::nameof_test_samples::Action>{bee::nameof_test_samples::Action::Attack, "attack", ""}
    };
};

template <>
struct EnumScanRange<bee::nameof_test_samples::AutoCheckedAction>
{
    static constexpr int kMin = 0;
    static constexpr int kMax = 1;
    static constexpr bool kEnableAliasCheck = true;
};

template <>
struct EnumEntries<bee::nameof_test_samples::Permission>
{
    static constexpr std::array<EnumEntry<bee::nameof_test_samples::Permission>, 4> entries = {
        EnumEntry<bee::nameof_test_samples::Permission>{bee::nameof_test_samples::Permission::Read, "read", ""},
        EnumEntry<bee::nameof_test_samples::Permission>{bee::nameof_test_samples::Permission::Write, "write", ""},
        EnumEntry<bee::nameof_test_samples::Permission>{bee::nameof_test_samples::Permission::Execute, "execute", ""},
        EnumEntry<bee::nameof_test_samples::Permission>{bee::nameof_test_samples::Permission::None, "none", ""}
    };
};

template <>
struct EnumFlags<bee::nameof_test_samples::Permission>
{
    static constexpr bool kEnabled = true;
};

template <>
struct EnumScanRange<bee::nameof_test_samples::AutoPermission>
{
    static constexpr int kMin = 0;
    static constexpr int kMax = 4;
    static constexpr bool kEnableAliasCheck = false;
};

template <>
struct EnumFlags<bee::nameof_test_samples::AutoPermission>
{
    static constexpr bool kEnabled = true;
};

} // namespace bee::Customize

namespace
{

auto Contains(std::string_view full, std::string_view part) -> bool
{
    return full.find(part) != std::string_view::npos;
}

auto IsLowerSnakeCase(std::string_view text) -> bool
{
    if (text.empty()) {
        return false;
    }
    if (text.front() == '_' || text.back() == '_') {
        return false;
    }

    bool prev_underscore = false;
    for (char c : text) {
        const bool lower_or_digit = (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9');
        if (lower_or_digit) {
            prev_underscore = false;
            continue;
        }
        if (c == '_') {
            if (prev_underscore) {
                return false;
            }
            prev_underscore = true;
            continue;
        }
        return false;
    }
    return true;
}

template <typename E>
auto EnumOr(E left, E right) -> E
{
    using U = std::underlying_type_t<E>;
    return static_cast<E>(static_cast<U>(left) | static_cast<U>(right));
}

} // namespace

TEST(NameofTypeNameRawTests, ReturnsStringViewAtCompileTime)
{
    constexpr auto name = bee::type_name_raw<int>();
    static_assert(std::is_same_v<decltype(name), const std::string_view>);
    EXPECT_FALSE(name.empty());
}

TEST(NameofTypeNameRawTests, PreservesFundamentalTypeToken)
{
    const auto name = bee::type_name_raw<int>();
    EXPECT_TRUE(Contains(name, "int"));
}

TEST(NameofTypeNameRawTests, PreservesNamespaceForUserType)
{
    const auto name = bee::type_name_raw<bee::nameof_test_samples::Vec3>();
    EXPECT_TRUE(Contains(name, "Vec3"));
    EXPECT_TRUE(Contains(name, "nameof_test_samples"));
}

TEST(NameofTypeNameRawTests, PreservesTemplateStructure)
{
    const auto name = bee::type_name_raw<bee::nameof_test_samples::Box<int>>();
    EXPECT_TRUE(Contains(name, "Box"));
    EXPECT_TRUE(Contains(name, "int"));
}

TEST(NameofTypeNameRawTests, DistinguishesCvRefQualifiedTypes)
{
    const auto name = bee::type_name_raw<const bee::nameof_test_samples::Vec3&>();
    EXPECT_TRUE(Contains(name, "Vec3"));
    EXPECT_TRUE(Contains(name, "const"));
}

TEST(NameofTypeNameRawTests, SupportsEnumTypeName)
{
    const auto name = bee::type_name_raw<bee::nameof_test_samples::Color>();
    EXPECT_TRUE(Contains(name, "Color"));
}

TEST(NameofTypeNameRawTests, IsDeterministicForSameType)
{
    constexpr auto first  = bee::type_name_raw<bee::nameof_test_samples::Vec3>();
    constexpr auto second = bee::type_name_raw<bee::nameof_test_samples::Vec3>();
    EXPECT_EQ(first, second);
}

TEST(NameofTypeNameRawTests, DoesNotReturnUnsupportedSentinelForKnownCompiler)
{
    const auto name = bee::type_name_raw<int>();
    EXPECT_NE(name, "[Unsupported Compiler Format]");
}

TEST(NameofTypeNameShortTests, ReturnsStringViewAtCompileTime)
{
    constexpr auto name = bee::type_name_short<int>();
    static_assert(std::is_same_v<decltype(name), const std::string_view>);
    EXPECT_FALSE(name.empty());
}

TEST(NameofTypeNameShortTests, RemovesNamespaceForUserType)
{
    const auto name = bee::type_name_short<bee::nameof_test_samples::Vec3>();
    EXPECT_TRUE(Contains(name, "Vec3"));
    EXPECT_FALSE(Contains(name, "nameof_test_samples"));
}

TEST(NameofTypeNameShortTests, PreservesTemplateStructure)
{
    const auto name = bee::type_name_short<bee::nameof_test_samples::Box<int>>();
    EXPECT_TRUE(Contains(name, "Box"));
    EXPECT_TRUE(Contains(name, "int"));
}

TEST(NameofTypeNameShortTests, PreservesTemplateTypeWhenTemplateArgumentHasNamespace)
{
    const auto name = bee::type_name_short<bee::nameof_test_samples::Box<bee::nameof_test_samples::nested::Inner>>();
    EXPECT_TRUE(Contains(name, "Box<"));
    EXPECT_TRUE(Contains(name, "nested::Inner"));
}

TEST(NameofTypeNameShortTests, CollapsesCvRefToBaseTypeName)
{
    const auto name = bee::type_name_short<const bee::nameof_test_samples::Vec3&>();
    EXPECT_TRUE(Contains(name, "Vec3"));
    EXPECT_FALSE(Contains(name, "const"));
    EXPECT_FALSE(Contains(name, "nameof_test_samples"));
}

TEST(NameofTypeNameShortTests, IsDeterministicForSameType)
{
    constexpr auto first  = bee::type_name_short<bee::nameof_test_samples::Color>();
    constexpr auto second = bee::type_name_short<bee::nameof_test_samples::Color>();
    EXPECT_EQ(first, second);
}

TEST(NameofTypeNameShortTests, UsesCustomizeTypeNameOverride)
{
    EXPECT_EQ(bee::type_name_short<bee::nameof_test_samples::nested::Inner>(), "Gameplay::InnerNode");
}

TEST(NameofTypeNameShortTests, UsesCustomizeTypeNameOverrideForCvRef)
{
    EXPECT_EQ(bee::type_name_short<const bee::nameof_test_samples::nested::Inner&>(), "Gameplay::InnerNode");
}

TEST(NameofTypeNameKeyTests, ConvertsFundamentalTypeToStableKey)
{
    static_assert(std::is_same_v<decltype(bee::type_name_key<int>()), std::string_view>);
    EXPECT_EQ(bee::type_name_key<int>(), "int");
}

TEST(NameofTypeNameKeyTests, ConvertsPascalCaseToSnakeCase)
{
    EXPECT_EQ(bee::type_name_key<bee::nameof_test_samples::MoveSpeed>(), "move_speed");
}

TEST(NameofTypeNameKeyTests, RemovesNamespaceAndCvRef)
{
    EXPECT_EQ(bee::type_name_key<const bee::nameof_test_samples::Vec3&>(), "vec3");
}

TEST(NameofTypeNameKeyTests, ConvertsTemplateTypeToSnakeCaseKey)
{
    const auto key = bee::type_name_key<bee::nameof_test_samples::Box<int>>();
    EXPECT_TRUE(Contains(key, "box"));
    EXPECT_TRUE(Contains(key, "int"));
    EXPECT_TRUE(IsLowerSnakeCase(key));
}

TEST(NameofTypeNameKeyTests, IsDeterministicForSameType)
{
    const auto first  = bee::type_name_key<bee::nameof_test_samples::Color>();
    const auto second = bee::type_name_key<bee::nameof_test_samples::Color>();
    EXPECT_EQ(first, second);
}

TEST(NameofTypeNameKeyTests, UsesCustomizeTypeNameAsKeySource)
{
    EXPECT_EQ(bee::type_name_key<bee::nameof_test_samples::nested::Inner>(), "gameplay_inner_node");
}

TEST(NameofTypeNameDisplayTests, ReturnsStringViewAtCompileTime)
{
    constexpr auto name = bee::type_name_display<int>();
    static_assert(std::is_same_v<decltype(name), const std::string_view>);
    EXPECT_FALSE(name.empty());
}

TEST(NameofTypeNameDisplayTests, DefaultsToShortNameForUserType)
{
    const auto display_name = bee::type_name_display<bee::nameof_test_samples::Vec3>();
    const auto short_name   = bee::type_name_short<bee::nameof_test_samples::Vec3>();
    EXPECT_EQ(display_name, short_name);
    EXPECT_TRUE(Contains(display_name, "Vec3"));
}

TEST(NameofTypeNameDisplayTests, KeepsTemplateReadability)
{
    const auto display_name = bee::type_name_display<bee::nameof_test_samples::Box<int>>();
    EXPECT_TRUE(Contains(display_name, "Box"));
    EXPECT_TRUE(Contains(display_name, "int"));
}

TEST(NameofTypeNameDisplayTests, IsDeterministicForSameType)
{
    constexpr auto first  = bee::type_name_display<bee::nameof_test_samples::Color>();
    constexpr auto second = bee::type_name_display<bee::nameof_test_samples::Color>();
    EXPECT_EQ(first, second);
}

TEST(NameofTypeNameDisplayTests, UsesCustomizeDisplayNameOverride)
{
    EXPECT_EQ(bee::type_name_display<bee::nameof_test_samples::MoveSpeed>(), "Move Speed");
}

TEST(NameofTypeNameDisplayTests, KeepsShortNameWhenNoCustomizeOverride)
{
    const auto display_name = bee::type_name_display<bee::nameof_test_samples::Vec3>();
    const auto short_name   = bee::type_name_short<bee::nameof_test_samples::Vec3>();
    EXPECT_EQ(display_name, short_name);
}

TEST(NameofTypeNameDisplayTests, FallsBackToCustomizeTypeNameWhenNoDisplayName)
{
    EXPECT_EQ(bee::type_name_display<bee::nameof_test_samples::nested::Inner>(), "Gameplay::InnerNode");
}

TEST(NameofTypeNameDisplayTests, PrefersDisplayNameOverTypeName)
{
    EXPECT_EQ(bee::type_name_display<bee::nameof_test_samples::FancyName>(), "Fancy Display");
}

TEST(NameofValueNameShortTests, ReturnsStableEntryName)
{
    EXPECT_EQ(bee::value_name_short<bee::nameof_test_samples::Action::Idle>(), "idle");
}

TEST(NameofValueNameShortTests, ReturnsUnknownSentinelWhenValueNotRegistered)
{
    EXPECT_EQ(bee::value_name_short<static_cast<bee::nameof_test_samples::Action>(99)>(), "[Unknown Enum Value]");
}

TEST(NameofValueNameKeyTests, ConvertsStableNameToSnakeCase)
{
    EXPECT_EQ(bee::value_name_key<bee::nameof_test_samples::Action::Move>(), "move");
}

TEST(NameofValueNameDisplayTests, UsesDisplayTextWhenProvided)
{
    EXPECT_EQ(bee::value_name_display<bee::nameof_test_samples::Action::Move>(), "Move Action");
}

TEST(NameofValueNameDisplayTests, FallsBackToStableNameWhenDisplayTextMissing)
{
    EXPECT_EQ(bee::value_name_display<bee::nameof_test_samples::Action::Attack>(), "attack");
}

TEST(NameofEnumBridgeTests, EnumToNameResolvesRegisteredValue)
{
    EXPECT_EQ(bee::enum_to_name(bee::nameof_test_samples::Action::Attack), "attack");
}

TEST(NameofEnumBridgeTests, EnumToNameReturnsUnknownForUnregisteredValue)
{
    EXPECT_EQ(bee::enum_to_name(static_cast<bee::nameof_test_samples::Action>(123)), "[Unknown Enum Value]");
}

TEST(NameofEnumBridgeTests, EnumFromNameParsesRegisteredValue)
{
    const auto value = bee::enum_from_name<bee::nameof_test_samples::Action>("move");
    ASSERT_TRUE(value.has_value());
    EXPECT_EQ(*value, bee::nameof_test_samples::Action::Move);
}

TEST(NameofEnumBridgeTests, EnumFromNameReturnsNulloptForUnknownName)
{
    const auto value = bee::enum_from_name<bee::nameof_test_samples::Action>("missing");
    EXPECT_FALSE(value.has_value());
}

TEST(NameofValueNameAutoScanTests, ValueNameShortExtractsFromCompilerForEnumValue)
{
    EXPECT_EQ(bee::value_name_short<bee::nameof_test_samples::AutoAction::Waiting>(), "Waiting");
}

TEST(NameofValueNameAutoScanTests, ValueNameDisplayFallsBackToAutoShortName)
{
    EXPECT_EQ(bee::value_name_display<bee::nameof_test_samples::AutoAction::Running>(), "Running");
}

TEST(NameofValueNameAutoScanTests, ValueNameKeyUsesAutoShortName)
{
    EXPECT_EQ(bee::value_name_key<bee::nameof_test_samples::AutoAction::Done>(), "done");
}

TEST(NameofValueNameAutoScanTests, EnumToNameScansRangeWhenNoEnumEntries)
{
    EXPECT_EQ(bee::enum_to_name(bee::nameof_test_samples::AutoAction::Running), "Running");
}

TEST(NameofValueNameAutoScanTests, EnumFromNameScansRangeWhenNoEnumEntries)
{
    const auto value = bee::enum_from_name<bee::nameof_test_samples::AutoAction>("Waiting");
    ASSERT_TRUE(value.has_value());
    EXPECT_EQ(*value, bee::nameof_test_samples::AutoAction::Waiting);
}

TEST(NameofValueNameAutoScanTests, EnumToNameReturnsUnknownForOutOfScanValue)
{
    EXPECT_EQ(bee::enum_to_name(static_cast<bee::nameof_test_samples::AutoAction>(99)), "[Unknown Enum Value]");
}

TEST(NameofValueNameAutoScanTests, EnumFromNameReturnsNulloptWhenNotFoundByScan)
{
    const auto value = bee::enum_from_name<bee::nameof_test_samples::AutoAction>("Missing");
    EXPECT_FALSE(value.has_value());
}

TEST(NameofValueNameAutoScanTests, EnumToNameWorksWhenAliasCheckEnabled)
{
    EXPECT_EQ(bee::enum_to_name(bee::nameof_test_samples::AutoCheckedAction::Walk), "Walk");
}

TEST(NameofValueNameAutoScanTests, EnumFromNameWorksWhenAliasCheckEnabled)
{
    const auto value = bee::enum_from_name<bee::nameof_test_samples::AutoCheckedAction>("Stand");
    ASSERT_TRUE(value.has_value());
    EXPECT_EQ(*value, bee::nameof_test_samples::AutoCheckedAction::Stand);
}

TEST(NameofEnumFlagsTests, CombinesSingleBitNames)
{
    const auto value = EnumOr(bee::nameof_test_samples::Permission::Write, bee::nameof_test_samples::Permission::Read);
    EXPECT_EQ(bee::enum_flags_to_name(value), "read|write");
}

TEST(NameofEnumFlagsTests, ReturnsUnknownForUnmappedFlagBits)
{
    const auto value = static_cast<bee::nameof_test_samples::Permission>(8);
    EXPECT_EQ(bee::enum_flags_to_name(value), "[Unknown Enum Value]");
}

TEST(NameofEnumFlagsTests, ParsesCombinedFlagsFromText)
{
    const auto value = bee::enum_flags_from_name<bee::nameof_test_samples::Permission>("read|execute");
    ASSERT_TRUE(value.has_value());
    const auto expected = EnumOr(bee::nameof_test_samples::Permission::Read, bee::nameof_test_samples::Permission::Execute);
    EXPECT_EQ(*value, expected);
}

TEST(NameofEnumFlagsTests, ParsesCombinedFlagsWithWhitespace)
{
    const auto value = bee::enum_flags_from_name<bee::nameof_test_samples::Permission>("read | write");
    ASSERT_TRUE(value.has_value());
    const auto expected = EnumOr(bee::nameof_test_samples::Permission::Read, bee::nameof_test_samples::Permission::Write);
    EXPECT_EQ(*value, expected);
}

TEST(NameofEnumFlagsTests, ReturnsNulloptForUnknownFlagToken)
{
    const auto value = bee::enum_flags_from_name<bee::nameof_test_samples::Permission>("read|missing");
    EXPECT_FALSE(value.has_value());
}

TEST(NameofEnumFlagsTests, FallsBackToExactLookupWhenFlagsDisabled)
{
    EXPECT_EQ(bee::enum_flags_to_name(bee::nameof_test_samples::Action::Move), "move");
    const auto exact = bee::enum_flags_from_name<bee::nameof_test_samples::Action>("move");
    ASSERT_TRUE(exact.has_value());
    EXPECT_EQ(*exact, bee::nameof_test_samples::Action::Move);
    const auto combined = bee::enum_flags_from_name<bee::nameof_test_samples::Action>("move|attack");
    EXPECT_FALSE(combined.has_value());
}

TEST(NameofEnumFlagsTests, ReturnsExactNameForZeroWhenRegistered)
{
    EXPECT_EQ(bee::enum_flags_to_name(bee::nameof_test_samples::Permission::None), "none");
}

TEST(NameofEnumFlagsTests, ParsesExactZeroNameWhenRegistered)
{
    const auto value = bee::enum_flags_from_name<bee::nameof_test_samples::Permission>("none");
    ASSERT_TRUE(value.has_value());
    EXPECT_EQ(*value, bee::nameof_test_samples::Permission::None);
}

TEST(NameofEnumFlagsTests, RejectsEmptyInput)
{
    const auto value = bee::enum_flags_from_name<bee::nameof_test_samples::Permission>("");
    EXPECT_FALSE(value.has_value());
}

TEST(NameofEnumFlagsTests, RejectsMalformedTokenSequence)
{
    const auto value = bee::enum_flags_from_name<bee::nameof_test_samples::Permission>("read||write");
    EXPECT_FALSE(value.has_value());
}

TEST(NameofEnumFlagsTests, ReturnsUnknownForCombinedWhenFlagsDisabled)
{
    const auto combined = EnumOr(bee::nameof_test_samples::Action::Move, bee::nameof_test_samples::Action::Attack);
    EXPECT_EQ(bee::enum_flags_to_name(combined), "[Unknown Enum Value]");
}

TEST(NameofEnumFlagsTests, AutoScanFlagsToNameSupportsComposite)
{
    const auto value = EnumOr(bee::nameof_test_samples::AutoPermission::Write, bee::nameof_test_samples::AutoPermission::Read);
    EXPECT_EQ(bee::enum_flags_to_name(value), "Read|Write");
}

TEST(NameofEnumFlagsTests, AutoScanFlagsFromNameSupportsComposite)
{
    const auto value = bee::enum_flags_from_name<bee::nameof_test_samples::AutoPermission>("Read|Execute");
    ASSERT_TRUE(value.has_value());
    const auto expected = EnumOr(bee::nameof_test_samples::AutoPermission::Read, bee::nameof_test_samples::AutoPermission::Execute);
    EXPECT_EQ(*value, expected);
}

TEST(NameofEnumFlagsTests, AutoScanFlagsReturnsUnknownWhenBitNotMapped)
{
    const auto value = static_cast<bee::nameof_test_samples::AutoPermission>(8);
    EXPECT_EQ(bee::enum_flags_to_name(value), "[Unknown Enum Value]");
}
