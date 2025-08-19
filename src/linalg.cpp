#include <cmath>

namespace igfx {
    inline v2f32 v2f32::lerp(v2f32 a, v2f32 b, f32 t) {
        return {std::fmaf(b.x - a.x, t, a.x), std::fmaf(b.y - a.y, t, a.y)};
    }
    
    inline v2f32 v2f32::operator+(v2f32 rhs) const {
        return {this->x + rhs.x, this->y + rhs.y};
    }
    
    inline v2f32 v2f32::operator-(v2f32 rhs) const {
        return {this->x - rhs.x, this->y - rhs.y};
    }
    
    inline v2f32 v2f32::operator*(v2f32 rhs) const {
        return {this->x * rhs.x, this->y * rhs.y};
    }
    
    inline v2f32 v2f32::operator/(v2f32 rhs) const {
        return {this->x / rhs.x, this->y / rhs.y};
    }
    
    inline void v2f32::operator+=(v2f32 rhs) {
        *this = {this->x + rhs.x, this->y + rhs.y};
    }
    
    inline void v2f32::operator-=(v2f32 rhs) {
        *this = {this->x - rhs.x, this->y - rhs.y};
    }
    
    inline void v2f32::operator*=(v2f32 rhs) {
        *this = {this->x * rhs.x, this->y * rhs.y};
    }
    
    inline void v2f32::operator/=(v2f32 rhs) {
        *this = {this->x / rhs.x, this->y / rhs.y};
    }
    
    inline v2f32 v2f32::operator*(f32 rhs) const {
        return {this->x * rhs, this->y * rhs};
    }
    
    inline v2f32 v2f32::operator/(f32 rhs) const {
        return {this->x / rhs, this->y / rhs};
    }
    
    inline void v2f32::operator*=(f32 rhs) {
        *this = {this->x * rhs, this->y * rhs};
    }
    
    inline void v2f32::operator/=(f32 rhs) {
        *this = {this->x / rhs, this->y / rhs};
    }
    
    const v2f32 v2f32::zero = {0, 0};
    const v2f32 v2f32::one = {1, 1};
    
    inline static v2f32 operator*(f32 lhs, v2f32 rhs) {
        return {lhs * rhs.x, lhs * rhs.y};
    }
    
    inline static v2f32 operator/(f32 lhs, v2f32 rhs) {
        return {lhs / rhs.x, lhs / rhs.y};
    }
}
