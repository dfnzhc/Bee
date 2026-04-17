/**
 * @File InlineString.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2026/4/17
 * @Brief This file is part of Bee.
 */

#pragma once

#include <algorithm>
#include <cstring>
#include <memory>
#include <string>
#include <string_view>

namespace bee
{

/// A small-buffer-optimized immutable string. Stores up to kInlineCapacity bytes
/// inline; falls back to heap allocation for longer strings.
class InlineString
{
public:
    static constexpr std::size_t kInlineCapacity = 126;

    InlineString() noexcept = default;

    InlineString(std::string_view sv)
    {
        assign(sv.data(), sv.size());
    }

    InlineString(const char* s)
    {
        if (s) {
            assign(s, std::strlen(s));
        }
    }

    InlineString(const std::string& s)
    {
        assign(s.data(), s.size());
    }

    InlineString(const InlineString& other)
    {
        assign(other.data(), other.size());
    }

    InlineString(InlineString&& other) noexcept
        : size_(other.size_)
        , on_heap_(other.on_heap_)
    {
        if (on_heap_) {
            heap_ = other.heap_;
            other.heap_    = nullptr;
            other.size_    = 0;
            other.on_heap_ = false;
        } else {
            std::memcpy(inline_, other.inline_, size_);
            inline_[size_] = '\0';
        }
    }

    auto operator=(const InlineString& other) -> InlineString&
    {
        if (this != &other) {
            free_heap();
            assign(other.data(), other.size());
        }
        return *this;
    }

    auto operator=(InlineString&& other) noexcept -> InlineString&
    {
        if (this != &other) {
            free_heap();
            size_    = other.size_;
            on_heap_ = other.on_heap_;
            if (on_heap_) {
                heap_ = other.heap_;
                other.heap_    = nullptr;
                other.size_    = 0;
                other.on_heap_ = false;
            } else {
                std::memcpy(inline_, other.inline_, size_);
                inline_[size_] = '\0';
            }
        }
        return *this;
    }

    auto operator=(std::string_view sv) -> InlineString&
    {
        free_heap();
        assign(sv.data(), sv.size());
        return *this;
    }

    ~InlineString()
    {
        free_heap();
    }

    [[nodiscard]] auto data() const noexcept -> const char*
    {
        return on_heap_ ? heap_ : inline_;
    }

    [[nodiscard]] auto size() const noexcept -> std::size_t
    {
        return size_;
    }

    [[nodiscard]] auto empty() const noexcept -> bool
    {
        return size_ == 0;
    }

    [[nodiscard]] auto view() const noexcept -> std::string_view
    {
        return {data(), size_};
    }

    [[nodiscard]] operator std::string_view() const noexcept
    {
        return view();
    }

    [[nodiscard]] auto str() const -> std::string
    {
        return std::string(data(), size_);
    }

    friend auto operator==(const InlineString& a, const InlineString& b) noexcept -> bool
    {
        return a.view() == b.view();
    }

    friend auto operator==(const InlineString& a, std::string_view b) noexcept -> bool
    {
        return a.view() == b;
    }

    friend auto operator==(const InlineString& a, const char* b) noexcept -> bool
    {
        return b ? a.view() == std::string_view(b) : a.empty();
    }

private:
    void assign(const char* src, std::size_t len)
    {
        size_ = len;
        if (len <= kInlineCapacity) {
            on_heap_ = false;
            std::memcpy(inline_, src, len);
            inline_[len] = '\0';
        } else {
            on_heap_ = true;
            heap_    = new char[len + 1];
            std::memcpy(heap_, src, len);
            heap_[len] = '\0';
        }
    }

    void free_heap() noexcept
    {
        if (on_heap_) {
            delete[] heap_;
            heap_    = nullptr;
            on_heap_ = false;
            size_    = 0;
        }
    }

    // kInlineCapacity + 1 for null terminator; total inline storage = 127 bytes.
    // Combined with size_ (2 bytes) + on_heap_ (1 byte) + padding, struct fits in 128-ish bytes.
    char        inline_[kInlineCapacity + 1]{};
    char*       heap_{nullptr};
    std::size_t size_{0};
    bool        on_heap_{false};
};

} // namespace bee
