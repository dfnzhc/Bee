/**
 * @File Error.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/10/21
 * @Brief This file is part of Bee.
 */

#pragma once

#include <expected>

#include "Core/Base/Defines.hpp"

namespace Bee
{
    enum class ErrorDomain : u16
    {
        NoError = 0x0000,

        // === 系统错误 ===
        System     = 0x0001,
        Memory     = 0x0002,
        FileSystem = 0x0003,
        Threading  = 0x0004,

        // === 层级错误 ===
        Core        = 0x1001,
        Platform    = 0x1002,
        Graphics    = 0x1003,
        Engine      = 0x1004,
        Application = 0x1005,

        // === 其他分类 ===
        Physics   = 0x2001,
        Animation = 0x2002,
        Resource  = 0x2003,
        Editor    = 0x2004,

        // 未知错误
        Unknown = 0xFFFF
    };

    constexpr std::string_view GetErrorDomainName(ErrorDomain domain) noexcept;

    struct ErrorId
    {
        u16 domain = {};
        u16 code   = {};

        // clang-format off
        constexpr ErrorId() noexcept = default;
        constexpr ErrorId(ErrorDomain d, u16 c) noexcept : domain(static_cast<u16>(d)), code(c) {}
        constexpr explicit ErrorId(u32 combined) noexcept : domain(static_cast<u16>(combined >> 16)), code(static_cast<u16>(combined & 0xFFFF)) {}
        constexpr ErrorId(ErrorDomain d, std::integral auto c) : ErrorId(d, static_cast<u16>(c)) {}
        // clang-format on

        // === 转换操作 ===
        constexpr u32 value() const noexcept
        {
            return (static_cast<u32>(domain) << 16) | code;
        }

        std::string hex() const;
        std::string format() const;

        // === 访问 ===
        constexpr ErrorDomain getDomain() const noexcept
        {
            return static_cast<ErrorDomain>(domain);
        }

        constexpr u16 getCode() const noexcept
        {
            return code;
        }

        constexpr bool isSuccess() const noexcept
        {
            return domain == 0 && code == 0;
        }

        // === 比较操作 ===
        constexpr auto operator<=>(const ErrorId&) const = default;
    };

    class Error
    {
    public:
        constexpr Error() = default;

        // clang-format off
        constexpr explicit Error(ErrorId id) : _id(id) {}
        constexpr Error(ErrorDomain domain, u16 code) : _id(domain, code) {}
        constexpr Error(ErrorDomain domain, std::integral auto code) : Error(domain, static_cast<u16>(code)){}
        // clang-format on

        // === 基础访问 ===
        constexpr ErrorId id() const noexcept
        {
            return _id;
        }

        constexpr ErrorDomain domain() const noexcept
        {
            return _id.getDomain();
        }

        constexpr u16 code() const noexcept
        {
            return _id.getCode();
        }

        // === 状态检查 ===
        constexpr bool isSuccess() const noexcept
        {
            return _id.isSuccess();
        }

        constexpr bool isError() const noexcept
        {
            return !isSuccess();
        }

        constexpr explicit operator bool() const noexcept
        {
            return isError();
        }

        constexpr bool isDomain(ErrorDomain domain) const noexcept
        {
            return _id.getDomain() == domain;
        }

        constexpr bool isCode(u16 code) const noexcept
        {
            return _id.getCode() == code;
        }

        // === 比较操作 ===
        constexpr auto operator<=>(const Error&) const = default;

    private:
        ErrorId _id = {};
    };

    // ==================== 工具类型定义 ====================

    // 带值的结果类型
    template <typename T>
    using Result = std::expected<T, Error>;

    // 无值的结果类型
    using VResult = std::expected<void, Error>;

    // 创建成功结果
    template <typename T>
    constexpr Result<std::decay_t<T>> Ok(T&& value)
    {
        return std::expected<std::decay_t<T>, Error>{std::forward<T>(value)};
    }

    // 创建成功的无值结果
    constexpr VResult Ok()
    {
        return std::expected<void, Error>{};
    }

    // 创建错误结果
    template <typename... Args>
    constexpr auto Err(Args&&... args)
    {
        return std::unexpected{Error{std::forward<Args>(args)...}};
    }

    // ==================== 特征 ====================

    namespace Details
    {
        // 是否为 Result 类型
        template <typename T>
        struct is_result : std::false_type
        {
        };

        template <typename T>
        struct is_result<Result<T>> : std::true_type
        {
        };

        template <>
        struct is_result<VResult> : std::true_type
        {
        };

        // 获取 Result 的值类型
        template <typename T>
        struct result_value_type
        {
        };

        template <typename T>
        struct result_value_type<Result<T>>
        {
            using type = T;
        };

        template <>
        struct result_value_type<VResult>
        {
            using type = void;
        };
    } // namespace Details


    template <typename T>
    constexpr bool IsResult = Details::is_result<T>::value;

    template <typename T>
    using ResultValueType = typename Details::result_value_type<T>::type;

    // ==================== 宏工具 ====================

    // 尝试获取结果值，失败时返回错误
    #define BEE_TRY(expr)                                                           \
        ({                                                                          \
            auto&& BEE_UNIQUE_VAR(_result) = (expr);                                \
            if (!BEE_UNIQUE_VAR(_result).has_value()) {                             \
                return std::unexpected{std::move(BEE_UNIQUE_VAR(_result).error())}; \
            }                                                                       \
            std::move(BEE_UNIQUE_VAR(_result).value());                             \
        })

    // 尝试执行无值操作，失败时返回错误
    #define BEE_TRY_VOID(expr)                                                      \
        do {                                                                        \
            auto&& BEE_UNIQUE_VAR(_result) = (expr);                                \
            if (!BEE_UNIQUE_VAR(_result).has_value()) {                             \
                return std::unexpected{std::move(BEE_UNIQUE_VAR(_result).error())}; \
            }                                                                       \
        } while(0)

    // 尝试获取结果值，失败时执行指定操作并返回默认值
    #define BEE_TRY_OR(expr, default_value)                     \
        ({                                                      \
            auto&& BEE_UNIQUE_VAR(_result) = (expr);            \
            BEE_UNIQUE_VAR(_result).has_value() ?               \
                std::move(BEE_UNIQUE_VAR(_result).value()) :    \
                (default_value);                                \
        })
} // namespace Bee

namespace std
{
    template <>
    struct hash<Bee::ErrorId>
    {
        size_t operator()(const Bee::ErrorId& id) const noexcept
        {
            return std::hash<uint32_t>{}(id.value());
        }
    };
}
