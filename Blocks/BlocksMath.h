/*********************************************************
*
* BlocksMath.h
* IMBlocks
*
* Sean Hickey
* 2020
*
**********************************************************/

inline
f32 DistSq(v2 a, v2 b) {
  return ((a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y));
}

