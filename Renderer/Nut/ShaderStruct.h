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

    struct
    {
        float unused1, unused2;
    } padding;
};

static_assert(sizeof(InstanceData) == 80, "InstanceData size must be 80 bytes to match WGSL layout.");
static_assert(sizeof(InstanceData) % 16 == 0, "InstanceData size must be multiple of 16 bytes.");

struct Vertex
{
    float position[2];
    float texCoord[2];
};

#endif //LUMAENGINE_SHADERSTRUCT_H
