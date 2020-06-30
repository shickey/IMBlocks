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
