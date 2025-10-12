#pragma once

#include <Math/FxQuat.hpp>

#ifdef FX_USE_NEON

FX_FORCE_INLINE FxQuat& FxQuat::operator=(const float32x4_t& other)
{
    mIntrin = other;
    return *this;
}

FX_FORCE_INLINE bool FxQuat::IsCloseTo(const FxQuat& other, const float32 tolerance) const
{
    return IsCloseTo(other.mIntrin, tolerance);
}

FX_FORCE_INLINE bool FxQuat::IsCloseTo(const float32x4_t& other, const float32 tolerance) const
{
    // Get the absolute difference between the vectors
    const float32x4_t diff = vabdq_f32(mIntrin, other);

    // Is the difference greater than our threshhold?
    const uint32x4_t lt = vcgtq_f32(diff, vdupq_n_f32(tolerance));

    // If any components are true, return false.
    return vmaxvq_u32(lt) == 0;
}

#endif
