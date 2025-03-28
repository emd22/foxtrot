#pragma once

#include <cstddef>
#include <stdexcept>
#include <initializer_list>

#include <cstdio>

static void NoMemError()
{
    puts("StaticArray: out of memory error");
    std::terminate();
}

template <typename ElementType>
class StaticArray
{
public:
    struct Iterator
    {
        Iterator(ElementType *ptr, size_t index)
            : mPtr(ptr), mIndex(index)
        {
        }

        Iterator &operator++()
        {
            this->mIndex++;
            return *this;
        }

        Iterator &operator++(int value) {
            Iterator before = *this;
            this->mIndex++;
            return before;
        }

        ElementType &operator * () const
        {
            return this->mPtr[this->mIndex];
        }

        bool operator==(const Iterator &b) const
        {
            return this->mPtr == b.mPtr && this->mIndex == b.mIndex;
        }

    private:
        ElementType *mPtr;
        size_t mIndex;
    };


    StaticArray(size_t element_count)
        : Capacity(element_count)
    {
        try {
            this->Data = new ElementType[element_count];
        }
        catch (std::exception &e) {
            NoMemError();
        }
    }

    StaticArray(std::initializer_list<ElementType> list)
    {
        this->InitSize(list.size());

        int index = 0;
        for (const ElementType obj : list) {
            this->Data[index] = obj;
            index++;
        }
    }

    StaticArray(StaticArray<ElementType> &&other)
    {
        this->Data = std::move(other.Data);
        other.Data = nullptr;

        this->Capacity = other.Capacity;
        this->Size = other.Size;
    }

    StaticArray() = default;

    ~StaticArray()
    {
        this->Free();
    }

    void Free()
    {
        if (this->Data != nullptr) {
            printf("Freeing StaticArray of size %lu (type: %s)\n", this->Size, typeid(ElementType).name());
            delete this->Data;

            this->Data = nullptr;
        }
    }

    Iterator begin()
    {
        return Iterator(this->Data, 0);
    }

    Iterator end()
    {
        return Iterator(this->Data, this->Size);
    }

    operator ElementType *() const
    {
        return this->Data;
    }

    operator ElementType &() const
    {
        return *this->Data;
    }

    const ElementType &operator[] (size_t index) const
    {
        if (index > this->Capacity) {
            throw std::out_of_range("StaticArray access out of range");
        }
        return this->Data[index];
    }

    ElementType &operator[] (size_t index)
    {
        if (index > this->Capacity) {
            throw std::out_of_range("StaticArray access out of range");
        }
        return this->Data[index];
    }

    StaticArray<ElementType> operator=(StaticArray<ElementType> &&other)
    {
        this->Data = std::move(other.Data);
        other.Data = nullptr;

        this->Size = other.Size;
        this->Capacity = other.Capacity;

        return *this;
    }

    void Clear() { this->Size = 0; }
    bool IsEmpty() { return this->Size == 0; }
    bool IsNotEmpty() { return !this->IsEmpty(); }

    void Insert(ElementType object)
    {
        if (this->Size + 1 > this->Capacity) {
            throw std::out_of_range("StaticArray insert is larger than the capacity!");
        }
        this->Data[this->Size++] = object;
    }

    void InitCapacity(size_t element_count)
    {
        if (this->Data != nullptr) {
            throw std::runtime_error("StaticArray has already been previously initialized, cannot InitCapacity!");
        }

        try {
            this->Data = new ElementType[element_count];
        }
        catch (std::exception &e) {
            NoMemError();
        }

        this->Capacity = element_count;
    }

    /**
     *   Initializes an array to contain `element_count` elements, which can be modified externally.
     *
     *   Example:
     *
     *      StaticArray<int32> int_array(10);
     *      int_array.InitSize();
     *
     *      StaticArray<int32> other_array;
     *      other_array.InitSize(15);
     */
    void InitSize(size_t element_count)
    {
        if (this->Capacity == 0) {
            this->InitCapacity(element_count);
        }

        this->Size = this->Capacity;
    }

    void InitSize()
    {
        if (this->Capacity == 0) {
            throw std::runtime_error("Static array capacity has not been set!");
        }

        this->Size = this->Capacity;
    }

public:
    size_t Size = 0;
    size_t Capacity = 0;

    ElementType *Data = nullptr;
};
