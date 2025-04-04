#pragma once

#include <Core/Types.hpp>

#ifdef AR_USE_NEON
#include <arm_neon.h>
#endif

class Vec4f {
public:
    Vec4f() = default;
    Vec4f(float32 x, float32 y, float32 z, float32 w);
    explicit Vec4f(float32 scalar);

    Vec4f operator + (const Vec4f &other) const;
    Vec4f operator * (const Vec4f &other) const;

    Vec4f &operator += (const Vec4f &other);
    Vec4f &operator -= (const Vec4f &other);
    Vec4f &operator *= (const Vec4f &other);

    static Vec4f MulAdd(const Vec4f &add_value, const Vec4f &mul_a, const Vec4f &mul_b);

    float32 GetX() const;
    float32 GetY() const;
    float32 GetZ() const;
    float32 GetW() const;

    static const Vec4f Zero()
    {
        return Vec4f(0.0f, 0.0f, 0.0f, 0.0f);
    };

#ifdef AR_USE_NEON
    explicit Vec4f(float32x4_t intrin)
        : mIntrin(intrin)
    {
    }

    operator float32x4_t() const
    {
        return this->mIntrin;
    }
#endif

private:
#if defined(AR_USE_NEON)
    union alignas(16) {
        float32x4_t mIntrin;
        float32 mData[4];
    };

#else
    float32 mX, mY, mZ, mW;
#endif
};
