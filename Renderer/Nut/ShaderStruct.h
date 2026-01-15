#ifndef LUMAENGINE_SHADERSTRUCT_H
#define LUMAENGINE_SHADERSTRUCT_H

#include <glm/glm.hpp>

struct EngineData
{
    glm::vec2 CameraPosition;

    float CameraScaleX;

    float CameraScaleY;

    float CameraSinR;

    float CameraCosR;

    glm::vec2 ViewportSize;

    glm::vec2 TimeData;

    glm::vec2 MousePosition;
};

static_assert(sizeof(EngineData) % 16 == 0, "EngineData size must be multiple of 16 bytes.");

struct alignas(16) InstanceData
{
    struct
    {
        float x, y, z, w;
    } position;

    float scaleX, scaleY, sinR, cosR;

    struct
    {
        float r, g, b, a;
    } color;

    struct
    {
        float x, y, z, w;
    } uvRect;

    struct
    {
        float width, height;
    } size;

    uint32_t lightLayer;  ///< 光照层掩码
    uint32_t padding;     ///< 对齐填充

    // 自发光数据 (Feature: 2d-lighting-enhancement)
    struct
    {
        float r, g, b, a;
    } emissionColor;

    float emissionIntensity;  ///< 自发光强度（支持 HDR > 1.0）
    float emissionPadding1;   ///< 对齐填充
    float emissionPadding2;   ///< 对齐填充
    float emissionPadding3;   ///< 对齐填充
};

static_assert(sizeof(InstanceData) == 112, "InstanceData size must be 112 bytes to match WGSL layout.");
static_assert(sizeof(InstanceData) % 16 == 0, "InstanceData size must be multiple of 16 bytes.");

struct Vertex
{
    float position[2];
    float texCoord[2];
};

#endif //LUMAENGINE_SHADERSTRUCT_H
