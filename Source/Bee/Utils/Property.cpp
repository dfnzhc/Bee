/**
 * @File Property.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2025/4/13
 * @Brief This file is part of Bee.
 */

#include "Utils/Property.hpp"

#include "Base/Error.hpp"
#include "Base/Logger.hpp"

#include <SDL3/SDL.h>
#include <SDL3/SDL_properties.h>

using namespace bee;

namespace {
std::unordered_map<StringView, SDL_PropertiesID> PropertiesMap;

constexpr SDL_PropertiesID InvalidPropID = 0;

SDL_PropertiesID GetOrCreatePropertiesID(StringView propName)
{
    SDL_PropertiesID id = InvalidPropID;
    if (PropertiesMap.contains(propName)) {
        id = PropertiesMap.at(propName);
    }

    if (id == InvalidPropID) {
        id = SDL_CreateProperties();
        BEE_ASSERT(id != InvalidPropID, "Failed to create sdl properties: {}.", SDL_GetError());

        PropertiesMap.emplace(propName, id);
    }

    return id;
}

bool bIsEngineRunning = true;
} // namespace 

bool Property::Has(StringView propName)
{
    if (!PropertiesMap.contains(propName))
        return false;

    return SDL_HasProperty(PropertiesMap.at(propName), propName.data());
}

bool Property::Lock(StringView propName)
{
    if (!Has(propName))
        return false;

    return SDL_LockProperties(PropertiesMap.at(propName));
}

void Property::Unlock(StringView propName)
{
    if (Has(propName))
        SDL_UnlockProperties(PropertiesMap.at(propName));
}

Property::Type Property::GetType(StringView propName)
{
    if (!Has(propName))
        return Type::Unknown;

    const auto type = SDL_GetPropertyType(PropertiesMap.at(propName), propName.data());
    switch (type) {
    case SDL_PROPERTY_TYPE_BOOLEAN: return Type::Boolean;
    case SDL_PROPERTY_TYPE_FLOAT: return Type::Float32;
    case SDL_PROPERTY_TYPE_NUMBER: return Type::SInt64;
    case SDL_PROPERTY_TYPE_STRING: return Type::String;
    default: ;
    }

    return Type::Unknown;
}

bool Property::CheckType(StringView propName, Type type)
{
    return GetType(propName) == type;
}

bool Property::SetBool(StringView propName, bool value)
{
    return SDL_SetBooleanProperty(GetOrCreatePropertiesID(propName), propName.data(), value);
}

bool Property::SetFloat(StringView propName, f32 value)
{
    return SDL_SetFloatProperty(GetOrCreatePropertiesID(propName), propName.data(), value);
}

bool Property::SetNumber(StringView propName, i64 value)
{
    return SDL_SetNumberProperty(GetOrCreatePropertiesID(propName), propName.data(), value);
}

bool Property::SetString(StringView propName, StringView value)
{
    return SDL_SetStringProperty(GetOrCreatePropertiesID(propName), propName.data(), value.data());
}

bool Property::GetBool(StringView propName, bool defaultValue)
{
    if (!Has(propName))
        return defaultValue;

    return SDL_GetBooleanProperty(GetOrCreatePropertiesID(propName), propName.data(), defaultValue);
}

f32 Property::GetFloat(StringView propName, f32 defaultValue)
{
    if (!Has(propName))
        return defaultValue;

    return SDL_GetFloatProperty(GetOrCreatePropertiesID(propName), propName.data(), defaultValue);
}

i64 Property::GetNumber(StringView propName, i64 defaultValue)
{
    if (!Has(propName))
        return defaultValue;

    return SDL_GetNumberProperty(GetOrCreatePropertiesID(propName), propName.data(), defaultValue);
}

StringView Property::GetString(StringView propName, StringView defaultValue)
{
    if (!Has(propName))
        return defaultValue;

    return SDL_GetStringProperty(GetOrCreatePropertiesID(propName), propName.data(), defaultValue.data());
}

bool Property::IsEngineRunning()
{
    return GetBool(Property_EngineRunningName, false);
}

void Property::RequestEngineExit()
{
    SetBool(Property_EngineRunningName, false);
}