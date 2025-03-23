#ifdef AR_NO_SIMD

#include <Core/Defines.hpp>
#include <Math/Vec3.hpp>

inline Vec3f::Vec3f(float32 x, float32 y, float32 z)
    : mX(x), mY(y), mZ(z)
{
}

inline Vec3f::Vec3f(float32 scalar)
    : mX(scalar), mY(scalar), mZ(scalar)
{
}

inline Vec3f Vec3f::operator + (const Vec3f &other) const
{
    Vec3f result;

    result.mX = this->mX + other.mX;
    result.mY = this->mY + other.mY;
    result.mZ = this->mZ + other.mZ;

    return result;
}

inline Vec3f Vec3f::operator * (const Vec3f &other) const
{
    Vec3f result;

    result.mX = this->mX * other.mX;
    result.mY = this->mY * other.mY;
    result.mZ = this->mZ * other.mZ;

    return result;
}

inline Vec3f &Vec3f::operator += (const Vec3f &other)
{
    this->mX += other.mX;
    this->mY += other.mY;
    this->mZ += other.mZ;

    return *this;
}

inline Vec3f &Vec3f::operator -= (const Vec3f &other)
{
    this->mX -= other.mX;
    this->mY -= other.mY;
    this->mZ -= other.mZ;

    return *this;
}

inline Vec3f &Vec3f::operator *= (const Vec3f &other)
{
    this->mX *= other.mX;
    this->mY *= other.mY;
    this->mZ *= other.mZ;

    return *this;
}

inline Vec3f Vec3f::MulAdd(const Vec3f &add_value, const Vec3f &mul_a, const Vec3f &mul_b)
{
    Vec3f result;
    result = add_value + (mul_a * mul_b);
    return result;
}

inline float32 Vec3f::GetX() const
{
    return this->mX;
}

inline float32 Vec3f::GetY() const
{
    return this->mY;
}

inline float32 Vec3f::GetZ() const
{
    return this->mZ;
}

#endif // #ifdef AR_NO_SIMD
