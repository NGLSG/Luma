#include "SIMDWrapper.h"
#ifdef LUMA_X64
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
            __cpuid(level, regs[0], regs[1], regs[2], regs[3]);
#endif
        }

        void cpuidex(std::array<int, 4>& regs, int level, int count)
        {
#if defined(_MSC_VER)
            __cpuidex(regs.data(), level, count);
#elif defined(__GNUC__) || defined(__clang__)
            __cpuid_count(level, count, regs[0], regs[1], regs[2], regs[3]);
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
        int i = 0;
        for (; i <= count - 4; i += 4)
        {
            __m128 av = _mm_loadu_ps(a + i);
            __m128 bv = _mm_loadu_ps(b + i);
            __m128 rv = _mm_add_ps(av, bv);
            _mm_storeu_ps(result + i, rv);
        }
        for (; i < count; i++)
        {
            result[i] = a[i] + b[i];
        }
    }

    void VectorMultiply(const float* a, const float* b, float* result, size_t count) override
    {
        int i = 0;
        for (; i <= count - 4; i += 4)
        {
            __m128 av = _mm_loadu_ps(a + i);
            __m128 bv = _mm_loadu_ps(b + i);
            __m128 rv = _mm_mul_ps(av, bv);
            _mm_storeu_ps(result + i, rv);
        }
        for (; i < count; i++)
        {
            result[i] = a[i] * b[i];
        }
    }

    float VectorDotProduct(const float* a, const float* b, size_t count) override
    {
        __m128 sum = _mm_setzero_ps();
        int i = 0;
        for (; i <= count - 4; i += 4)
        {
            __m128 av = _mm_loadu_ps(a + i);
            __m128 bv = _mm_loadu_ps(b + i);
            __m128 mul = _mm_mul_ps(av, bv);
            sum = _mm_add_ps(sum, mul);
        }
        float temp[4];
        _mm_storeu_ps(temp, sum);
        float result = temp[0] + temp[1] + temp[2] + temp[3];
        for (; i < count; i++)
        {
            result += a[i] * b[i];
        }
        return result;
    }

    void VectorSqrt(const float* input, float* result, size_t count) override
    {
        int i = 0;
        for (; i <= count - 4; i += 4)
        {
            __m128 iv = _mm_loadu_ps(input + i);
            __m128 rv = _mm_sqrt_ps(iv);
            _mm_storeu_ps(result + i, rv);
        }
        for (; i < count; i++)
        {
            result[i] = sqrtf(input[i]);
        }
    }

    void VectorReciprocal(const float* input, float* result, size_t count) override
    {
        int i = 0;
        for (; i <= count - 4; i += 4)
        {
            __m128 iv = _mm_loadu_ps(input + i);
            __m128 rv = _mm_rcp_ps(iv);
            _mm_storeu_ps(result + i, rv);
        }
        for (; i < count; i++)
        {
            result[i] = 1.0f / input[i];
        }
    }

    float VectorMax(const float* input, size_t count) override
    {
        int i = 0;
        __m128 maxVec = _mm_set1_ps(-FLT_MAX);
        for (; i <= count - 4; i += 4)
        {
            __m128 iv = _mm_loadu_ps(input + i);
            maxVec = _mm_max_ps(maxVec, iv);
        }
        float temp[4];
        _mm_storeu_ps(temp, maxVec);
        float result = temp[0];
        for (int j = 1; j < 4; j++)
        {
            if (temp[j] > result)
                result = temp[j];
        }
        for (; i < count; i++)
        {
            if (input[i] > result)
                result = input[i];
        }
        return result;
    }

    float VectorMin(const float* input, size_t count) override
    {
        int i = 0;
        __m128 minVec = _mm_set1_ps(FLT_MAX);
        for (; i <= count - 4; i += 4)
        {
            __m128 iv = _mm_loadu_ps(input + i);
            minVec = _mm_min_ps(minVec, iv);
        }
        float temp[4];
        _mm_storeu_ps(temp, minVec);
        float result = temp[0];
        for (int j = 1; j < 4; j++)
        {
            if (temp[j] < result)
                result = temp[j];
        }
        for (; i < count; i++)
        {
            if (input[i] < result)
                result = input[i];
        }
        return result;
    }

    void VectorAbs(const float* input, float* result, size_t count) override
    {
        int i = 0;
        __m128 signMask = _mm_set1_ps(-0.0f);
        for (; i <= count - 4; i += 4)
        {
            __m128 iv = _mm_loadu_ps(input + i);
            __m128 rv = _mm_andnot_ps(signMask, iv);
            _mm_storeu_ps(result + i, rv);
        }
        for (; i < count; i++)
        {
            result[i] = fabsf(input[i]);
        }
    }

    const char* GetSupportedInstructions() const override
    {
        return "SSE4.2";
    }

    void VectorMultiplyAdd(const float* a, const float* b, const float* c, float* result, size_t count) override
    {
        int i = 0;
        for (; i <= count - 4; i += 4)
        {
            __m128 av = _mm_loadu_ps(a + i);
            __m128 bv = _mm_loadu_ps(b + i);
            __m128 cv = _mm_loadu_ps(c + i);
            __m128 mul = _mm_mul_ps(av, bv);
            __m128 rv = _mm_add_ps(mul, cv);
            _mm_storeu_ps(result + i, rv);
        }
        for (; i < count; i++)
        {
            result[i] = a[i] * b[i] + c[i];
        }
    }

    void VectorRotatePoints(const float* points_x, const float* points_y, const float* sin_vals, const float* cos_vals,
                            float* result_x, float* result_y, size_t count) override
    {
        int i = 0;
        for (; i <= count - 4; i += 4)
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
        for (; i < count; i++)
        {
            result_x[i] = points_x[i] * cos_vals[i] - points_y[i] * sin_vals[i];
            result_y[i] = points_x[i] * sin_vals[i] + points_y[i] * cos_vals[i];
        }
    }
};
#if defined(LUMA_AVX)
class AVX : public SIMDIpml
{
public:
    void VectorAdd(const float* a, const float* b, float* result, size_t count) override
    {
        int i = 0;
        for (; i <= count - 8; i += 8)
        {
            __m256 av = _mm256_loadu_ps(a + i);
            __m256 bv = _mm256_loadu_ps(b + i);
            __m256 rv = _mm256_add_ps(av, bv);
            _mm256_storeu_ps(result + i, rv);
        }
        for (; i < count; i++)
        {
            result[i] = a[i] + b[i];
        }
    }

    void VectorMultiply(const float* a, const float* b, float* result, size_t count) override
    {
        int i = 0;
        for (; i <= count - 8; i += 8)
        {
            __m256 av = _mm256_loadu_ps(a + i);
            __m256 bv = _mm256_loadu_ps(b + i);
            __m256 rv = _mm256_mul_ps(av, bv);
            _mm256_storeu_ps(result + i, rv);
        }
        for (; i < count; i++)
        {
            result[i] = a[i] * b[i];
        }
    }

    float VectorDotProduct(const float* a, const float* b, size_t count) override
    {
        __m256 sum = _mm256_setzero_ps();
        int i = 0;
        for (; i <= count - 8; i += 8)
        {
            __m256 av = _mm256_loadu_ps(a + i);
            __m256 bv = _mm256_loadu_ps(b + i);
            __m256 mul = _mm256_mul_ps(av, bv);
            sum = _mm256_add_ps(sum, mul);
        }
        float temp[8];
        _mm256_storeu_ps(temp, sum);
        float result = temp[0] + temp[1] + temp[2] + temp[3] + temp[4] + temp[5] + temp[6] + temp[7];
        for (; i < count; i++)
        {
            result += a[i] * b[i];
        }
        return result;
    }

    void VectorSqrt(const float* input, float* result, size_t count) override
    {
        int i = 0;
        for (; i <= count - 8; i += 8)
        {
            __m256 iv = _mm256_loadu_ps(input + i);
            __m256 rv = _mm256_sqrt_ps(iv);
            _mm256_storeu_ps(result + i, rv);
        }
        for (; i < count; i++)
        {
            result[i] = sqrtf(input[i]);
        }
    }

    void VectorReciprocal(const float* input, float* result, size_t count) override
    {
        int i = 0;
        for (; i <= count - 8; i += 8)
        {
            __m256 iv = _mm256_loadu_ps(input + i);
            __m256 rv = _mm256_rcp_ps(iv);
            _mm256_storeu_ps(result + i, rv);
        }
        for (; i < count; i++)
        {
            result[i] = 1.0f / input[i];
        }
    }

    float VectorMax(const float* input, size_t count) override
    {
        int i = 0;
        __m256 maxVec = _mm256_set1_ps(-FLT_MAX);
        for (; i <= count - 8; i += 8)
        {
            __m256 iv = _mm256_loadu_ps(input + i);
            maxVec = _mm256_max_ps(maxVec, iv);
        }
        float temp[8];
        _mm256_storeu_ps(temp, maxVec);
        float result = temp[0];
        for (int j = 1; j < 8; j++)
        {
            if (temp[j] > result)
                result = temp[j];
        }
        for (; i < count; i++)
        {
            if (input[i] > result)
                result = input[i];
        }
        return result;
    }

    float VectorMin(const float* input, size_t count) override
    {
        int i = 0;
        __m256 minVec = _mm256_set1_ps(FLT_MAX);
        for (; i <= count - 8; i += 8)
        {
            __m256 iv = _mm256_loadu_ps(input + i);
            minVec = _mm256_min_ps(minVec, iv);
        }
        float temp[8];
        _mm256_storeu_ps(temp, minVec);
        float result = temp[0];
        for (int j = 1; j < 8; j++)
        {
            if (temp[j] < result)
                result = temp[j];
        }
        for (; i < count; i++)
        {
            if (input[i] < result)
                result = input[i];
        }
        return result;
    }

    void VectorAbs(const float* input, float* result, size_t count) override
    {
        int i = 0;
        __m256 signMask = _mm256_set1_ps(-0.0f);
        for (; i <= count - 8; i += 8)
        {
            __m256 iv = _mm256_loadu_ps(input + i);
            __m256 rv = _mm256_andnot_ps(signMask, iv);
            _mm256_storeu_ps(result + i, rv);
        }
        for (; i < count; i++)
        {
            result[i] = fabsf(input[i]);
        }
    }

    const char* GetSupportedInstructions() const override
    {
        return "AVX";
    }

    void VectorMultiplyAdd(const float* a, const float* b, const float* c, float* result, size_t count) override
    {
        int i = 0;
        for (; i <= count - 8; i += 8)
        {
            __m256 av = _mm256_loadu_ps(a + i);
            __m256 bv = _mm256_loadu_ps(b + i);
            __m256 cv = _mm256_loadu_ps(c + i);
            __m256 mul = _mm256_mul_ps(av, bv);
            __m256 rv = _mm256_add_ps(mul, cv);
            _mm256_storeu_ps(result + i, rv);
        }
        for (; i < count; i++)
        {
            result[i] = a[i] * b[i] + c[i];
        }
    }

    void VectorRotatePoints(const float* points_x, const float* points_y, const float* sin_vals, const float* cos_vals,
                            float* result_x, float* result_y, size_t count) override
    {
        int i = 0;
        for (; i <= count - 8; i += 8)
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
        for (; i < count; i++)
        {
            result_x[i] = points_x[i] * cos_vals[i] - points_y[i] * sin_vals[i];
            result_y[i] = points_x[i] * sin_vals[i] + points_y[i] * cos_vals[i];
        }
    }
};
#endif

#if defined(LUMA_AVX2)
class AVX2 : public AVX
{
public:
    float VectorDotProduct(const float* a, const float* b, size_t count) override
    {
        __m256 sum = _mm256_setzero_ps();
        int i = 0;
        for (; i <= count - 8; i += 8)
        {
            __m256 av = _mm256_loadu_ps(a + i);
            __m256 bv = _mm256_loadu_ps(b + i);
            sum = _mm256_fmadd_ps(av, bv, sum);
        }

        float temp[8];
        _mm256_storeu_ps(temp, sum);
        float result = temp[0] + temp[1] + temp[2] + temp[3] + temp[4] + temp[5] + temp[6] + temp[7];
        for (; i < count; i++)
        {
            result += a[i] * b[i];
        }
        return result;
    }

    void VectorMultiplyAdd(const float* a, const float* b, const float* c, float* result, size_t count) override
    {
        int i = 0;
        for (; i <= count - 8; i += 8)
        {
            __m256 av = _mm256_loadu_ps(a + i);
            __m256 bv = _mm256_loadu_ps(b + i);
            __m256 cv = _mm256_loadu_ps(c + i);
            __m256 rv = _mm256_fmadd_ps(av, bv, cv);
            _mm256_storeu_ps(result + i, rv);
        }
        for (; i < count; i++)
        {
            result[i] = a[i] * b[i] + c[i];
        }
    }

    const char* GetSupportedInstructions() const override
    {
        return "AVX2";
    }
};
#endif
#if defined(LUMA_AVX512)
class AVX512 : public SIMDIpml
{
public:
    void VectorAdd(const float* a, const float* b, float* result, size_t count) override
    {
        size_t i = 0;
        for (; i <= count - 16; i += 16)
        {
            __m512 av = _mm512_loadu_ps(a + i);
            __m512 bv = _mm512_loadu_ps(b + i);
            __m512 rv = _mm512_add_ps(av, bv);
            _mm512_storeu_ps(result + i, rv);
        }
        for (; i < count; i++)
        {
            result[i] = a[i] + b[i];
        }
    }

    void VectorMultiply(const float* a, const float* b, float* result, size_t count) override
    {
        size_t i = 0;
        for (; i <= count - 16; i += 16)
        {
            __m512 av = _mm512_loadu_ps(a + i);
            __m512 bv = _mm512_loadu_ps(b + i);
            __m512 rv = _mm512_mul_ps(av, bv);
            _mm512_storeu_ps(result + i, rv);
        }
        for (; i < count; i++)
        {
            result[i] = a[i] * b[i];
        }
    }

    float VectorDotProduct(const float* a, const float* b, size_t count) override
    {
        __m512 sumVec = _mm512_setzero_ps();
        size_t i = 0;
        for (; i <= count - 16; i += 16)
        {
            __m512 av = _mm512_loadu_ps(a + i);
            __m512 bv = _mm512_loadu_ps(b + i);
            sumVec = _mm512_fmadd_ps(av, bv, sumVec);
        }
        float result = _mm512_reduce_add_ps(sumVec);

        for (; i < count; i++)
        {
            result += a[i] * b[i];
        }
        return result;
    }

    void VectorSqrt(const float* input, float* result, size_t count) override
    {
        size_t i = 0;
        for (; i <= count - 16; i += 16)
        {
            __m512 iv = _mm512_loadu_ps(input + i);
            __m512 rv = _mm512_sqrt_ps(iv);
            _mm512_storeu_ps(result + i, rv);
        }
        for (; i < count; i++)
        {
            result[i] = sqrtf(input[i]);
        }
    }

    void VectorReciprocal(const float* input, float* result, size_t count) override
    {
        size_t i = 0;
        for (; i <= count - 16; i += 16)
        {
            __m512 iv = _mm512_loadu_ps(input + i);
            __m512 rv = _mm512_rcp14_ps(iv);
            _mm512_storeu_ps(result + i, rv);
        }
        for (; i < count; i++)
        {
            result[i] = 1.0f / input[i];
        }
    }

    float VectorMax(const float* input, size_t count) override
    {
        if (count == 0) return -FLT_MAX;

        __m512 maxVec = _mm512_set1_ps(-FLT_MAX);
        size_t i = 0;
        for (; i <= count - 16; i += 16)
        {
            __m512 iv = _mm512_loadu_ps(input + i);
            maxVec = _mm512_max_ps(maxVec, iv);
        }
        float result = _mm512_reduce_max_ps(maxVec);

        
        for (; i < count; i++)
        {
            if (input[i] > result)
                result = input[i];
        }
        return result;
    }

    float VectorMin(const float* input, size_t count) override
    {
        if (count == 0) return FLT_MAX;

        __m512 minVec = _mm512_set1_ps(FLT_MAX);
        size_t i = 0;
        for (; i <= count - 16; i += 16)
        {
            __m512 iv = _mm512_loadu_ps(input + i);
            minVec = _mm512_min_ps(minVec, iv);
        }
        float result = _mm512_reduce_min_ps(minVec);

        for (; i < count; i++)
        {
            if (input[i] < result)
                result = input[i];
        }
        return result;
    }

    void VectorAbs(const float* input, float* result, size_t count) override
    {
        size_t i = 0;
        __m512 signMask = _mm512_set1_ps(-0.0f);
        for (; i <= count - 16; i += 16)
        {
            __m512 iv = _mm512_loadu_ps(input + i);
            __m512 rv = _mm512_andnot_ps(signMask, iv);
            _mm512_storeu_ps(result + i, rv);
        }
        for (; i < count; i++)
        {
            result[i] = fabsf(input[i]);
        }
    }

    const char* GetSupportedInstructions() const override
    {
        return "AVX512";
    }

    void VectorMultiplyAdd(const float* a, const float* b, const float* c, float* result, size_t count) override
    {
        size_t i = 0;
        for (; i <= count - 16; i += 16)
        {
            __m512 av = _mm512_loadu_ps(a + i);
            __m512 bv = _mm512_loadu_ps(b + i);
            __m512 cv = _mm512_loadu_ps(c + i);
            __m512 rv = _mm512_fmadd_ps(av, bv, cv);
            _mm512_storeu_ps(result + i, rv);
        }
        for (; i < count; i++)
        {
            result[i] = a[i] * b[i] + c[i];
        }
    }

    void VectorRotatePoints(const float* points_x, const float* points_y, const float* sin_vals, const float* cos_vals,
                            float* result_x, float* result_y, size_t count) override
    {
        size_t i = 0;
        for (; i <= count - 16; i += 16)
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
        for (; i < count; i++)
        {
            result_x[i] = points_x[i] * cos_vals[i] - points_y[i] * sin_vals[i];
            result_y[i] = points_x[i] * sin_vals[i] + points_y[i] * cos_vals[i];
        }
    }
};
#endif

#elif defined(LUMA_ARM64) && defined(LUMA_NEON)
#include <arm_neon.h> 
#include <cfloat>
#include <cmath>

class NEON : public SIMDIpml
{
public:
    void VectorAdd(const float* a, const float* b, float* result, size_t count) override
    {
        size_t i = 0;
        
        for (; i <= count - 4; i += 4)
        {
            float32x4_t av = vld1q_f32(a + i);
            float32x4_t bv = vld1q_f32(b + i);
            float32x4_t rv = vaddq_f32(av, bv);
            vst1q_f32(result + i, rv);
        }
        
        for (; i < count; i++)
        {
            result[i] = a[i] + b[i];
        }
    }

    void VectorMultiply(const float* a, const float* b, float* result, size_t count) override
    {
        size_t i = 0;
        for (; i <= count - 4; i += 4)
        {
            float32x4_t av = vld1q_f32(a + i);
            float32x4_t bv = vld1q_f32(b + i);
            float32x4_t rv = vmulq_f32(av, bv);
            vst1q_f32(result + i, rv);
        }
        for (; i < count; i++)
        {
            result[i] = a[i] * b[i];
        }
    }

    float VectorDotProduct(const float* a, const float* b, size_t count) override
    {
        float32x4_t sumVec = vdupq_n_f32(0.0f);
        size_t i = 0;
        for (; i <= count - 4; i += 4)
        {
            float32x4_t av = vld1q_f32(a + i);
            float32x4_t bv = vld1q_f32(b + i);
            
            sumVec = vfmaq_f32(sumVec, av, bv);
        }
        
        float result = vaddvq_f32(sumVec);

        for (; i < count; i++)
        {
            result += a[i] * b[i];
        }
        return result;
    }

    void VectorSqrt(const float* input, float* result, size_t count) override
    {
        size_t i = 0;
        for (; i <= count - 4; i += 4)
        {
            float32x4_t iv = vld1q_f32(input + i);
            float32x4_t rv = vsqrtq_f32(iv);
            vst1q_f32(result + i, rv);
        }
        for (; i < count; i++)
        {
            result[i] = sqrtf(input[i]);
        }
    }

    void VectorReciprocal(const float* input, float* result, size_t count) override
    {
        size_t i = 0;
        for (; i <= count - 4; i += 4)
        {
            float32x4_t iv = vld1q_f32(input + i);
            
            float32x4_t rv = vrecpeq_f32(iv);
            vst1q_f32(result + i, rv);
        }
        for (; i < count; i++)
        {
            result[i] = 1.0f / input[i];
        }
    }

    float VectorMax(const float* input, size_t count) override
    {
        if (count == 0) return -FLT_MAX;

        float32x4_t maxVec = vdupq_n_f32(-FLT_MAX);
        size_t i = 0;
        for (; i <= count - 4; i += 4)
        {
            float32x4_t iv = vld1q_f32(input + i);
            maxVec = vmaxq_f32(maxVec, iv);
        }
        
        float result = vmaxvq_f32(maxVec);

        for (; i < count; i++)
        {
            if (input[i] > result)
                result = input[i];
        }
        return result;
    }

    float VectorMin(const float* input, size_t count) override
    {
        if (count == 0) return FLT_MAX;

        float32x4_t minVec = vdupq_n_f32(FLT_MAX);
        size_t i = 0;
        for (; i <= count - 4; i += 4)
        {
            float32x4_t iv = vld1q_f32(input + i);
            minVec = vminq_f32(minVec, iv);
        }
        
        float result = vminvq_f32(minVec);

        for (; i < count; i++)
        {
            if (input[i] < result)
                result = input[i];
        }
        return result;
    }

    void VectorAbs(const float* input, float* result, size_t count) override
    {
        size_t i = 0;
        for (; i <= count - 4; i += 4)
        {
            float32x4_t iv = vld1q_f32(input + i);
            
            float32x4_t rv = vabsq_f32(iv);
            vst1q_f32(result + i, rv);
        }
        for (; i < count; i++)
        {
            result[i] = fabsf(input[i]);
        }
    }

    const char* GetSupportedInstructions() const override
    {
        return "NEON";
    }

    void VectorMultiplyAdd(const float* a, const float* b, const float* c, float* result, size_t count) override
    {
        size_t i = 0;
        for (; i <= count - 4; i += 4)
        {
            float32x4_t av = vld1q_f32(a + i);
            float32x4_t bv = vld1q_f32(b + i);
            float32x4_t cv = vld1q_f32(c + i);
            
            float32x4_t rv = vfmaq_f32(cv, av, bv);
            vst1q_f32(result + i, rv);
        }
        for (; i < count; i++)
        {
            result[i] = a[i] * b[i] + c[i];
        }
    }

    void VectorRotatePoints(const float* points_x, const float* points_y, const float* sin_vals, const float* cos_vals,
                            float* result_x, float* result_y, size_t count) override
    {
        size_t i = 0;
        for (; i <= count - 4; i += 4)
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
        for (; i < count; i++)
        {
            result_x[i] = points_x[i] * cos_vals[i] - points_y[i] * sin_vals[i];
            result_y[i] = points_x[i] * sin_vals[i] + points_y[i] * cos_vals[i];
        }
    }
};
#endif

void SIMD::VectorAdd(const float* a, const float* b, float* result, size_t count)
{
    if (impl)
    {
        impl->VectorAdd(a, b, result, count);
    }
}

void SIMD::VectorMultiply(const float* a, const float* b, float* result, size_t count)
{
    if (impl)
    {
        impl->VectorMultiply(a, b, result, count);
    }
}

void SIMD::VectorMultiplyAdd(const float* a, const float* b, const float* c, float* result, size_t count)
{
    if (impl)
    {
        impl->VectorMultiplyAdd(a, b, c, result, count);
    }
}

void SIMD::VectorRotatePoints(const float* points_x, const float* points_y, const float* sin_vals,
                              const float* cos_vals, float* result_x, float* result_y, size_t count)
{
    if (impl)
    {
        impl->VectorRotatePoints(points_x, points_y, sin_vals, cos_vals, result_x, result_y, count);
    }
}

float SIMD::VectorDotProduct(const float* a, const float* b, size_t count)
{
    if (count == 0) return 0.0f;
    if (impl)
    {
        return impl->VectorDotProduct(a, b, count);
    }
    return 0.0f;
}

void SIMD::VectorSqrt(const float* input, float* result, size_t count)
{
    if (impl)
    {
        impl->VectorSqrt(input, result, count);
    }
}

void SIMD::VectorReciprocal(const float* input, float* result, size_t count)
{
    if (impl)
    {
        impl->VectorReciprocal(input, result, count);
    }
}

float SIMD::VectorMax(const float* input, size_t count)
{
    if (count == 0) return FLT_MAX;
    if (impl)
    {
        return impl->VectorMax(input, count);
    }
    return FLT_MAX;
}

float SIMD::VectorMin(const float* input, size_t count)
{
    if (count == 0) return -FLT_MAX;
    if (impl)
    {
        return impl->VectorMin(input, count);
    }
    return -FLT_MAX;
}

void SIMD::VectorAbs(const float* input, float* result, size_t count)
{
    if (impl)
    {
        impl->VectorAbs(input, result, count);
    }
}

const char* SIMD::GetSupportedInstructions() const
{
    if (impl)
    {
        return impl->GetSupportedInstructions();
    }
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
    return g_cpu_info.IsAVX2Supported() && IsAVXSupported();
#else
    return false;
#endif
}

bool SIMD::IsAVXSupported() const
{
#ifdef LUMA_X64
    return g_cpu_info.IsAVXSupported() && IsSSE42Supported();
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

SIMD::SIMD()
{
#if defined(LUMA_X64)
    if (g_cpu_info.IsAVX512FSupported())
    {
        impl = std::make_unique<AVX512>();
        return;
    }
    else if (g_cpu_info.IsAVX2Supported())
    {
        impl = std::make_unique<AVX2>();
        return;
    }
    else if (g_cpu_info.IsAVXSupported())
    {
        impl = std::make_unique<AVX>();
        return;
    }
    else if (g_cpu_info.IsSSE42Supported())
    {
        impl = std::make_unique<SSE42>();
        return;
    }
#else
    impl = std::make_unique<NEON>();
    return;
#endif
}
