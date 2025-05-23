#pragma once

#include <Core/Types.hpp>

#ifdef FX_USE_NEON
#include <arm_neon.h>
#endif

class Vec3f {
public:
    Vec3f() = default;
    Vec3f(float32 x, float32 y, float32 z);
    explicit Vec3f(float32 scalar);

    void Set(float32 x, float32 y, float32 z);

    Vec3f operator + (const Vec3f &other) const;
    Vec3f operator - (const Vec3f &other) const;
    Vec3f operator * (const Vec3f &other) const;
    Vec3f operator * (float32 scalar) const;

    Vec3f operator - () const;

    Vec3f &operator += (const Vec3f &other);
    Vec3f &operator -= (const Vec3f &other);
    Vec3f &operator *= (const Vec3f &other);

    static Vec3f MulAdd(const Vec3f &add_value, const Vec3f &mul_a, const Vec3f &mul_b);

    Vec3f Normalize() const;
    float32 Length() const;
    Vec3f Cross(const Vec3f &other) const;

    float32 Dot(const Vec3f &other) const;

    float32 GetX() const;
    float32 GetY() const;
    float32 GetZ() const;

    static const Vec3f Zero;
    static const Vec3f Up;

#ifdef FX_USE_NEON
    explicit Vec3f(float32x4_t intrin)
        : mIntrin(intrin)
    {
    }

    Vec3f &operator = (const float32x4_t &other)
    {
        mIntrin = other;
        return *this;
    }

    operator float32x4_t() const
    {
        return mIntrin;
    }
#endif

public:
#if defined(FX_USE_NEON)
    union alignas(16) {
        float32x4_t mIntrin;
        float mData[4];

        struct { float32 X, Y, Z, mPadding0; };
    };

#else
    struct { float32 X, Y, Z; };
#endif

};
