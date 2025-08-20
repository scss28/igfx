#pragma once

#include "igfx/nums.h"
#include <cmath>

namespace igfx {
    struct vec2 {
        f32 x;
        f32 y;
    
        static const vec2 zero;
        static const vec2 one;

        inline vec2 lerp(vec2 a, vec2 b, f32 t) {
            return {std::fmaf(b.x - a.x, t, a.x), std::fmaf(b.y - a.y, t, a.y)};
        }
        
        inline vec2 operator+(vec2 rhs) const {
            return {this->x + rhs.x, this->y + rhs.y};
        }
        
        inline vec2 operator-(vec2 rhs) const {
            return {this->x - rhs.x, this->y - rhs.y};
        }
        
        inline vec2 operator*(vec2 rhs) const {
            return {this->x * rhs.x, this->y * rhs.y};
        }
        
        inline vec2 operator/(vec2 rhs) const {
            return {this->x / rhs.x, this->y / rhs.y};
        }
        
        inline void operator+=(vec2 rhs) {
            *this = {this->x + rhs.x, this->y + rhs.y};
        }
        
        inline void operator-=(vec2 rhs) {
            *this = {this->x - rhs.x, this->y - rhs.y};
        }
        
        inline void operator*=(vec2 rhs) {
            *this = {this->x * rhs.x, this->y * rhs.y};
        }
        
        inline void operator/=(vec2 rhs) {
            *this = {this->x / rhs.x, this->y / rhs.y};
        }
    
        inline vec2 operator*(f32 rhs) const {
            return {this->x * rhs, this->y * rhs};
        }

        inline vec2 operator/(f32 rhs) const {
            return {this->x / rhs, this->y / rhs};
        }

        inline void operator*=(f32 rhs) {
            *this = {this->x * rhs, this->y * rhs};
        }

        inline void operator/=(f32 rhs) {
            *this = {this->x / rhs, this->y / rhs};
        }
    };

    inline static vec2 operator*(f32 lhs, vec2 rhs) {
        return {lhs * rhs.x, lhs * rhs.y};
    }

    inline static vec2 operator/(f32 lhs, vec2 rhs) {
        return {lhs / rhs.x, lhs / rhs.y};
    }
}
