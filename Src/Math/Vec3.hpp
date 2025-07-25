#pragma once

#include <Core/Types.hpp>

#ifdef FX_USE_NEON
#include <arm_neon.h>
#endif

class FxVec3f {
public:
    FxVec3f() = default;
    FxVec3f(float32 x, float32 y, float32 z);
    explicit FxVec3f(float32 scalar);

    void Set(float32 x, float32 y, float32 z);

    FxVec3f operator + (const FxVec3f &other) const;
    FxVec3f operator - (const FxVec3f &other) const;
    FxVec3f operator * (const FxVec3f &other) const;
    FxVec3f operator * (float32 scalar) const;
    FxVec3f operator / (float32 scalar) const;

    FxVec3f operator - () const;

    FxVec3f &operator += (const FxVec3f &other);
    FxVec3f &operator -= (const FxVec3f &other);
    FxVec3f &operator *= (const FxVec3f &other);

    static FxVec3f MulAdd(const FxVec3f &add_value, const FxVec3f &mul_a, const FxVec3f &mul_b);

    FX_FORCE_INLINE FxVec3f Normalize() const;
    FX_FORCE_INLINE void NormalizeIP();

    FX_FORCE_INLINE float32 Length() const;
    FxVec3f Cross(const FxVec3f &other) const;

    float32 Dot(const FxVec3f &other) const;

    static const FxVec3f Zero;
    static const FxVec3f One;
    static const FxVec3f Up;

#ifdef FX_USE_NEON
    explicit FxVec3f(float32x4_t intrin)
        : mIntrin(intrin)
    {
    }

    FxVec3f& operator = (const float32x4_t &other)
    {
        mIntrin = other;
        return *this;
    }

    FxVec3f& operator = (const float32x4_t other)
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
