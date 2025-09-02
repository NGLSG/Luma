#include "../Transform.h"

void Transform::updateMatrix()
{
    data.setIdentity();


    data.preTranslate(position.x, position.y);
    data.preScale(scale.x, scale.y);
    data.preRotate(rotation, pivot.x, pivot.y);
}

Transform::Transform() : position{0, 0}, scale{1, 1}, rotation{0}, pivot{0, 0}
{
    data.setIdentity();
}

Transform::Transform(const Vector2Df& pos, const Vector2Df& scl, float rot, const Vector2Df& pvt) : position(pos),
    scale(scl), rotation(rot), pivot(pvt)
{
    updateMatrix();
}

Transform::Transform(const Transform&) = default;
Transform& Transform::operator=(const Transform&) = default;
Transform::Transform(Transform&&) noexcept = default;
Transform& Transform::operator=(Transform&&) noexcept = default;

void Transform::SetPosition(const Vector2Df& pos)
{
    position = pos;
    updateMatrix();
}

void Transform::SetScale(const Vector2Df& scl)
{
    scale = scl;
    updateMatrix();
}

void Transform::SetRotation(float rot)
{
    rotation = rot;
    updateMatrix();
}

void Transform::SetPivot(const Vector2Df& pvt)
{
    pivot = pvt;
    updateMatrix();
}

Vector2Df Transform::GetPosition() const
{
    return position;
}

Vector2Df Transform::GetScale() const
{
    return scale;
}

float Transform::GetRotation() const
{
    return rotation;
}

Vector2Df Transform::GetPivot() const
{
    return pivot;
}

Transform::operator SkMatrix() const
{
    return data;
}

Transform::operator SkRSXform() const
{
    SkPoint origin = {0.0f, 0.0f};
    SkPoint xAxis = {1.0f, 0.0f};


    SkPoint mappedOrigin{}, mappedXAxis{};
    data.mapPoints(SkSpan(&mappedOrigin, 1), SkSpan(&origin, 1));
    data.mapPoints(SkSpan(&mappedXAxis, 1), SkSpan(&xAxis, 1));


    const float tx = mappedOrigin.fX;
    const float ty = mappedOrigin.fY;
    const float scos = mappedXAxis.fX - tx;
    const float ssin = mappedXAxis.fY - ty;

    return SkRSXform::Make(scos, ssin, tx, ty);
}
