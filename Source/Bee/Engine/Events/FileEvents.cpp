/**
 * @File FileEvents.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/4/11
 * @Brief This file is part of Bee.
 */

#include "Engine/Events/FileEvents.hpp"

namespace bee {
#if 0
DropFileEvent::DropFileEvent() : Event(EventType::DragFile)
{
}

DropFileEvent::DropFileEvent(const char* fileName) : Event(EventType::DragFile), _fileName(fileName)
{
}

StringView DropFileEvent::fileName() const
{
    return _fileName;
}

String DropFileEvent::toString() const
{
    return std::format("Event - Drag File: {}", _fileName);
}
#endif
} // namespace bee