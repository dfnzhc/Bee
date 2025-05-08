/**
 * @File FileEvents.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/4/11
 * @Brief This file is part of Bee.
 */
 
#pragma once

#include "Engine/Events/Event.hpp"

namespace bee {

#if 0
class BEE_API DropFileEvent final : public Event
{
public:
    DropFileEvent();
    DropFileEvent(const char* fileName);

    ~DropFileEvent() override = default;

    StringView fileName() const;

    String toString() const override;

private:
    StringView _fileName;
};
#endif


} // namespace bee