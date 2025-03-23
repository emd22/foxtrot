#ifdef AR_USE_NEON

#include <Math/Vec3.hpp>
#include <arm_neon.h>

inline Vec3f::Vec3f(float32 x, float32 y, float32 z)
{
    const float32 values[4] = {x, y, z, 0};
    this->mIntrin = vld1q_f32(values);
}

inline Vec3f::Vec3f(float32 scalar)
{
    this->mIntrin = vdupq_n_f32(scalar);
}


inline Vec3f Vec3f::MulAdd(const Vec3f &add_value, const Vec3f &mul_a, const Vec3f &mul_b)
{
    Vec3f result;
    result.mIntrin = vmlaq_f32(add_value, mul_a, mul_b);
    return result;
}

//////////////////////////////
// Operator Overloads
//////////////////////////////

inline Vec3f Vec3f::operator + (const Vec3f &other) const
{
    Vec3f result = Vec3f(this->mIntrin);
    result += other;
    return result;
}

inline Vec3f Vec3f::operator * (const Vec3f &other) const
{
    Vec3f result = Vec3f(this->mIntrin);
    result *= other;
    return result;
}

inline Vec3f &Vec3f::operator += (const Vec3f &other)
{
    this->mIntrin = vaddq_f32(this->mIntrin, other);
    return *this;
}

inline Vec3f &Vec3f::operator -= (const Vec3f &other)
{
    this->mIntrin = vsubq_f32(this->mIntrin, other);
    return *this;
}

inline Vec3f &Vec3f::operator *= (const Vec3f &other)
{
    this->mIntrin = vmulq_f32(this->mIntrin, other);
    return *this;
}

/////////////////////////////////
// Component Getters
/////////////////////////////////

inline float32 Vec3f::GetX() const
{
    return vgetq_lane_f32(this->mIntrin, 0);
}

inline float32 Vec3f::GetY() const
{
    return vgetq_lane_f32(this->mIntrin, 1);
}

inline float32 Vec3f::GetZ() const
{
    return vgetq_lane_f32(this->mIntrin, 2);
}

#endif
