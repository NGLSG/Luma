#include "SIMDWrapper.h"
#include <array>
#include <cfloat>
#include <cmath>


#if defined(LUMA_X64)
#ifdef _WIN32
#include <intrin.h>
#else
#include <x86intrin.h>
#endif

namespace
{
    class CPUInfo
    {
    public:
        CPUInfo()
        {
            std::array<int, 4> regs;
            cpuid(regs, 1);
            f_1_ECX_ = regs[2];

            cpuidex(regs, 7, 0);
            f_7_EBX_ = regs[1];
            f_7_ECX_ = regs[2];
        }

        bool IsSSE42Supported() const { return (f_1_ECX_ & (1 << 20)) != 0; }

        bool IsAVXSupported() const
        {
            bool avx = (f_1_ECX_ & (1 << 28)) != 0;
            bool osxsave = (f_1_ECX_ & (1 << 27)) != 0;
            if (osxsave && avx)
            {
                unsigned long long xcr0 = _xgetbv(0);
                return (xcr0 & 0x6) == 0x6;
            }
            return false;
        }

        bool IsAVX2Supported() const { return (f_7_EBX_ & (1 << 5)) != 0; }

        bool IsAVX512FSupported() const
        {
            bool avx512f = (f_7_EBX_ & (1 << 16)) != 0;
            if (avx512f)
            {
                unsigned long long xcr0 = _xgetbv(0);
                return (xcr0 & 0xE6) == 0xE6;
            }
            return false;
        }

    private:
        void cpuid(std::array<int, 4>& regs, int level)
        {
#if defined(_MSC_VER)
            __cpuid(regs.data(), level);
#elif defined(__GNUC__) || defined(__clang__)
            cpuid(regs, level);
#endif
        }

        void cpuidex(std::array<int, 4>& regs, int level, int count)
        {
#if defined(_MSC_VER)
            __cpuidex(regs.data(), level, count);
#elif defined(__GNUC__) || defined(__clang__)
            cpuidex(regs, level, count);
#endif
        }

        int f_1_ECX_ = 0;
        int f_7_EBX_ = 0;
        int f_7_ECX_ = 0;
    };

    static const CPUInfo g_cpu_info;
}
#endif

#if defined(LUMA_X64)
class SSE42 : public SIMDIpml
{
public:
    void VectorAdd(const float* a, const float* b, float* result, size_t count) override
    {
        size_t i = 0;
        for (; i + 4 <= count; i += 4)
        {
            _mm_storeu_ps(result + i, _mm_add_ps(_mm_loadu_ps(a + i), _mm_loadu_ps(b + i)));
        }
        for (; i < count; ++i) result[i] = a[i] + b[i];
    }

    void VectorMultiply(const float* a, const float* b, float* result, size_t count) override
    {
        size_t i = 0;
        for (; i + 4 <= count; i += 4)
        {
            _mm_storeu_ps(result + i, _mm_mul_ps(_mm_loadu_ps(a + i), _mm_loadu_ps(b + i)));
        }
        for (; i < count; ++i) result[i] = a[i] * b[i];
    }

    void VectorScalarMultiply(const float* a, float b, float* result, size_t count) override
    {
        size_t i = 0;
        __m128 bv = _mm_set1_ps(b);
        for (; i + 4 <= count; i += 4)
        {
            _mm_storeu_ps(result + i, _mm_mul_ps(_mm_loadu_ps(a + i), bv));
        }
        for (; i < count; ++i) result[i] = a[i] * b;
    }

    void VectorMultiplyAdd(const float* a, const float* b, const float* c, float* result, size_t count) override
    {
        size_t i = 0;
        for (; i + 4 <= count; i += 4)
        {
            _mm_storeu_ps(result + i, _mm_add_ps(_mm_mul_ps(_mm_loadu_ps(a + i), _mm_loadu_ps(b + i)),
                                                 _mm_loadu_ps(c + i)));
        }
        for (; i < count; ++i) result[i] = a[i] * b[i] + c[i];
    }

    float VectorDotProduct(const float* a, const float* b, size_t count) override
    {
        __m128 sum = _mm_setzero_ps();
        size_t i = 0;
        for (; i + 4 <= count; i += 4)
        {
            sum = _mm_add_ps(sum, _mm_mul_ps(_mm_loadu_ps(a + i), _mm_loadu_ps(b + i)));
        }
        float temp[4];
        _mm_storeu_ps(temp, sum);
        float result = temp[0] + temp[1] + temp[2] + temp[3];
        for (; i < count; ++i) result += a[i] * b[i];
        return result;
    }

    void VectorSqrt(const float* input, float* result, size_t count) override
    {
        size_t i = 0;
        for (; i + 4 <= count; i += 4)
        {
            _mm_storeu_ps(result + i, _mm_sqrt_ps(_mm_loadu_ps(input + i)));
        }
        for (; i < count; ++i) result[i] = sqrtf(input[i]);
    }

    void VectorReciprocal(const float* input, float* result, size_t count) override
    {
        size_t i = 0;
        for (; i + 4 <= count; i += 4)
        {
            _mm_storeu_ps(result + i, _mm_rcp_ps(_mm_loadu_ps(input + i)));
        }
        for (; i < count; ++i) result[i] = 1.0f / input[i];
    }

    void VectorRotatePoints(const float* points_x, const float* points_y, const float* sin_vals, const float* cos_vals,
                            float* result_x, float* result_y, size_t count) override
    {
        size_t i = 0;
        for (; i + 4 <= count; i += 4)
        {
            __m128 px = _mm_loadu_ps(points_x + i);
            __m128 py = _mm_loadu_ps(points_y + i);
            __m128 sv = _mm_loadu_ps(sin_vals + i);
            __m128 cv = _mm_loadu_ps(cos_vals + i);
            __m128 rx = _mm_sub_ps(_mm_mul_ps(px, cv), _mm_mul_ps(py, sv));
            __m128 ry = _mm_add_ps(_mm_mul_ps(px, sv), _mm_mul_ps(py, cv));
            _mm_storeu_ps(result_x + i, rx);
            _mm_storeu_ps(result_y + i, ry);
        }
        for (; i < count; ++i)
        {
            result_x[i] = points_x[i] * cos_vals[i] - points_y[i] * sin_vals[i];
            result_y[i] = points_x[i] * sin_vals[i] + points_y[i] * cos_vals[i];
        }
    }

    float VectorMax(const float* input, size_t count) override
    {
        if (count == 0) return -FLT_MAX;
        size_t i = 0;
        __m128 maxVec = _mm_set1_ps(input[0]);
        for (; i + 4 <= count; i += 4)
        {
            maxVec = _mm_max_ps(maxVec, _mm_loadu_ps(input + i));
        }
        float temp[4];
        _mm_storeu_ps(temp, maxVec);
        float result = std::max({temp[0], temp[1], temp[2], temp[3]});
        for (; i < count; ++i) result = std::max(result, input[i]);
        return result;
    }

    float VectorMin(const float* input, size_t count) override
    {
        if (count == 0) return FLT_MAX;
        size_t i = 0;
        __m128 minVec = _mm_set1_ps(input[0]);
        for (; i + 4 <= count; i += 4)
        {
            minVec = _mm_min_ps(minVec, _mm_loadu_ps(input + i));
        }
        float temp[4];
        _mm_storeu_ps(temp, minVec);
        float result = std::min({temp[0], temp[1], temp[2], temp[3]});
        for (; i < count; ++i) result = std::min(result, input[i]);
        return result;
    }

    void VectorAbs(const float* input, float* result, size_t count) override
    {
        size_t i = 0;
        __m128 signMask = _mm_set1_ps(-0.0f);
        for (; i + 4 <= count; i += 4)
        {
            _mm_storeu_ps(result + i, _mm_andnot_ps(signMask, _mm_loadu_ps(input + i)));
        }
        for (; i < count; ++i) result[i] = fabsf(input[i]);
    }

    const char* GetSupportedInstructions() const override { return "SSE4.2"; }
};

#if defined(LUMA_AVX) || defined(LUMA_AVX2)
class AVX : public SIMDIpml
{
public:
    void VectorAdd(const float* a, const float* b, float* result, size_t count) override
    {
        size_t i = 0;
        for (; i + 8 <= count; i += 8)
        {
            _mm256_storeu_ps(result + i, _mm256_add_ps(_mm256_loadu_ps(a + i), _mm256_loadu_ps(b + i)));
        }
        SSE42 sse_fallback;
        sse_fallback.VectorAdd(a + i, b, result + i, count - i);
    }

    void VectorMultiply(const float* a, const float* b, float* result, size_t count) override
    {
        size_t i = 0;
        for (; i + 8 <= count; i += 8)
        {
            _mm256_storeu_ps(result + i, _mm256_mul_ps(_mm256_loadu_ps(a + i), _mm256_loadu_ps(b + i)));
        }
        SSE42 sse_fallback;
        sse_fallback.VectorMultiply(a + i, b, result + i, count - i);
    }

    void VectorScalarMultiply(const float* a, float b, float* result, size_t count) override
    {
        size_t i = 0;
        __m256 bv = _mm256_set1_ps(b);
        for (; i + 8 <= count; i += 8)
        {
            _mm256_storeu_ps(result + i, _mm256_mul_ps(_mm256_loadu_ps(a + i), bv));
        }
        SSE42 sse_fallback;
        sse_fallback.VectorScalarMultiply(a + i, b, result + i, count - i);
    }

    void VectorMultiplyAdd(const float* a, const float* b, const float* c, float* result, size_t count) override
    {
        size_t i = 0;
        for (; i + 8 <= count; i += 8)
        {
            _mm256_storeu_ps(result + i, _mm256_add_ps(_mm256_mul_ps(_mm256_loadu_ps(a + i), _mm256_loadu_ps(b + i)),
                                                       _mm256_loadu_ps(c + i)));
        }
        SSE42 sse_fallback;
        sse_fallback.VectorMultiplyAdd(a + i, b, c, result + i, count - i);
    }

    float VectorDotProduct(const float* a, const float* b, size_t count) override
    {
        __m256 sum = _mm256_setzero_ps();
        size_t i = 0;
        for (; i + 8 <= count; i += 8)
        {
            sum = _mm256_add_ps(sum, _mm256_mul_ps(_mm256_loadu_ps(a + i), _mm256_loadu_ps(b + i)));
        }
        float temp[8];
        _mm256_storeu_ps(temp, sum);
        float result = temp[0] + temp[1] + temp[2] + temp[3] + temp[4] + temp[5] + temp[6] + temp[7];

        SSE42 sse_fallback;
        result += sse_fallback.VectorDotProduct(a + i, b, count - i);
        return result;
    }

    void VectorSqrt(const float* input, float* result, size_t count) override
    {
        size_t i = 0;
        for (; i + 8 <= count; i += 8)
        {
            _mm256_storeu_ps(result + i, _mm256_sqrt_ps(_mm256_loadu_ps(input + i)));
        }
        SSE42 sse_fallback;
        sse_fallback.VectorSqrt(input + i, result + i, count - i);
    }

    void VectorReciprocal(const float* input, float* result, size_t count) override
    {
        size_t i = 0;
        for (; i + 8 <= count; i += 8)
        {
            _mm256_storeu_ps(result + i, _mm256_rcp_ps(_mm256_loadu_ps(input + i)));
        }
        SSE42 sse_fallback;
        sse_fallback.VectorReciprocal(input + i, result + i, count - i);
    }

    void VectorRotatePoints(const float* points_x, const float* points_y, const float* sin_vals, const float* cos_vals,
                            float* result_x, float* result_y, size_t count) override
    {
        size_t i = 0;
        for (; i + 8 <= count; i += 8)
        {
            __m256 px = _mm256_loadu_ps(points_x + i);
            __m256 py = _mm256_loadu_ps(points_y + i);
            __m256 sv = _mm256_loadu_ps(sin_vals + i);
            __m256 cv = _mm256_loadu_ps(cos_vals + i);
            __m256 rx = _mm256_sub_ps(_mm256_mul_ps(px, cv), _mm256_mul_ps(py, sv));
            __m256 ry = _mm256_add_ps(_mm256_mul_ps(px, sv), _mm256_mul_ps(py, cv));
            _mm256_storeu_ps(result_x + i, rx);
            _mm256_storeu_ps(result_y + i, ry);
        }
        SSE42 sse_fallback;
        sse_fallback.VectorRotatePoints(points_x + i, points_y + i, sin_vals + i, cos_vals + i, result_x + i,
                                        result_y + i, count - i);
    }

    float VectorMax(const float* input, size_t count) override
    {
        if (count == 0) return -FLT_MAX;
        size_t i = 0;
        __m256 maxVec = _mm256_set1_ps(input[0]);
        for (; i + 8 <= count; i += 8)
        {
            maxVec = _mm256_max_ps(maxVec, _mm256_loadu_ps(input + i));
        }
        float temp[8];
        _mm256_storeu_ps(temp, maxVec);
        float result = temp[0];
        for (int j = 1; j < 8; ++j) result = std::max(result, temp[j]);

        SSE42 sse_fallback;
        result = std::max(result, sse_fallback.VectorMax(input + i, count - i));
        return result;
    }

    float VectorMin(const float* input, size_t count) override
    {
        if (count == 0) return FLT_MAX;
        size_t i = 0;
        __m256 minVec = _mm256_set1_ps(input[0]);
        for (; i + 8 <= count; i += 8)
        {
            minVec = _mm256_min_ps(minVec, _mm256_loadu_ps(input + i));
        }
        float temp[8];
        _mm256_storeu_ps(temp, minVec);
        float result = temp[0];
        for (int j = 1; j < 8; ++j) result = std::min(result, temp[j]);

        SSE42 sse_fallback;
        result = std::min(result, sse_fallback.VectorMin(input + i, count - i));
        return result;
    }

    void VectorAbs(const float* input, float* result, size_t count) override
    {
        size_t i = 0;
        __m256 signMask = _mm256_set1_ps(-0.0f);
        for (; i + 8 <= count; i += 8)
        {
            _mm256_storeu_ps(result + i, _mm256_andnot_ps(signMask, _mm256_loadu_ps(input + i)));
        }
        SSE42 sse_fallback;
        sse_fallback.VectorAbs(input + i, result + i, count - i);
    }

    const char* GetSupportedInstructions() const override { return "AVX"; }
};
#endif

#if defined(LUMA_AVX2)
class AVX2 : public AVX
{
public:
    void VectorMultiplyAdd(const float* a, const float* b, const float* c, float* result, size_t count) override
    {
        size_t i = 0;
        for (; i + 8 <= count; i += 8)
        {
            _mm256_storeu_ps(result + i, _mm256_fmadd_ps(_mm256_loadu_ps(a + i), _mm256_loadu_ps(b + i),
                                                         _mm256_loadu_ps(c + i)));
        }
        SSE42 sse_fallback;
        sse_fallback.VectorMultiplyAdd(a + i, b, c, result + i, count - i);
    }

    float VectorDotProduct(const float* a, const float* b, size_t count) override
    {
        __m256 sum = _mm256_setzero_ps();
        size_t i = 0;
        for (; i + 8 <= count; i += 8)
        {
            sum = _mm256_fmadd_ps(_mm256_loadu_ps(a + i), _mm256_loadu_ps(b + i), sum);
        }
        float temp[8];
        _mm256_storeu_ps(temp, sum);
        float result = temp[0] + temp[1] + temp[2] + temp[3] + temp[4] + temp[5] + temp[6] + temp[7];

        SSE42 sse_fallback;
        result += sse_fallback.VectorDotProduct(a + i, b, count - i);
        return result;
    }

    const char* GetSupportedInstructions() const override { return "AVX2"; }
};
#endif

#if defined(LUMA_AVX512)
class AVX512 : public SIMDIpml
{
public:
    void VectorAdd(const float* a, const float* b, float* result, size_t count) override
    {
        size_t i = 0;
        for (; i + 16 <= count; i += 16)
        {
            _mm512_storeu_ps(result + i, _mm512_add_ps(_mm512_loadu_ps(a + i), _mm512_loadu_ps(b + i)));
        }
        AVX2 avx2_fallback;
        avx2_fallback.VectorAdd(a + i, b, result + i, count - i);
    }

    void VectorMultiply(const float* a, const float* b, float* result, size_t count) override
    {
        size_t i = 0;
        for (; i + 16 <= count; i += 16)
        {
            _mm512_storeu_ps(result + i, _mm512_mul_ps(_mm512_loadu_ps(a + i), _mm512_loadu_ps(b + i)));
        }
        AVX2 avx2_fallback;
        avx2_fallback.VectorMultiply(a + i, b, result + i, count - i);
    }

    void VectorScalarMultiply(const float* a, float b, float* result, size_t count) override
    {
        size_t i = 0;
        __m512 bv = _mm512_set1_ps(b);
        for (; i + 16 <= count; i += 16)
        {
            _mm512_storeu_ps(result + i, _mm512_mul_ps(_mm512_loadu_ps(a + i), bv));
        }
        AVX2 avx2_fallback;
        avx2_fallback.VectorScalarMultiply(a + i, b, result + i, count - i);
    }

    void VectorMultiplyAdd(const float* a, const float* b, const float* c, float* result, size_t count) override
    {
        size_t i = 0;
        for (; i + 16 <= count; i += 16)
        {
            _mm512_storeu_ps(result + i, _mm512_fmadd_ps(_mm512_loadu_ps(a + i), _mm512_loadu_ps(b + i),
                                                         _mm512_loadu_ps(c + i)));
        }
        AVX2 avx2_fallback;
        avx2_fallback.VectorMultiplyAdd(a + i, b, c, result + i, count - i);
    }

    float VectorDotProduct(const float* a, const float* b, size_t count) override
    {
        __m512 sumVec = _mm512_setzero_ps();
        size_t i = 0;
        for (; i + 16 <= count; i += 16)
        {
            sumVec = _mm512_fmadd_ps(_mm512_loadu_ps(a + i), _mm512_loadu_ps(b + i), sumVec);
        }
        float result = _mm512_reduce_add_ps(sumVec);
        AVX2 avx2_fallback;
        result += avx2_fallback.VectorDotProduct(a + i, b, count - i);
        return result;
    }

    void VectorSqrt(const float* input, float* result, size_t count) override
    {
        size_t i = 0;
        for (; i + 16 <= count; i += 16)
        {
            _mm512_storeu_ps(result + i, _mm512_sqrt_ps(_mm512_loadu_ps(input + i)));
        }
        AVX2 avx2_fallback;
        avx2_fallback.VectorSqrt(input + i, result + i, count - i);
    }

    void VectorReciprocal(const float* input, float* result, size_t count) override
    {
        size_t i = 0;
        for (; i + 16 <= count; i += 16)
        {
            _mm512_storeu_ps(result + i, _mm512_rcp14_ps(_mm512_loadu_ps(input + i)));
        }
        AVX2 avx2_fallback;
        avx2_fallback.VectorReciprocal(input + i, result + i, count - i);
    }

    void VectorRotatePoints(const float* points_x, const float* points_y, const float* sin_vals, const float* cos_vals,
                            float* result_x, float* result_y, size_t count) override
    {
        size_t i = 0;
        for (; i + 16 <= count; i += 16)
        {
            __m512 px = _mm512_loadu_ps(points_x + i);
            __m512 py = _mm512_loadu_ps(points_y + i);
            __m512 sv = _mm512_loadu_ps(sin_vals + i);
            __m512 cv = _mm512_loadu_ps(cos_vals + i);
            __m512 rx = _mm512_sub_ps(_mm512_mul_ps(px, cv), _mm512_mul_ps(py, sv));
            __m512 ry = _mm512_add_ps(_mm512_mul_ps(px, sv), _mm512_mul_ps(py, cv));
            _mm512_storeu_ps(result_x + i, rx);
            _mm512_storeu_ps(result_y + i, ry);
        }
        AVX2 avx2_fallback;
        avx2_fallback.VectorRotatePoints(points_x + i, points_y + i, sin_vals + i, cos_vals + i, result_x + i,
                                         result_y + i, count - i);
    }

    float VectorMax(const float* input, size_t count) override
    {
        if (count == 0) return -FLT_MAX;
        __m512 maxVec = _mm512_set1_ps(-FLT_MAX);
        size_t i = 0;
        for (; i + 16 <= count; i += 16)
        {
            maxVec = _mm512_max_ps(maxVec, _mm512_loadu_ps(input + i));
        }
        float result = _mm512_reduce_max_ps(maxVec);
        AVX2 avx2_fallback;
        result = std::max(result, avx2_fallback.VectorMax(input + i, count - i));
        return result;
    }

    float VectorMin(const float* input, size_t count) override
    {
        if (count == 0) return FLT_MAX;
        __m512 minVec = _mm512_set1_ps(FLT_MAX);
        size_t i = 0;
        for (; i + 16 <= count; i += 16)
        {
            minVec = _mm512_min_ps(minVec, _mm512_loadu_ps(input + i));
        }
        float result = _mm512_reduce_min_ps(minVec);
        AVX2 avx2_fallback;
        result = std::min(result, avx2_fallback.VectorMin(input + i, count - i));
        return result;
    }

    void VectorAbs(const float* input, float* result, size_t count) override
    {
        size_t i = 0;
        __m512 signMask = _mm512_set1_ps(-0.0f);
        for (; i + 16 <= count; i += 16)
        {
            _mm512_storeu_ps(result + i, _mm512_andnot_ps(signMask, _mm512_loadu_ps(input + i)));
        }
        AVX2 avx2_fallback;
        avx2_fallback.VectorAbs(input + i, result + i, count - i);
    }

    const char* GetSupportedInstructions() const override { return "AVX512"; }
};
#endif

#elif defined(LUMA_ARM64) && defined(LUMA_NEON)
#include <arm_neon.h>

class NEON : public SIMDIpml
{
public:
    void VectorAdd(const float* a, const float* b, float* result, size_t count) override
    {
        size_t i = 0;
        for (; i + 4 <= count; i += 4)
        {
            vst1q_f32(result + i, vaddq_f32(vld1q_f32(a + i), vld1q_f32(b + i)));
        }
        for (; i < count; ++i) result[i] = a[i] + b[i];
    }

    void VectorMultiply(const float* a, const float* b, float* result, size_t count) override
    {
        size_t i = 0;
        for (; i + 4 <= count; i += 4)
        {
            vst1q_f32(result + i, vmulq_f32(vld1q_f32(a + i), vld1q_f32(b + i)));
        }
        for (; i < count; ++i) result[i] = a[i] * b[i];
    }

    void VectorScalarMultiply(const float* a, float b, float* result, size_t count) override
    {
        size_t i = 0;
        float32x4_t bv = vdupq_n_f32(b);
        for (; i + 4 <= count; i += 4)
        {
            vst1q_f32(result + i, vmulq_f32(vld1q_f32(a + i), bv));
        }
        for (; i < count; ++i) result[i] = a[i] * b;
    }

    void VectorMultiplyAdd(const float* a, const float* b, const float* c, float* result, size_t count) override
    {
        size_t i = 0;
        for (; i + 4 <= count; i += 4)
        {
            vst1q_f32(result + i, vfmaq_f32(vld1q_f32(c + i), vld1q_f32(a + i), vld1q_f32(b + i)));
        }
        for (; i < count; ++i) result[i] = a[i] * b[i] + c[i];
    }

    float VectorDotProduct(const float* a, const float* b, size_t count) override
    {
        float32x4_t sumVec = vdupq_n_f32(0.0f);
        size_t i = 0;
        for (; i + 4 <= count; i += 4)
        {
            sumVec = vfmaq_f32(sumVec, vld1q_f32(a + i), vld1q_f32(b + i));
        }
        float result = vaddvq_f32(sumVec);
        for (; i < count; ++i) result += a[i] * b[i];
        return result;
    }

    void VectorSqrt(const float* input, float* result, size_t count) override
    {
        size_t i = 0;
        for (; i + 4 <= count; i += 4)
        {
            vst1q_f32(result + i, vsqrtq_f32(vld1q_f32(input + i)));
        }
        for (; i < count; ++i) result[i] = sqrtf(input[i]);
    }

    void VectorReciprocal(const float* input, float* result, size_t count) override
    {
        size_t i = 0;
        for (; i + 4 <= count; i += 4)
        {
            vst1q_f32(result + i, vrecpeq_f32(vld1q_f32(input + i)));
        }
        for (; i < count; ++i) result[i] = 1.0f / input[i];
    }

    void VectorRotatePoints(const float* points_x, const float* points_y, const float* sin_vals, const float* cos_vals,
                            float* result_x, float* result_y, size_t count) override
    {
        size_t i = 0;
        for (; i + 4 <= count; i += 4)
        {
            float32x4_t px = vld1q_f32(points_x + i);
            float32x4_t py = vld1q_f32(points_y + i);
            float32x4_t sv = vld1q_f32(sin_vals + i);
            float32x4_t cv = vld1q_f32(cos_vals + i);
            float32x4_t rx = vsubq_f32(vmulq_f32(px, cv), vmulq_f32(py, sv));
            float32x4_t ry = vaddq_f32(vmulq_f32(px, sv), vmulq_f32(py, cv));
            vst1q_f32(result_x + i, rx);
            vst1q_f32(result_y + i, ry);
        }
        for (; i < count; ++i)
        {
            result_x[i] = points_x[i] * cos_vals[i] - points_y[i] * sin_vals[i];
            result_y[i] = points_x[i] * sin_vals[i] + points_y[i] * cos_vals[i];
        }
    }

    float VectorMax(const float* input, size_t count) override
    {
        if (count == 0) return -FLT_MAX;
        float32x4_t maxVec = vdupq_n_f32(input[0]);
        size_t i = 0;
        for (; i + 4 <= count; i += 4)
        {
            maxVec = vmaxq_f32(maxVec, vld1q_f32(input + i));
        }
        float result = vmaxvq_f32(maxVec);
        for (; i < count; ++i) result = std::max(result, input[i]);
        return result;
    }

    float VectorMin(const float* input, size_t count) override
    {
        if (count == 0) return FLT_MAX;
        float32x4_t minVec = vdupq_n_f32(input[0]);
        size_t i = 0;
        for (; i + 4 <= count; i += 4)
        {
            minVec = vminq_f32(minVec, vld1q_f32(input + i));
        }
        float result = vminvq_f32(minVec);
        for (; i < count; ++i) result = std::min(result, input[i]);
        return result;
    }

    void VectorAbs(const float* input, float* result, size_t count) override
    {
        size_t i = 0;
        for (; i + 4 <= count; i += 4)
        {
            vst1q_f32(result + i, vabsq_f32(vld1q_f32(input + i)));
        }
        for (; i < count; ++i) result[i] = fabsf(input[i]);
    }

    const char* GetSupportedInstructions() const override { return "NEON"; }
};
#endif

SIMD::SIMD()
{
#if defined(LUMA_X64)
#ifdef  LUMA_AVX512
    if (g_cpu_info.IsAVX512FSupported())
    {
        impl = std::make_unique<AVX512>();
        return;
    }
#elif  defined(LUMA_AVX2)
    if (g_cpu_info.IsAVX2Supported())
    {
        impl = std::make_unique<AVX2>();
        return;
    }
#elif defined (LUMA_AVX)|| defined (LUMA_AVX2)
    if (g_cpu_info.IsAVXSupported())
    {
        impl = std::make_unique<AVX>();
        return;
    }
#elif defined(LUMA_AVX) || defined(LUMA_AVX2) || defined(LUMA_AVX512) || defined(LUMA_SSE42)
    if (g_cpu_info.IsSSE42Supported())
    {
        impl = std::make_unique<SSE42>();
        return;
    }
#endif
#elif defined(LUMA_ARM64) && defined(LUMA_NEON)
    impl = std::make_unique<NEON>();
    return;
#endif
}

void SIMD::VectorAdd(const float* a, const float* b, float* result, size_t count)
{
    if (impl) impl->VectorAdd(a, b, result, count);
}

void SIMD::VectorMultiply(const float* a, const float* b, float* result, size_t count)
{
    if (impl) impl->VectorMultiply(a, b, result, count);
}

void SIMD::VectorScalarMultiply(const float* a, float b, float* result, size_t count)
{
    if (impl) impl->VectorScalarMultiply(a, b, result, count);
}

void SIMD::VectorMultiplyAdd(const float* a, const float* b, const float* c, float* result, size_t count)
{
    if (impl) impl->VectorMultiplyAdd(a, b, c, result, count);
}

void SIMD::VectorRotatePoints(const float* points_x, const float* points_y, const float* sin_vals,
                              const float* cos_vals, float* result_x, float* result_y, size_t count)
{
    if (impl) impl->VectorRotatePoints(points_x, points_y, sin_vals, cos_vals, result_x, result_y, count);
}

float SIMD::VectorDotProduct(const float* a, const float* b, size_t count)
{
    if (count > 0 && impl) return impl->VectorDotProduct(a, b, count);
    return 0.0f;
}

void SIMD::VectorSqrt(const float* input, float* result, size_t count)
{
    if (impl) impl->VectorSqrt(input, result, count);
}

void SIMD::VectorReciprocal(const float* input, float* result, size_t count)
{
    if (impl) impl->VectorReciprocal(input, result, count);
}

float SIMD::VectorMax(const float* input, size_t count)
{
    if (count > 0 && impl) return impl->VectorMax(input, count);
    return -FLT_MAX;
}

float SIMD::VectorMin(const float* input, size_t count)
{
    if (count > 0 && impl) return impl->VectorMin(input, count);
    return FLT_MAX;
}

void SIMD::VectorAbs(const float* input, float* result, size_t count)
{
    if (impl) impl->VectorAbs(input, result, count);
}

const char* SIMD::GetSupportedInstructions() const
{
    if (impl) return impl->GetSupportedInstructions();
    return "None";
}

bool SIMD::IsAVX512Supported() const
{
#ifdef LUMA_X64
    return g_cpu_info.IsAVX512FSupported();
#else
    return false;
#endif
}

bool SIMD::IsAVX2Supported() const
{
#ifdef LUMA_X64
    return g_cpu_info.IsAVX2Supported() && g_cpu_info.IsAVXSupported();
#else
    return false;
#endif
}

bool SIMD::IsAVXSupported() const
{
#ifdef LUMA_X64
    return g_cpu_info.IsAVXSupported() && g_cpu_info.IsSSE42Supported();
#else
    return false;
#endif
}

bool SIMD::IsSSE42Supported() const
{
#ifdef LUMA_X64
    return g_cpu_info.IsSSE42Supported();
#else
    return false;
#endif
}

bool SIMD::IsNEONSupported() const
{
#if defined(LUMA_ARM64) && defined(LUMA_NEON)
    return true;
#else
    return false;
#endif
}
