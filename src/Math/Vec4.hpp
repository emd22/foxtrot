#pragma once

#include <Core/Types.hpp>

#ifdef FX_USE_NEON
#include <arm_neon.h>
#endif

class alignas(16) Vec4f
{
public:
    friend class Mat4f;

    Vec4f() = default;
    Vec4f(float32 x, float32 y, float32 z, float32 w);
    explicit Vec4f(float32 scalar);

    Vec4f operator + (const Vec4f &other) const;
    Vec4f operator * (const Vec4f &other) const;

    Vec4f &operator += (const Vec4f &other);
    Vec4f &operator -= (const Vec4f &other);
    Vec4f &operator *= (const Vec4f &other);

    Vec4f &operator = (const Vec4f &other);

    static Vec4f MulAdd(const Vec4f &add_value, const Vec4f &mul_a, const Vec4f &mul_b);

    float32 GetX() const;
    float32 GetY() const;
    float32 GetZ() const;
    float32 GetW() const;

    void SetX(float32 x);
    void SetY(float32 y);
    void SetZ(float32 z);
    void SetW(float32 w);

    /**
     * Loads 4 values into the vector.
     */
    void Load4(float32 x, float32 y, float32 z, float32 w);

    /**
     * Loads 4 values into the vector from a pointer.
     */
    void Load4Ptr(float32 *values);

    /**
     * Loads a single value into the vector to all components.
     */
    void Load1(float32 scalar);

    static const Vec4f Zero()
    {
        return Vec4f(0.0f, 0.0f, 0.0f, 0.0f);
    };

#ifdef FX_USE_NEON
    explicit Vec4f(float32x4_t intrin)
        : mIntrin(intrin)
    {
    }

    Vec4f &operator = (const float32x4_t &other)
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
        float32 mData[4];
        struct { float32 X, Y, Z, W; };
    };

#else
    float32 X, Y, Z, W;
#endif
};
