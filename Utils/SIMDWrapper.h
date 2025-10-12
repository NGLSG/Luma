#ifndef LUMAENGINE_SIMDWRAPPER_H
#define LUMAENGINE_SIMDWRAPPER_H

/**
 * @class SIMDIpml
 * @brief SIMD指令集实现类接口
 *
 * 该抽象类定义了常用SIMD向量运算的接口，便于不同平台和指令集的具体实现。
 */
class SIMDIpml
{
public:
    /**
     * @brief 虚析构函数，允许派生类正确析构。
     */
    virtual ~SIMDIpml() = default;

    /**
     * @brief 向量加法运算
     * @param a 输入向量A
     * @param b 输入向量B
     * @param result 结果存储向量
     * @param count 向量元素数量
     */
    virtual void VectorAdd(const float* a, const float* b, float* result, size_t count) = 0;

    /**
     * @brief 向量乘法运算
     * @param a 输入向量A
     * @param b 输入向量B
     * @param result 结果存储向量
     * @param count 向量元素数量
     */
    virtual void VectorMultiply(const float* a, const float* b, float* result, size_t count) = 0;

    /**
     * @brief 向量乘加运算 (result = a * b + c)
     * @param a 输入向量A
     * @param b 输入向量B
     * @param c 输入向量C
     * @param result 结果存储向量
     * @param count 向量元素数量
     */
    virtual void VectorMultiplyAdd(const float* a, const float* b, const float* c, float* result, size_t count) = 0;

    /**
     * @brief 向量点积运算
     * @param a 输入向量A
     * @param b 输入向量B
     * @param count 向量元素数量
     * @return 点积结果
     */
    virtual float VectorDotProduct(const float* a, const float* b, size_t count) = 0;

    /**
     * @brief 向量平方根运算
     * @param input 输入向量
     * @param result 结果存储向量
     * @param count 向量元素数量
     */
    virtual void VectorSqrt(const float* input, float* result, size_t count) = 0;

    /**
     * @brief 向量倒数运算
     * @param input 输入向量
     * @param result 结果存储向量
     * @param count 向量元素数量
     */
    virtual void VectorReciprocal(const float* input, float* result, size_t count) = 0;

    /**
     * @brief 批量旋转二维点坐标
     * @param points_x 输入点的 x 坐标数组
     * @param points_y 输入点的 y 坐标数组
     * @param sin_vals 每个点对应的旋转角度的正弦值数组
     * @param cos_vals 每个点对应的旋转角度的余弦值数组
     * @param result_x 输出旋转后点的 x 坐标数组
     * @param result_y 输出旋转后点的 y 坐标数组
     * @param count 点的数量
     */
    virtual void VectorRotatePoints(const float* points_x, const float* points_y,
                                    const float* sin_vals, const float* cos_vals,
                                    float* result_x, float* result_y, size_t count) = 0;

    /**
     * @brief 向量最大值运算
     * @param input 输入向量
     * @param count 向量元素数量
     * @return 向量中的最大值
     */
    virtual float VectorMax(const float* input, size_t count) = 0;

    /**
     * @brief 向量最小值运算
     * @param input 输入向量
     * @param count 向量元素数量
     * @return 向量中的最小值
     */
    virtual float VectorMin(const float* input, size_t count) = 0;

    /**
     * @brief 向量绝对值运算
     * @param input 输入向量
     * @param result 结果存储向量
     * @param count 向量元素数量
     */
    virtual void VectorAbs(const float* input, float* result, size_t count) = 0;
    /**
        * @brief 向量与标量乘法运算 (result = a * b)
        * @param a 输入向量A
        * @param b 输入标量B
        * @param result 结果存储向量
        * @param count 向量元素数量
        */
    virtual void VectorScalarMultiply(const float* a, float b, float* result, size_t count) = 0;

    /**
     * @brief 获取当前实现支持的SIMD指令集描述
     * @return 指令集字符串
     */
    virtual const char* GetSupportedInstructions() const = 0;
};

/**
 * @class SIMD
 * @brief SIMD指令集封装单例类
 *
 * 该类提供了跨架构(Arm, x86)的SIMD指令统一接口，自动检测并调用当前CPU支持的最优指令集实现。
 * 支持AVX, AVX2, AVX512和Arm NEON等多种SIMD指令集。
 */
class SIMD : public LazySingleton<SIMD>
{
    friend class LazySingleton<SIMD>;

public:
    /**
     * @brief 单精度浮点向量加法运算
     * @param a 输入向量A
     * @param b 输入向量B
     * @param result 结果存储向量
     * @param count 向量元素数量
     */
    void VectorAdd(const float* a, const float* b, float* result, size_t count);

    /**
     * @brief 单精度浮点向量乘法运算
     * @param a 输入向量A
     * @param b 输入向量B
     * @param result 结果存储向量
     * @param count 向量元素数量
     */
    void VectorMultiply(const float* a, const float* b, float* result, size_t count);

    /**
     * @brief 单精度浮点向量乘加运算 (result = a * b + c)
     * @param a 输入向量A
     * @param b 输入向量B
     * @param c 输入向量C
     * @param result 结果存储向量
     * @param count 向量元素数量
     */
    void VectorMultiplyAdd(const float* a, const float* b, const float* c, float* result, size_t count);

    /**
     * @brief 批量旋转二维点坐标
     * @param points_x 输入点的 x 坐标数组
     * @param points_y 输入点的 y 坐标数组
     * @param sin_vals 每个点对应的旋转角度的正弦值数组
     * @param cos_vals 每个点对应的旋转角度的余弦值数组
     * @param result_x 输出旋转后点的 x 坐标数组
     * @param result_y 输出旋转后点的 y 坐标数组
     * @param count 点的数量
     */
    void VectorRotatePoints(const float* points_x, const float* points_y,
                            const float* sin_vals, const float* cos_vals,
                            float* result_x, float* result_y, size_t count);
    /**
     * @brief 单精度浮点向量点积运算
     * @param a 输入向量A
     * @param b 输入向量B
     * @param count 向量元素数量
     * @return 点积结果
     */
    float VectorDotProduct(const float* a, const float* b, size_t count);

    /**
     * @brief 单精度浮点向量平方根运算
     * @param input 输入向量
     * @param result 结果存储向量
     * @param count 向量元素数量
     */
    void VectorSqrt(const float* input, float* result, size_t count);

    /**
     * @brief 单精度浮点向量倒数运算
     * @param input 输入向量
     * @param result 结果存储向量
     * @param count 向量元素数量
     */
    void VectorReciprocal(const float* input, float* result, size_t count);

    /**
     * @brief 单精度浮点向量最大值运算
     * @param input 输入向量
     * @param count 向量元素数量
     * @return 向量中的最大值
     */
    float VectorMax(const float* input, size_t count);

    /**
     * @brief 单精度浮点向量最小值运算
     * @param input 输入向量
     * @param count 向量元素数量
     * @return 向量中的最小值
     */
    float VectorMin(const float* input, size_t count);

    /**
     * @brief 单精度浮点向量绝对值运算
     * @param input 输入向量
     * @param result 结果存储向量
     * @param count 向量元素数量
     */
    void VectorAbs(const float* input, float* result, size_t count);
    /**
         * @brief 单精度浮点向量与标量乘法运算
         * @param a 输入向量A
         * @param b 输入标量B
         * @param result 结果存储向量
         * @param count 向量元素数量
         */
    void VectorScalarMultiply(const float* a, float b, float* result, size_t count);

    /**
     * @brief 获取当前CPU支持的SIMD指令集
     * @return 支持的SIMD指令集字符串描述
     */
    const char* GetSupportedInstructions() const;

    /**
     * @brief 检查AVX512指令集是否可用
     * @return 可用返回true，否则返回false
     */
    bool IsAVX512Supported() const;

    /**
     * @brief 检查AVX2指令集是否可用
     * @return 可用返回true，否则返回false
     */
    bool IsAVX2Supported() const;

    /**
     * @brief 检查AVX指令集是否可用
     * @return 可用返回true，否则返回false
     */
    bool IsAVXSupported() const;

    /**
     * @brief 检查SSE4.2指令集是否可用
     * @return 可用返回true，否则返回false
     */
    bool IsSSE42Supported() const;

    /**
     * @brief 检查Arm NEON指令集是否可用
     * @return 可用返回true，否则返回false
     */
    bool IsNEONSupported() const;

private:
    /**
     * @brief 私有构造函数
     */
    SIMD();

    std::unique_ptr<SIMDIpml> impl; ///< SIMD实现类的唯一指针
};
#endif // LUMAENGINE_SIMDWRAPPER_H
