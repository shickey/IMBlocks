/*********************************************************
*
* BlocksMath.h
* IMBlocks
*
* Sean Hickey
* 2020
*
**********************************************************/

#define Max(a, b) (((a) > (b)) ? (a) : (b))
#define Min(a, b) (((a) < (b)) ? (a) : (b))

inline
f32 DistSq(v2 a, v2 b) {
  return ((a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y));
}

inline
b32 PointInRect(v2 point, Rectangle rect) {
    return (point.x >= rect.x && point.x <= rect.x + rect.w) 
        && (point.y >= rect.y && point.y <= rect.y + rect.h);
}

inline
b32 RectsIntersect(Rectangle a, Rectangle b) {
  return (a.x < (b.x + b.w))
      && (b.x < (a.x + a.w))
      && (a.y < (b.y + b.h))
      && (b.y < (a.y + a.h));
}

inline
f32 Clamp(f32 val, f32 min, f32 max) {
  return Max(min, Min(max, val));
}

inline
v4 HexToColor(u32 hex) {
    return v4{
        (f32)((hex >> 16) & 0xFF) / 255.0f,
        (f32)((hex >> 8 ) & 0xFF) / 255.0f,
        (f32)((hex >> 0 ) & 0xFF) / 255.0f,
        1.0
    };
}

inline
mat4x4 OrthographicProjection(f32 left, f32 right, f32 bottom, f32 top, f32 near, f32 far) {
    return mat4x4{
        v4{            2.0f / (right - left),                                0,                            0,     0 },
        v4{                                0,            2.0f / (top - bottom),                            0,     0 },
        v4{                                0,                                0,         -2.0f / (far - near),     0 },
        v4{ -(right + left) / (right - left), -(top + bottom) / (top - bottom), -(far + near) / (far - near),  1.0f }
    };
}

inline
mat4x4 OrthographicUnprojection(f32 left, f32 right, f32 bottom, f32 top, f32 near, f32 far) {
    return mat4x4{
        v4{ (right - left) / 2.0f,                     0,                    0,    0 },
        v4{                     0, (top - bottom) / 2.0f,                    0,    0 },
        v4{                     0,                     0, (far - near) / -2.0f,    0 },
        v4{ (right + left) / 2.0f, (top + bottom) / 2.0f, -(far + near) / 2.0f, 1.0f }
    };
}

// @NOTE: This is more useful since we usually want to do [M] * [row vector] to unproject
inline
mat4x4 OrthographicUnprojectionTranspose(f32 left, f32 right, f32 bottom, f32 top, f32 near, f32 far) {
    return mat4x4{
        v4{ (right - left) / 2.0f,                     0,                    0,    (right + left) / 2.0f },
        v4{                     0, (top - bottom) / 2.0f,                    0,    (top + bottom) / 2.0f },
        v4{                     0,                     0, (far - near) / -2.0f,     -(far + near) / 2.0f },
        v4{                     0,                     0,                    0,                     1.0f }
    };
}

inline
TransformPair BlocksCameraTransformPair(v2 screenSize, f32 zoomLevel, v2 cameraOrigin) {
    f32 halfWidth = (screenSize.w / 2.0) / zoomLevel;
    f32 halfHeight = (screenSize.h / 2.0) / zoomLevel;
    TransformPair result;
    result.transform = OrthographicProjection(-halfWidth + cameraOrigin.x,
                                               halfWidth + cameraOrigin.x,
                                              -halfHeight + cameraOrigin.y,
                                               halfHeight + cameraOrigin.y, 1.0, -1.0);
    result.invTransform = OrthographicUnprojectionTranspose(-halfWidth + cameraOrigin.x,
                                                             halfWidth + cameraOrigin.x,
                                                            -halfHeight + cameraOrigin.y,
                                                             halfHeight + cameraOrigin.y, 1.0, -1.0);
    return result;
}

inline
TransformPair OneToOneCameraTransformPair(v2 screenSize) {
    TransformPair result;
    result.transform = OrthographicProjection(0, screenSize.w, 0, screenSize.h, 1.0, -1.0);
    result.invTransform = OrthographicUnprojectionTranspose(0, screenSize.w, 0, screenSize.h, 1.0, -1.0);
    return result;
}

inline
f32 Dot(v4 a, v4 b) {
  return (a.x * b.x) + (a.y * b.y) + (a.z * b.z) + (a.w * b.w);
}

inline
v4 operator*(mat4x4 a, v4 b) {
  return v4{ Dot(a.columns[0], b), Dot(a.columns[1], b), Dot(a.columns[2], b), Dot(a.columns[3], b) };
}

