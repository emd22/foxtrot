#pragma once

#include <cstddef>
#include <stdexcept>

#include <cstdio>

static void NoMemError()
{
    printf("StaticArray: memory error\n");
    std::terminate();
}

template <typename ElementType>
class StaticArray {
public:
    StaticArray(size_t element_count)
        : Capacity(element_count)
    {
        this->Data = new ElementType[element_count];
    }

    StaticArray() = default;

    ~StaticArray()
    {
        this->Free();
    }

    void Free()
    {
        if (this->Data == nullptr) {
            return;
        }
        delete this->Data;
    }

    const ElementType &operator[] (size_t index) const
    {
        if (index > this->Capacity) {
            throw std::runtime_error("StaticArray access out of range");
        }
        return this->Data[index];
    }

    ElementType &operator[] (size_t index)
    {
        if (index > this->Capacity) {
            throw std::runtime_error("StaticArray access out of range");
        }
        return this->Data[index];
    }

    void InitCapacity(size_t element_count)
    {
        this->Data = new ElementType[element_count];
        this->Capacity = element_count;
    }

    size_t Size = 0;
    size_t Capacity = 0;

    ElementType *Data = nullptr;
};
