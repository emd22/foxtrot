#pragma once

#include <type_traits>

template <typename EnumClass>
std::underlying_type<EnumClass>::type EnumToInt(EnumClass value)
{
    static_cast<typename std::underlying_type<EnumClass>::type>(value);
}
