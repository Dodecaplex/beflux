// :: util.h :: 12/6/2014
// Tony Chiodo - http://dodecaplex.net
////////////////////////////////////////////////////////////////////////////////

#pragma once

typedef unsigned char byte;
typedef unsigned int  uint;
typedef unsigned long ulong;

namespace Dodecaplex {

template <class T0, class T1>
void swap(T0 *a, T1 *b) {
  T0 temp = *a;
  *a = *b;
  *b = temp;
}

template <class T0, class T1>
T0 min(T0 a, T1 b) {
  return (a < b) ? a : b;
}

template <class T0, class T1>
T0 max(T0 a, T1 b) {
  return (a > b) ? a : b;
}

template <class T0, class T1, class T2>
T0 clamp(T0 x, T1 min, T2 max) {
  return (x < min) ? min : ((x > max) ? max : x);
}

template <class T0, class T1, class T2, class T3>
T0 clampindex(const T0 * const array, T1 x, T2 min, T3 max) {
  return array[clamp(x, min, max)];
}

template <class T0, class T1, class T2>
T0 wrap(T0 x, T1 min, T2 max) {
  if (max < min)
    swap(&max, &min);

  T0 n = max - min;
  if (n == 0)
    return min;

  if (x < min)
    x += n * ((max - x) / n);

  return min + (x - min) % n;
}

template <class T0, class T1, class T2, class T3>
T0 wrapindex(const T0 * const array, T1 x, T2 min, T3 max) {
  return array[wrap(x, min, max)];
}

} // namespace Dodecaplex
