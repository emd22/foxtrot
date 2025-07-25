#pragma once

#include "Types.hpp"

#include <stdexcept>

template <typename T, unsigned int TCapacity>
class FxStackArray
{
public:
    FxStackArray() = default;

    void Insert(const T& value)
    {
        if (Size + 1 > Capacity) {
            throw std::out_of_range("FxStackArray insert out of range");
        }
        Data[Size++] = value;
    }

    const T& operator[] (size_t index) const
    {
        if (index > Capacity) {
            throw std::out_of_range("FxStackArray access out of range");
        }
        return Data[index];
    }

    T& operator[] (size_t index)
    {
        if (index > Capacity) {
            throw std::out_of_range("FxStackArray access out of range");
        }
        return Data[index];
    }

    operator T* () noexcept
    {
        return Data;
    }

    operator const T* () const noexcept
    {
        return Data;
    }

public:
    uint32 Size = 0;
    const uint32 Capacity = TCapacity;

    T Data[TCapacity];

};
