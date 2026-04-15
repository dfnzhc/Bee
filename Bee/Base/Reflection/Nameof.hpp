/**
 * @File Nameof.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/3/17
 * @Brief This file is part of Bee.
 */

#pragma once

#include <source_location>
#include <array>
#include <bit>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#include "Base/String/Ascii.hpp"
#include "Base/String/Naming.hpp"
#include "Base/String/Text.hpp"

namespace bee
{

namespace Customize
{
    // TypeName 用于覆盖 short/key 流程使用的源名称。
    // 典型用途：跨模块保持稳定的序列化键命名。
    template <typename T>
    struct TypeName
    {
    };

    // DisplayName 用于覆盖面向用户的展示文本，优先级高于 TypeName。
    // 典型用途：UI 标签、日志与可读文本。
    template <typename T>
    struct DisplayName
    {
    };

    template <typename E>
    struct EnumEntry
    {
        E                value;
        std::string_view name;
        std::string_view display;
    };

    template <typename E>
    struct EnumEntries
    {
    };

    // 偏特化必须对提供下列所有三个值的定义，不然会引起奇怪的模板错误 (e.g., MSVC C2131)
    template <typename E>
    struct EnumScanRange
    {
        static constexpr int kMin = -64;
        static constexpr int kMax = 64;
        // 仅在扫描范围较小且 CI 中编译器值签名格式稳定时启用。
        // 过大的扫描范围会显著增加编译期开销。
        static constexpr bool kEnableAliasCheck = false;
    };

    template <typename E>
    struct EnumFlags
    {
        static constexpr bool kEnabled = false;
    };

} // namespace Customize

namespace internal
{

    inline constexpr std::string_view kUnsupportedCompilerFormat = "[Unsupported Compiler Format]";
    inline constexpr std::string_view kUnknownEnumValueName      = "[Unknown Enum Value]";
    inline constexpr int              kMaxAliasCheckScanCount    = 64;

    template <typename T>
    concept ValidEnumScanRange = requires {
        { T::kMin } -> std::convertible_to<int>;
        { T::kMax } -> std::convertible_to<int>;
        { T::kEnableAliasCheck } -> std::convertible_to<bool>;
    };

    constexpr auto SliceTemplateArg(std::string_view text, std::string_view marker) -> std::string_view
    {
        const auto marker_pos = text.find(marker);
        if (marker_pos == std::string_view::npos) {
            return kUnsupportedCompilerFormat;
        }

        const auto start = marker_pos + marker.size();
        if (start >= text.size()) {
            return kUnsupportedCompilerFormat;
        }

        int depth = 0;
        for (std::size_t i = start; i < text.size(); ++i) {
            if (text[i] == '<') {
                ++depth;
                continue;
            }

            if (text[i] == '>') {
                if (depth == 0) {
                    if (i <= start) {
                        return kUnsupportedCompilerFormat;
                    }
                    return text.substr(start, i - start);
                }
                --depth;
            }
        }

        return kUnsupportedCompilerFormat;
    }

    template <typename T>
    consteval auto WrappedTypeName() -> std::string_view
    {
        return std::source_location::current().function_name();
    }

    template <typename T>
    consteval auto TypeNameRawImpl() -> std::string_view
    {
        constexpr auto wrapped = WrappedTypeName<T>();

#if defined(__clang__)
        return Slice(wrapped, "[T = ", "]", kUnsupportedCompilerFormat);
#elif defined(__GNUC__)
        return Slice(wrapped, "with T = ", "]", kUnsupportedCompilerFormat);
#elif defined(_MSC_VER)
        return SliceTemplateArg(wrapped, "WrappedTypeName<");
#else
        return kUnsupportedCompilerFormat;
#endif
    }

    template <auto V>
    consteval auto WrappedValueName() -> std::string_view
    {
        return std::source_location::current().function_name();
    }

    template <auto V>
    consteval auto ValueNameRawImpl() -> std::string_view
    {
        constexpr auto wrapped = WrappedValueName<V>();

#if defined(__clang__)
        return Slice(wrapped, "[V = ", "]", kUnsupportedCompilerFormat);
#elif defined(__GNUC__)
        return Slice(wrapped, "with auto V = ", "]", kUnsupportedCompilerFormat);
#elif defined(_MSC_VER)
        return SliceTemplateArg(wrapped, "WrappedValueName<");
#else
        return kUnsupportedCompilerFormat;
#endif
    }

    constexpr auto StripLeadingKeywords(std::string_view text) -> std::string_view
    {
        text = TrimSpaces(text);

        constexpr std::string_view prefixes[] = {"enum class ", "const ", "volatile ", "class ", "struct ", "enum "};

        bool removed = true;
        while (removed) {
            removed = false;
            for (auto prefix : prefixes) {
                if (text.starts_with(prefix)) {
                    text.remove_prefix(prefix.size());
                    text    = TrimSpaces(text);
                    removed = true;
                    break;
                }
            }
        }

        return text;
    }

    constexpr auto FindTemplateEnd(std::string_view text, std::size_t start) -> std::size_t
    {
        if (start >= text.size() || text[start] != '<') {
            return std::string_view::npos;
        }

        int depth = 0;
        for (std::size_t i = start; i < text.size(); ++i) {
            if (text[i] == '<') {
                ++depth;
                continue;
            }
            if (text[i] == '>') {
                --depth;
                if (depth == 0) {
                    return i + 1;
                }
            }
        }

        return std::string_view::npos;
    }

    constexpr auto TypeNameShortFromRaw(std::string_view raw) -> std::string_view
    {
        if (raw == kUnsupportedCompilerFormat) {
            return raw;
        }

        // 规则：
        // 1) 移除 MSVC 注入的前导关键字；
        // 2) 仅在模板参数之前的基类型片段中剥离命名空间；
        // 3) 保留模板结构，同时移除尾部 cvref/pointer 标记。
        auto       text          = StripLeadingKeywords(raw);
        const auto lt_pos        = text.find('<');
        const auto base_name_len = (lt_pos == std::string_view::npos) ? text.size() : lt_pos;
        const auto ns_pos        = text.substr(0, base_name_len).rfind("::");
        if (ns_pos != std::string_view::npos) {
            text.remove_prefix(ns_pos + 2);
        }
        text = TrimSpaces(text);

        const auto new_lt_pos = text.find('<');
        if (new_lt_pos == std::string_view::npos) {
            const auto end = text.find_first_of(" &*");
            if (end != std::string_view::npos) {
                return text.substr(0, end);
            }
            return text;
        }

        const auto template_end = FindTemplateEnd(text, new_lt_pos);
        if (template_end == std::string_view::npos) {
            return text;
        }
        return text.substr(0, template_end);
    }

    constexpr auto IsLikelyAutoEnumRawName(std::string_view raw) -> bool
    {
        if (raw.empty() || raw == kUnsupportedCompilerFormat) {
            return false;
        }

        raw = TrimSpaces(raw);
        if (raw.empty()) {
            return false;
        }
        if (raw.find('(') != std::string_view::npos || raw.find(')') != std::string_view::npos) {
            return false;
        }

        auto       token  = raw;
        const auto ns_pos = token.rfind("::");
        if (ns_pos != std::string_view::npos) {
            token.remove_prefix(ns_pos + 2);
        }
        token = TrimSpaces(token);
        if (token.empty()) {
            return false;
        }
        if (!(IsAsciiAlpha(token.front()) || token.front() == '_')) {
            return false;
        }
        for (char c : token) {
            if (!(IsAsciiAlpha(c) || IsAsciiDigit(c) || c == '_')) {
                return false;
            }
        }
        return true;
    }

    constexpr auto ValueNameShortFromRaw(std::string_view raw) -> std::string_view
    {
        if (!IsLikelyAutoEnumRawName(raw)) {
            return kUnknownEnumValueName;
        }

        auto       token  = TrimSpaces(raw);
        const auto ns_pos = token.rfind("::");
        if (ns_pos != std::string_view::npos) {
            token.remove_prefix(ns_pos + 2);
        }
        token = TrimSpaces(token);
        if (token.empty()) {
            return kUnknownEnumValueName;
        }
        return token;
    }

    template <typename E, int Min, int... Offsets>
    consteval auto BuildAutoEnumEntries(std::integer_sequence<int, Offsets...>)
    {
        return std::array<Customize::EnumEntry<E>, sizeof...(Offsets)>{Customize::EnumEntry<E>{
                static_cast<E>(Min + Offsets), ValueNameShortFromRaw(ValueNameRawImpl<static_cast<E>(Min + Offsets)>()), ""
        }...};
    }

    template <typename E>
    consteval auto AutoEnumEntries()
    {
        static_assert(
                ValidEnumScanRange<Customize::EnumScanRange<E>>,
                "EnumScanRange specialization must provide kMin (int), kMax (int), and kEnableAliasCheck (bool)."
        );
        constexpr int kMin  = Customize::EnumScanRange<E>::kMin;
        constexpr int kMax  = Customize::EnumScanRange<E>::kMax;
        constexpr int kSize = kMax - kMin + 1;
        static_assert(kMax >= kMin);
        return BuildAutoEnumEntries<E, kMin>(std::make_integer_sequence<int, kSize>{});
    }

    template <typename E, std::size_t N>
    consteval auto AutoEnumNoDuplicateNames(const std::array<Customize::EnumEntry<E>, N>& entries) -> bool
    {
        for (std::size_t i = 0; i < N; ++i) {
            if (entries[i].name == kUnknownEnumValueName) {
                continue;
            }
            for (std::size_t j = i + 1; j < N; ++j) {
                if (entries[j].name == kUnknownEnumValueName) {
                    continue;
                }
                if (entries[i].name == entries[j].name) {
                    return false;
                }
            }
        }
        return true;
    }

    template <typename E>
    constexpr auto ShouldRunAliasCheck() -> bool
    {
        if constexpr (Customize::EnumScanRange<E>::kEnableAliasCheck) {
            constexpr int kMin  = Customize::EnumScanRange<E>::kMin;
            constexpr int kMax  = Customize::EnumScanRange<E>::kMax;
            constexpr int kSize = kMax - kMin + 1;
            static_assert(
                    kSize <= kMaxAliasCheckScanCount,
                    "Alias check is enabled with a scan range that is too large. Reduce EnumScanRange or disable alias check."
            );
            return true;
        } else {
            return false;
        }
    }

    // Compact enum entry table: holds up to Capacity entries with only 'size' valid.
    // Entries are sorted by underlying value for binary search support.
    template <typename E, std::size_t Capacity>
    struct EnumTable
    {
        std::array<Customize::EnumEntry<E>, Capacity> entries{};
        std::size_t                                   size = 0;

        constexpr auto begin() const
        {
            return entries.begin();
        }

        constexpr auto end() const
        {
            return entries.begin() + static_cast<std::ptrdiff_t>(size);
        }
    };

    template <typename E, std::size_t N>
    consteval auto SortTableByValue(EnumTable<E, N> table) -> EnumTable<E, N>
    {
        using U = std::underlying_type_t<E>;
        for (std::size_t i = 1; i < table.size; ++i) {
            auto        key     = table.entries[i];
            auto        key_val = static_cast<U>(key.value);
            std::size_t j       = i;
            while (j > 0 && static_cast<U>(table.entries[j - 1].value) > key_val) {
                table.entries[j] = table.entries[j - 1];
                --j;
            }
            table.entries[j] = key;
        }
        return table;
    }

    template <typename E>
    constexpr auto EnumToUnsigned(E value) -> std::make_unsigned_t<std::underlying_type_t<E>>
    {
        using U = std::make_unsigned_t<std::underlying_type_t<E>>;
        return static_cast<U>(static_cast<std::underlying_type_t<E>>(value));
    }

    // Binary search by value in a sorted EnumTable. O(log n).
    template <typename E, std::size_t N>
    constexpr auto FindEnumNameByValue(E value, const EnumTable<E, N>& table) -> std::string_view
    {
        using U            = std::underlying_type_t<E>;
        const auto  target = static_cast<U>(value);
        std::size_t lo     = 0;
        std::size_t hi     = table.size;
        while (lo < hi) {
            const std::size_t mid     = lo + (hi - lo) / 2;
            const auto        mid_val = static_cast<U>(table.entries[mid].value);
            if (mid_val < target) {
                lo = mid + 1;
            } else if (mid_val > target) {
                hi = mid;
            } else {
                return table.entries[mid].name;
            }
        }
        return kUnknownEnumValueName;
    }

    template <typename E, typename Entries>
    constexpr auto FindEnumValueByName(std::string_view name, const Entries& entries) -> std::optional<E>
    {
        for (const auto& entry : entries) {
            if (entry.name == name) {
                return entry.value;
            }
        }
        return std::nullopt;
    }

    template <typename E, typename Entries>
    auto EnumFlagsToNameFromEntries(E value, const Entries& entries, bool allow_composite) -> std::string
    {
        const auto exact_name = FindEnumNameByValue(value, entries);
        if (exact_name != kUnknownEnumValueName) {
            return std::string(exact_name);
        }
        if (!allow_composite) {
            return std::string(kUnknownEnumValueName);
        }

        using U     = std::make_unsigned_t<std::underlying_type_t<E>>;
        U remaining = EnumToUnsigned(value);
        if (remaining == 0) {
            return std::string(kUnknownEnumValueName);
        }

        std::string   result;
        constexpr int kBitCount = std::numeric_limits<U>::digits;
        for (int bit = 0; bit < kBitCount; ++bit) {
            const U mask = static_cast<U>(U{1} << bit);
            if ((remaining & mask) == 0) {
                continue;
            }

            std::string_view bit_name = kUnknownEnumValueName;
            for (const auto& entry : entries) {
                const U entry_value = EnumToUnsigned(entry.value);
                if (!std::has_single_bit(entry_value)) {
                    continue;
                }
                if (entry_value == mask) {
                    bit_name = entry.name;
                    break;
                }
            }

            if (bit_name == kUnknownEnumValueName) {
                return std::string(kUnknownEnumValueName);
            }

            if (!result.empty()) {
                result.push_back('|');
            }
            result.append(bit_name);
            remaining = static_cast<U>(remaining & ~mask);
        }

        if (remaining != 0 || result.empty()) {
            return std::string(kUnknownEnumValueName);
        }
        return result;
    }

    template <typename E, typename Entries>
    auto EnumFlagsFromNameFromEntries(std::string_view name, const Entries& entries, bool allow_composite) -> std::optional<E>
    {
        const auto trimmed = TrimSpaces(name);
        if (trimmed.empty()) {
            return std::nullopt;
        }

        const auto exact_value = FindEnumValueByName<E>(trimmed, entries);
        if (exact_value.has_value()) {
            return exact_value;
        }
        if (!allow_composite) {
            return std::nullopt;
        }

        using U  = std::make_unsigned_t<std::underlying_type_t<E>>;
        U result = 0;

        std::size_t start = 0;
        while (start < trimmed.size()) {
            const auto end           = trimmed.find('|', start);
            const auto token         = (end == std::string_view::npos) ? trimmed.substr(start) : trimmed.substr(start, end - start);
            const auto token_trimmed = TrimSpaces(token);
            if (token_trimmed.empty()) {
                return std::nullopt;
            }

            const auto token_value = FindEnumValueByName<E>(token_trimmed, entries);
            if (!token_value.has_value()) {
                return std::nullopt;
            }

            result = static_cast<U>(result | EnumToUnsigned(*token_value));
            if (end == std::string_view::npos) {
                break;
            }
            start = end + 1;
        }

        return static_cast<E>(result);
    }

    // Resolves the effective enum entry table for E:
    // uses Customize::EnumEntries if specialized, otherwise auto-scans with optional alias check.
    // Returns an EnumTable with only valid entries, sorted by underlying value for binary search.
    template <typename E>
    consteval auto ResolvedEnumEntries()
    {
        if constexpr (requires { Customize::EnumEntries<E>::entries; }) {
            constexpr auto&          src = Customize::EnumEntries<E>::entries;
            EnumTable<E, src.size()> table;
            for (const auto& entry : src) {
                table.entries[table.size++] = entry;
            }
            return SortTableByValue(table);
        } else {
            constexpr auto all = AutoEnumEntries<E>();
            if constexpr (ShouldRunAliasCheck<E>()) {
                static_assert(
                        AutoEnumNoDuplicateNames(all),
                        "Auto enum scan found duplicate names for different values. Please provide Customize::EnumEntries."
                );
            }
            EnumTable<E, all.size()> table;
            for (const auto& entry : all) {
                if (entry.name != kUnknownEnumValueName) {
                    table.entries[table.size++] = entry;
                }
            }
            return SortTableByValue(table);
        }
    }

} // namespace internal

template <typename T>
constexpr auto type_name_raw() noexcept -> std::string_view
{
    return internal::TypeNameRawImpl<T>();
}

template <typename T>
constexpr auto type_name_short() noexcept -> std::string_view
{
    // 优先级：Customize::TypeName > 自动提取的 short 名称。
    if constexpr (requires { Customize::TypeName<std::remove_cvref_t<T>>::value; }) {
        return std::string_view(Customize::TypeName<std::remove_cvref_t<T>>::value);
    }
    return internal::TypeNameShortFromRaw(type_name_raw<T>());
}

template <typename T>
auto type_name_key() noexcept -> std::string_view
{
    // Key 基于 short 名称生成，天然继承 TypeName 覆盖链路。
    // 使用静态缓存避免热路径重复分配。
    const auto short_name = type_name_short<T>();
    if (short_name == internal::kUnsupportedCompilerFormat) {
        return short_name;
    }

    static const std::string key = ToSnakeCase(short_name);
    return key;
}

template <typename T>
constexpr auto type_name_display() noexcept -> std::string_view
{
    // 优先级：DisplayName > TypeName > 自动提取的 short 名称。
    if constexpr (requires { Customize::DisplayName<std::remove_cvref_t<T>>::value; }) {
        return std::string_view(Customize::DisplayName<std::remove_cvref_t<T>>::value);
    }
    return type_name_short<T>();
}

template <auto V>
constexpr auto value_name_short() noexcept -> std::string_view
{
    using E = std::remove_cvref_t<decltype(V)>;
    static_assert(std::is_enum_v<E>);

    if constexpr (requires { Customize::EnumEntries<E>::entries; }) {
        for (const auto& entry : Customize::EnumEntries<E>::entries) {
            if (entry.value == V) {
                return entry.name;
            }
        }
    }

    return internal::ValueNameShortFromRaw(internal::ValueNameRawImpl<V>());
}

template <auto V>
auto value_name_key() noexcept -> std::string_view
{
    const auto short_name = value_name_short<V>();
    if (short_name == internal::kUnknownEnumValueName) {
        return short_name;
    }

    static const std::string key = ToSnakeCase(short_name);
    return key;
}

template <auto V>
constexpr auto value_name_display() noexcept -> std::string_view
{
    using E = std::remove_cvref_t<decltype(V)>;
    static_assert(std::is_enum_v<E>);

    if constexpr (requires { Customize::EnumEntries<E>::entries; }) {
        for (const auto& entry : Customize::EnumEntries<E>::entries) {
            if (entry.value == V) {
                if (!entry.display.empty()) {
                    return entry.display;
                }
                return entry.name;
            }
        }
    }

    return value_name_short<V>();
}

template <typename E>
constexpr auto enum_to_name(E value) noexcept -> std::string_view
{
    static_assert(std::is_enum_v<E>);

    constexpr auto entries = internal::ResolvedEnumEntries<E>();
    return internal::FindEnumNameByValue(value, entries);
}

template <typename E>
constexpr auto enum_from_name(std::string_view name) noexcept -> std::optional<E>
{
    static_assert(std::is_enum_v<E>);

    constexpr auto entries = internal::ResolvedEnumEntries<E>();
    return internal::FindEnumValueByName<E>(name, entries);
}

template <typename E>
auto enum_flags_to_name(E value) -> std::string
{
    static_assert(std::is_enum_v<E>);

    constexpr bool allow_composite = Customize::EnumFlags<E>::kEnabled;
    constexpr auto entries         = internal::ResolvedEnumEntries<E>();
    return internal::EnumFlagsToNameFromEntries(value, entries, allow_composite);
}

template <typename E>
auto enum_flags_from_name(std::string_view name) -> std::optional<E>
{
    static_assert(std::is_enum_v<E>);

    constexpr bool allow_composite = Customize::EnumFlags<E>::kEnabled;
    constexpr auto entries         = internal::ResolvedEnumEntries<E>();
    return internal::EnumFlagsFromNameFromEntries<E>(name, entries, allow_composite);
}

static_assert(!type_name_raw<int>().empty());
static_assert(type_name_raw<int>() == type_name_raw<int>());
static_assert(type_name_short<const int&>().find("int") != std::string_view::npos);
static_assert(std::is_same_v<decltype(type_name_key<int>()), std::string_view>);
static_assert(noexcept(type_name_key<int>()));

#define BEE_ENUM_SCAN_RANGE(EnumName, RangeMin, RangeMax, EnableAliasCheck) \
    namespace Customize                                                     \
    {                                                                       \
        template <>                                                         \
        struct EnumScanRange<EnumName>                                      \
        {                                                                   \
            static constexpr int  kMin              = RangeMin;             \
            static constexpr int  kMax              = RangeMax;             \
            static constexpr bool kEnableAliasCheck = EnableAliasCheck;     \
        };                                                                  \
    }

#define BEE_ENUM_SCAN_COUNT(EnumName, Count) BEE_ENUM_SCAN_RANGE(EnumName, 0, Count - 1, false)

} // namespace bee
