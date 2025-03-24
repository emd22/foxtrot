#pragma once

#include <Core/Types.hpp>

template <typename Type>
class Vec2Base {
public:
    Vec2Base() = default;

    Vec2Base(Type x, Type y)
        : mX(x), mY(y)
    {
    }

    explicit Vec2Base(Type scalar)
        : mX(scalar), mY(scalar)
    {
    }

    Vec2Base operator + (const Vec2Base &other) const
    {
        Vec2Base result = *this;
        result.mX += other.mX;
        result.mY += other.mY;
        return result;
    }

    Vec2Base operator * (const Vec2Base &other) const
    {
        Vec2Base result = *this;
        result.mX *= other.mX;
        result.mY *= other.mY;
        return result;
    }

    Vec2Base &operator += (const Vec2Base &other)
    {
        this->mX += other.mX;
        this->mY += other.mY;
    }

    Vec2Base &operator -= (const Vec2Base &other)
    {
        this->mX -= other.mX;
        this->mY -= other.mY;
    }

    Vec2Base &operator *= (const Vec2Base &other)
    {
        this->mX *= other.mX;
        this->mY *= other.mY;
    }

    Type GetX() const { return this->mX; }
    Type GetY() const { return this->mY; }

    Type X() const { return this->GetX(); }
    Type Y() const { return this->GetY(); }

    Type Width() const { return this->GetX(); }
    Type Height() const { return this->GetY(); }

    static const Vec2Base<Type> Zero()
    {
        return Vec2Base<Type>(0, 0);
    };

private:
    Type mX;
    Type mY;
};

using Vec2f = Vec2Base<float32>;
using Vec2d = Vec2Base<float64>;


using Vec2i = Vec2Base<int32>;
using Vec2u = Vec2Base<uint32>;
