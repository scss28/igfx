#pragma once
#include "igfx/nums.h"

namespace igfx {
struct v2f32 {
    f32 x;
    f32 y;

    static const v2f32 zero;
    static const v2f32 one;

    inline static v2f32 lerp(v2f32 a, v2f32 b, f32 t);
    
    inline v2f32 operator+(v2f32 rhs) const;
    inline v2f32 operator-(v2f32 rhs) const;
    inline v2f32 operator*(v2f32 rhs) const;
    inline v2f32 operator/(v2f32 rhs) const;
    
    inline void operator+=(v2f32 rhs);
    inline void operator-=(v2f32 rhs);
    inline void operator*=(v2f32 rhs);
    inline void operator/=(v2f32 rhs);

    inline v2f32 operator*(f32 rhs) const;
    inline v2f32 operator/(f32 rhs) const;
    inline void operator*=(f32 rhs);
    inline void operator/=(f32 rhs);
};
}
