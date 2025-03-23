#pragma once

#ifdef __ARM_NEON__
#define AR_USE_NEON 1
#endif

#if !defined(AR_USE_NEON) && !defined(AR_USE_SSE)
#define AR_NO_SIMD 1
#endif
