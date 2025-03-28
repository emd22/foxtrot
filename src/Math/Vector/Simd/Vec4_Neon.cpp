#ifdef AR_USE_NEON

#include <Math/Vec4.hpp>
#include <arm_neon.h>

inline Vec4f::Vec4f(float32 x, float32 y, float32 z, float32 w)
{
    const float32 values[4] = {x, y, z, w};
    this->mIntrin = vld1q_f32(values);
}

inline Vec4f::Vec4f(float32 scalar)
{
    this->mIntrin = vdupq_n_f32(scalar);
}

inline Vec4f Vec4f::MulAdd(const Vec4f &add_value, const Vec4f &mul_a, const Vec4f &mul_b)
{
    Vec4f result;
    result.mIntrin = vmlaq_f32(add_value, mul_a, mul_b);
    return result;
}

inline Vec4f Vec4f::operator + (const Vec4f &other) const
{
    Vec4f result = Vec4f(this->mIntrin);
    result += other;
    return result;
}

inline Vec4f Vec4f::operator * (const Vec4f &other) const
{
    Vec4f result = Vec4f(this->mIntrin);
    result *= other;
    return result;
}

inline Vec4f &Vec4f::operator += (const Vec4f &other)
{
    this->mIntrin = vaddq_f32(this->mIntrin, other);
    return *this;
}

inline Vec4f &Vec4f::operator -= (const Vec4f &other)
{
    this->mIntrin = vsubq_f32(this->mIntrin, other);
    return *this;
}

inline Vec4f &Vec4f::operator *= (const Vec4f &other)
{
    this->mIntrin = vmulq_f32(this->mIntrin, other);
    return *this;
}


/////////////////////////////////
// Component Getters
/////////////////////////////////

inline float32 Vec4f::GetX() const
{
    // Oddly enough using this->mData[0] seems to compile to less & simpler instructions
    // with clang. Although, this is the correct way to do it, and will likely reduce neon->GP
    // register latency.
    //
    // when accessing mData[i]:
    //
    // ldr s0, [sp, #48 + (ComponentSize * Offset)]
    // str s0, [sp, #44 + (ComponentSize * Offset)]
    //
    // Whereas vgetq_lane_f32:
    //
    // ldr q0, [sp, #48]
    // str q0, [sp, #16] # store back to sp
    // ldr q0, [sp, #16] # load back into q0?
    // str s0, ...
    // # ... operates on s0

    return vgetq_lane_f32(this->mIntrin, 0);
}

inline float32 Vec4f::GetY() const
{
    return vgetq_lane_f32(this->mIntrin, 1);
}

inline float32 Vec4f::GetZ() const
{
    return vgetq_lane_f32(this->mIntrin, 2);
}

inline float32 Vec4f::GetW() const
{
    return vgetq_lane_f32(this->mIntrin, 3);
}

#endif // #ifdef AR_USE_NEON
