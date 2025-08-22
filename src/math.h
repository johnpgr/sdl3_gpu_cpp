#pragma once

#include "lib/def.h"
#include <cmath>

struct Mat4x4 {
    union {
        f32 m[16];
        f32 mat[4][4];
        struct {
            f32 m00, m01, m02, m03;
            f32 m10, m11, m12, m13;
            f32 m20, m21, m22, m23;
            f32 m30, m31, m32, m33;
        };
    };

    Mat4x4() { *this = identity(); }

    Mat4x4(const f32 data[16]) {
        for (i32 i = 0; i < 16; i++) {
            m[i] = data[i];
        }
    }

    static Mat4x4 identity() {
        Mat4x4 result = {};
        result.m00 = result.m11 = result.m22 = result.m33 = 1.0f;
        return result;
    }

    static Mat4x4 scale(f32 x, f32 y, f32 z) {
        Mat4x4 result = {};
        result.m00 = x;
        result.m11 = y;
        result.m22 = z;
        result.m33 = 1.0f;
        return result;
    }

    static Mat4x4 rotation_z(f32 radians) {
        Mat4x4 result = identity();
        f32 c = cosf(radians);
        f32 s = sinf(radians);

        result.m00 = c;
        result.m01 = -s;
        result.m10 = s;
        result.m11 = c;

        return result;
    }

    static Mat4x4 translation(f32 x, f32 y, f32 z) {
        Mat4x4 result = identity();
        result.m03 = x;
        result.m13 = y;
        result.m23 = z;
        return result;
    }

    Mat4x4 operator*(Mat4x4 other) const {
        Mat4x4 result = {};

        for (i32 row = 0; row < 4; row++) {
            for (i32 col = 0; col < 4; col++) {
                for (i32 k = 0; k < 4; k++) {
                    result.mat[row][col] += mat[row][k] * other.mat[k][col];
                }
            }
        }

        return result;
    }

    f32& operator[](i32 index) { return m[index]; }
    const f32& operator[](i32 index) const { return m[index]; }

    f32* data() { return m; }
    const f32* data() const { return m; }
};
