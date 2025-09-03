using System;
using System.Runtime.InteropServices;

namespace Luma.SDK;

public static class SIMD
{
    public static float[] VectorAdd(float[] a, float[] b)
    {
        ValidateInputVectors(a, b, nameof(a), nameof(b));
        if (a.Length == 0) return Array.Empty<float>();

        float[] result = new float[a.Length];
        Native.SIMDVectorAdd(a, b, result, a.Length);
        return result;
    }

    public static float[] VectorMultiply(float[] a, float[] b)
    {
        ValidateInputVectors(a, b, nameof(a), nameof(b));
        if (a.Length == 0) return Array.Empty<float>();

        float[] result = new float[a.Length];
        Native.SIMDVectorMultiply(a, b, result, a.Length);
        return result;
    }

    public static float VectorDotProduct(float[] a, float[] b)
    {
        ValidateInputVectors(a, b, nameof(a), nameof(b));
        if (a.Length == 0) return 0.0f;

        return Native.SIMDVectorDotProduct(a, b, a.Length);
    }

    public static float[] VectorMultiplyAdd(float[] a, float[] b, float[] c)
    {
        ValidateInputVectors(a, b, nameof(a), nameof(b));
        ValidateInputVectors(a, c, nameof(a), nameof(c));
        if (a.Length == 0) return Array.Empty<float>();

        float[] result = new float[a.Length];
        Native.SIMDVectorMultiplyAdd(a, b, c, result, a.Length);
        return result;
    }

    public static float[] VectorSqrt(float[] input)
    {
        ValidateInputVector(input, nameof(input));
        if (input.Length == 0) return Array.Empty<float>();

        float[] result = new float[input.Length];
        Native.SIMDVectorSqrt(input, result, input.Length);
        return result;
    }

    public static float[] VectorReciprocal(float[] input)
    {
        ValidateInputVector(input, nameof(input));
        if (input.Length == 0) return Array.Empty<float>();

        float[] result = new float[input.Length];
        Native.SIMDVectorReciprocal(input, result, input.Length);
        return result;
    }

    public static float VectorMax(float[] input)
    {
        ValidateInputVector(input, nameof(input), allowEmpty: false);
        return Native.SIMDVectorMax(input, input.Length);
    }

    public static float VectorMin(float[] input)
    {
        ValidateInputVector(input, nameof(input), allowEmpty: false);
        return Native.SIMDVectorMin(input, input.Length);
    }

    public static float[] VectorAbs(float[] input)
    {
        ValidateInputVector(input, nameof(input));
        if (input.Length == 0) return Array.Empty<float>();

        float[] result = new float[input.Length];
        Native.SIMDVectorAbs(input, result, input.Length);
        return result;
    }

    public static (float[] ResultX, float[] ResultY) VectorRotatePoints(
        float[] pointsX, float[] pointsY, float[] sinVals, float[] cosVals)
    {
        ValidateInputVectors(pointsX, pointsY, nameof(pointsX), nameof(pointsY));
        ValidateInputVectors(pointsX, sinVals, nameof(pointsX), nameof(sinVals));
        ValidateInputVectors(pointsX, cosVals, nameof(pointsX), nameof(cosVals));
        if (pointsX.Length == 0) return (Array.Empty<float>(), Array.Empty<float>());

        float[] resultX = new float[pointsX.Length];
        float[] resultY = new float[pointsX.Length];
        Native.SIMDVectorRotatePoints(pointsX, pointsY, sinVals, cosVals, resultX, resultY, pointsX.Length);
        return (resultX, resultY);
    }

    #region Private Validation Helpers

    private static void ValidateInputVector(float[] vec, string paramName, bool allowEmpty = true)
    {
        if (vec == null)
            throw new ArgumentNullException(paramName);
        if (!allowEmpty && vec.Length == 0)
            throw new ArgumentException("Input vector cannot be empty.", paramName);
    }

    private static void ValidateInputVectors(float[] vec1, float[] vec2, string paramName1, string paramName2)
    {
        if (vec1 == null) throw new ArgumentNullException(paramName1);
        if (vec2 == null) throw new ArgumentNullException(paramName2);
        if (vec1.Length != vec2.Length)
            throw new ArgumentOutOfRangeException(nameof(vec2), "All input vectors must have the same length.");
    }

    #endregion
}