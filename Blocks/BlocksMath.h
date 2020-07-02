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
        v4{ 2.0f / (right - left),                     0,                    0, -(right + left) / (right - left) },
        v4{                     0, 2.0f / (top - bottom),                    0, -(top + bottom) / (top - bottom) },
        v4{                     0,                     0, -2.0f / (far - near),     -(far + near) / (far - near) },
        v4{                     0,                     0,                    0,                             1.0f }
    };
}

inline
mat4x4 OrthographicUnprojection(f32 left, f32 right, f32 bottom, f32 top, f32 near, f32 far) {
    return mat4x4{
        v4{ (right - left) / 2.0f,                     0,                    0, (right + left) / 2.0f },
        v4{                     0, (top - bottom) / 2.0f,                    0, (top + bottom) / 2.0f },
        v4{                     0,                     0, (far - near) / -2.0f,   (far + near) / 2.0f },
        v4{                     0,                     0,                    0,                  1.0f }
    };
}

inline
f32 Dot(v4 a, v4 b) {
  return (a.x * b.x) + (a.y * b.y) + (a.z * b.z) + (a.w * b.w);
}

inline
v4 operator*(mat4x4 a, v4 b) {
  return v4{ Dot(a.columns[0], b), Dot(a.columns[1], b), Dot(a.columns[2], b), Dot(a.columns[3], b) };
}

