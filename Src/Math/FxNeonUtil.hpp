#pragma once

#include <Core/FxDefines.hpp>

#ifdef FX_USE_NEON

#include <arm_neon.h>
#include <math.h>

#include <Core/FxTypes.hpp>


typedef float32x4_t float128;

enum FxShuffleComponent
{
    /* A Vector */
    FxShuffle_AX = 0,
    FxShuffle_AY = 1,
    FxShuffle_AZ = 2,
    FxShuffle_AW = 3,

    /* B Vector */
    FxShuffle_BX = 4,
    FxShuffle_BY = 5,
    FxShuffle_BZ = 6,
    FxShuffle_BW = 7,
};

namespace FxNeon {


template <FxShuffleComponent TA, FxShuffleComponent TB, FxShuffleComponent TC, FxShuffleComponent TD>
concept C_SinglePermute = (TA <= FxShuffle_AW) && (TB <= FxShuffle_AW) && (TC <= FxShuffle_AW) && (TD <= FxShuffle_AW);


template <FxShuffleComponent TA, FxShuffleComponent TB, FxShuffleComponent TC, FxShuffleComponent TD>
concept C_SinglePermuteForB = (TA >= FxShuffle_BX) && (TB >= FxShuffle_BX) && (TC >= FxShuffle_BX) &&
                              (TD >= FxShuffle_BX);


FX_FORCE_INLINE inline float32 Length(float32x4_t vec)
{
    // Square the vector
    vec = vmulq_f32(vec, vec);

    // Add all components (horizontal add)
    const float32 len2 = vaddvq_f32(vec);

    return sqrtf(len2);
}

FX_FORCE_INLINE inline float32x4_t Normalize(float32x4_t vec)
{
    // Load vector into register
    const float32x4_t res = vec;

    // Calculate length and splat to register
    const float32x4_t len_v = vdupq_n_f32(Length(vec));

    // Divide vector by length and return
    return vdivq_f32(res, len_v);
}

////////////////////////////////
// Permute/Reorder Functions
////////////////////////////////

/**
 * @brief Returns a single vector rearranged to the template parameter values.
 */
template <FxShuffleComponent TComp1, FxShuffleComponent TComp2, FxShuffleComponent TComp3, FxShuffleComponent TComp4>
FX_FORCE_INLINE inline float32x4_t Permute4(float32x4_t a)
    requires C_SinglePermute<TComp1, TComp2, TComp3, TComp4>
{
    // Unspecialized shuffle, reorder the lanes directly

    float32x4_t result;
    result = vmovq_n_f32(vgetq_lane_f32(a, TComp1 & 0x03));
    result = vsetq_lane_f32(vgetq_lane_f32(a, TComp2 & 0x03), result, 1);
    result = vsetq_lane_f32(vgetq_lane_f32(a, TComp3 & 0x03), result, 2);
    result = vsetq_lane_f32(vgetq_lane_f32(a, TComp4 & 0x03), result, 3);

    return result;
}

// Specializations

template <>
FX_FORCE_INLINE inline float32x4_t Permute4<FxShuffle_AX, FxShuffle_AY, FxShuffle_AZ, FxShuffle_AW>(float32x4_t a)
{
    return a;
}

template <>
FX_FORCE_INLINE inline float32x4_t Permute4<FxShuffle_AX, FxShuffle_AY, FxShuffle_AZ, FxShuffle_AZ>(float32x4_t a)
{
    return vcombine_f32(vget_low_f32(a), vdup_lane_f32(vget_high_f32(a), 0));
}

template <>
FX_FORCE_INLINE inline float32x4_t Permute4<FxShuffle_AX, FxShuffle_AY, FxShuffle_AW, FxShuffle_AW>(float32x4_t a)
{
    return vcombine_f32(vget_low_f32(a), vdup_lane_f32(vget_high_f32(a), 1));
}


template <>
FX_FORCE_INLINE inline float32x4_t Permute4<FxShuffle_AY, FxShuffle_AX, FxShuffle_AW, FxShuffle_AZ>(float32x4_t a)
{
    return vcombine_f32(vrev64_f32(vget_low_f32(a)), vrev64_f32(vget_high_f32(a)));
}

template <>
FX_FORCE_INLINE inline float32x4_t Permute4<FxShuffle_AZ, FxShuffle_AZ, FxShuffle_AY, FxShuffle_AX>(float32x4_t a)
{
    return vcombine_f32(vdup_lane_f32(vget_high_f32(a), 0), vrev64_f32(vget_low_f32(a)));
}

template <>
FX_FORCE_INLINE inline float32x4_t Permute4<FxShuffle_AZ, FxShuffle_AW, FxShuffle_AX, FxShuffle_AY>(float32x4_t a)
{
    return vcombine_f32(vget_high_f32(a), vget_low_f32(a));
}

template <>
FX_FORCE_INLINE inline float32x4_t Permute4<FxShuffle_AY, FxShuffle_AZ, FxShuffle_AX, FxShuffle_AX>(float32x4_t a)
{
    static uint8x16_t table = { 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b,
                                0x00, 0x01, 0x02, 0x03, 0x00, 0x01, 0x02, 0x03 };
    return vreinterpretq_f32_u8(vqtbl1q_u8(vreinterpretq_u8_f32(a), table));
}

/**
 * @brief Returns the vector A rearranged to the components {Y, Z, X, W}.
 */
template <>
FX_FORCE_INLINE inline float32x4_t Permute4<FxShuffle_AY, FxShuffle_AZ, FxShuffle_AX, FxShuffle_AW>(float32x4_t a)
{
    // Extend vector (XYZW -> YZWX)
    a = vextq_f32(a, a, 1);

    // Get the low components (YZ)
    const float32x2_t a_lo = vget_low_f32(a);

    // Reverse high componets of YZWX (WX -> XW)
    const float32x2_t a_hi = vrev64_f32(vget_high_f32(a));

    // Return result YZXW
    return vcombine_f32(a_lo, a_hi);
}

/**
 * @brief Returns a vector containing elements across both vectors A and B, in the order of the template parameters.
 *
 * For example, given a vector A = {1, 2, 3, 4} and another vector B = {5, 6, 7, 8},
 * ```
 * // Returns {1, 6, 3, 8}
 * Permute4<FxShuffle_AX, FxShuffle_BY, FxShuffle_AZ, FxShuffle_BW>(A, B);
 * ```
 *
 */
template <FxShuffleComponent TComp1, FxShuffleComponent TComp2, FxShuffleComponent TComp3, FxShuffleComponent TComp4>
FX_FORCE_INLINE inline float32x4_t Permute4(float32x4_t a, float32x4_t b)
{
    // Unspecialized shuffle, reorder the lanes directly

    float32x4_t result;
    result = vmovq_n_f32(vgetq_lane_f32(TComp1 >= FxShuffle_BX ? b : a, TComp1 & 0x03));
    result = vsetq_lane_f32(vgetq_lane_f32(TComp2 >= FxShuffle_BX ? b : a, TComp2 & 0x03), result, 1);
    result = vsetq_lane_f32(vgetq_lane_f32(TComp3 >= FxShuffle_BX ? b : a, TComp3 & 0x03), result, 2);
    result = vsetq_lane_f32(vgetq_lane_f32(TComp3 >= FxShuffle_BX ? b : a, TComp4 & 0x03), result, 3);

    return result;
}

/**
 * @brief Returns the single permutation of the two vectors A and B, using the components passed in via template
 * parameters.
 */
template <FxShuffleComponent TComp1, FxShuffleComponent TComp2, FxShuffleComponent TComp3, FxShuffleComponent TComp4>
FX_FORCE_INLINE inline float32x4_t Permute4(float32x4_t a, float32x4_t b)
    requires C_SinglePermute<TComp1, TComp2, TComp3, TComp4>
{
    return Permute4<TComp1, TComp2, TComp3, TComp4>(a);
}

/**
 * @brief Returns the vector B shuffled to the order passed in from the template arguments.
 */
template <FxShuffleComponent TComp1, FxShuffleComponent TComp2, FxShuffleComponent TComp3, FxShuffleComponent TComp4>
FX_FORCE_INLINE inline float32x4_t Permute4(float32x4_t a, float32x4_t b)
    requires C_SinglePermuteForB<TComp1, TComp2, TComp3, TComp4>
{
    // Adjust indices for single permutations
    return Permute4<TComp1 - FxShuffle_BX, TComp2 - FxShuffle_BX, TComp3 - FxShuffle_BX, TComp4 - FxShuffle_BX>(b);
}

FX_FORCE_INLINE inline float32x4_t Xor(float32x4_t a, float32x4_t b)
{
    return vreinterpretq_f32_u32(veorq_u32(vreinterpretq_u32_f32(a), vreinterpretq_u32_f32(b)));
}


FX_FORCE_INLINE inline float32x4_t And(float32x4_t a, float32x4_t b)
{
    return vreinterpretq_f32_u32(vandq_u32(vreinterpretq_u32_f32(a), vreinterpretq_u32_f32(b)));
}

// Implementation based on `Vec4::FlipSign` from Jolt physics!
template <int TX, int TY, int TZ, int TW>
FX_FORCE_INLINE inline float32x4_t SetSign(float32x4_t v)
{
    // (this is such a cool idea)
    constexpr float32 signs[4] = {
        TX > 0 ? 0.0f : -0.0f,
        TY > 0 ? 0.0f : -0.0f,
        TZ > 0 ? 0.0f : -0.0f,
        TW > 0 ? 0.0f : -0.0f,
    };

    const float32x4_t sign_v = vld1q_f32(signs);

    return Xor(v, sign_v);
}

void SinCos4(float32x4_t x, float32x4_t* ysin, float32x4_t* ycos);

}; // namespace FxNeon

#endif // FX_USE_NEON
