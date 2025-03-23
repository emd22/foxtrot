#ifdef AR_NO_SIMD

#include <Core/Defines.hpp>
#include <Math/Vec4.hpp>

inline Vec4f::Vec4f(float32 x, float32 y, float32 z, float32 w)
    : mX(x), mY(y), mZ(z), mW(w)
{
}

inline Vec4f::Vec4f(float32 scalar)
    : mX(scalar), mY(scalar), mZ(scalar), mW(scalar)
{
}

inline Vec4f Vec4f::operator + (const Vec4f &other) const
{
    Vec4f result;

    result.mX = this->mX + other.mX;
    result.mY = this->mY + other.mY;
    result.mZ = this->mZ + other.mZ;
    result.mW = this->mW + other.mW;

    return result;
}

inline Vec4f Vec4f::operator * (const Vec4f &other) const
{
    Vec4f result;

    result.mX = this->mX * other.mX;
    result.mY = this->mY * other.mY;
    result.mZ = this->mZ * other.mZ;
    result.mW = this->mW * other.mW;

    return result;
}

inline Vec4f &Vec4f::operator += (const Vec4f &other)
{
    this->mX += other.mX;
    this->mY += other.mY;
    this->mZ += other.mZ;
    this->mW += other.mW;

    return *this;
}

inline Vec4f &Vec4f::operator -= (const Vec4f &other)
{
    this->mX -= other.mX;
    this->mY -= other.mY;
    this->mZ -= other.mZ;
    this->mW -= other.mW;

    return *this;
}

inline Vec4f &Vec4f::operator *= (const Vec4f &other)
{
    this->mX *= other.mX;
    this->mY *= other.mY;
    this->mZ *= other.mZ;
    this->mW *= other.mW;

    return *this;
}

inline Vec4f Vec4f::MulAdd(const Vec4f &add_value, const Vec4f &mul_a, const Vec4f &mul_b)
{
    Vec4f result;
    result = add_value + (mul_a * mul_b);
    return result;
}

inline float32 Vec4f::GetX() const
{
    return this->mX;
}

inline float32 Vec4f::GetY() const
{
    return this->mY;
}

inline float32 Vec4f::GetZ() const
{
    return this->mZ;
}

inline float32 Vec4f::GetW() const
{
    return this->mW;
}

#endif // #ifdef AR_NO_SIMD
