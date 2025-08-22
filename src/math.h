#pragma once

#include "lib/def.h"
#include <cmath>

struct Vec3 {
    union {
        struct { f32 x, y, z; };
        f32 data[3];
    };

    static Vec3 init(f32 x = 0.0f, f32 y = 0.0f, f32 z = 0.0f) {
        Vec3 result;
        result.x = x;
        result.y = y;
        result.z = z;
        return result;
    }

    static Vec3 zero() {
        return init(0.0f, 0.0f, 0.0f);
    }

    static Vec3 one() {
        return init(1.0f, 1.0f, 1.0f);
    }

    Vec3 operator+(Vec3 other) const {
        return init(x + other.x, y + other.y, z + other.z);
    }

    Vec3 operator-(Vec3 other) const {
        return init(x - other.x, y - other.y, z - other.z);
    }

    Vec3 operator*(f32 scalar) const {
        return init(x * scalar, y * scalar, z * scalar);
    }

    Vec3 operator/(f32 scalar) const {
        return init(x / scalar, y / scalar, z / scalar);
    }

    f32 dot(Vec3 other) const {
        return x * other.x + y * other.y + z * other.z;
    }

    Vec3 cross(Vec3 other) const {
        return init(
            y * other.z - z * other.y,
            z * other.x - x * other.z,
            x * other.y - y * other.x
        );
    }

    f32 length() const {
        return sqrtf(x * x + y * y + z * z);
    }

    f32 length_squared() const {
        return x * x + y * y + z * z;
    }

    Vec3 normalize() const {
        f32 len = length();
        if (len > 0.0f) {
            return *this / len;
        }
        return zero();
    }

    f32& operator[](i32 index) { return data[index]; }
    const f32& operator[](i32 index) const { return data[index]; }
};

struct Vec4 {
    union {
        struct { f32 x, y, z, w; };
        f32 data[4];
    };

    static Vec4 init(f32 x = 0.0f, f32 y = 0.0f, f32 z = 0.0f, f32 w = 0.0f) {
        Vec4 result;
        result.x = x;
        result.y = y;
        result.z = z;
        result.w = w;
        return result;
    }

    static Vec4 zero() {
        return init(0.0f, 0.0f, 0.0f, 0.0f);
    }

    static Vec4 one() {
        return init(1.0f, 1.0f, 1.0f, 1.0f);
    }

    static Vec4 from_vec3(Vec3 v, f32 w = 1.0f) {
        return init(v.x, v.y, v.z, w);
    }

    Vec4 operator+(Vec4 other) const {
        return init(x + other.x, y + other.y, z + other.z, w + other.w);
    }

    Vec4 operator-(Vec4 other) const {
        return init(x - other.x, y - other.y, z - other.z, w - other.w);
    }

    Vec4 operator*(f32 scalar) const {
        return init(x * scalar, y * scalar, z * scalar, w * scalar);
    }

    Vec4 operator/(f32 scalar) const {
        return init(x / scalar, y / scalar, z / scalar, w / scalar);
    }

    f32 dot(Vec4 other) const {
        return x * other.x + y * other.y + z * other.z + w * other.w;
    }

    f32 length() const {
        return sqrtf(x * x + y * y + z * z + w * w);
    }

    f32 length_squared() const {
        return x * x + y * y + z * z + w * w;
    }

    Vec4 normalize() const {
        f32 len = length();
        if (len > 0.0f) {
            return *this / len;
        }
        return zero();
    }

    Vec3 xyz() const {
        return Vec3::init(x, y, z);
    }

    f32& operator[](i32 index) { return data[index]; }
    const f32& operator[](i32 index) const { return data[index]; }
};

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

    Mat4x4() {
        for (i32 i = 0; i < 16; i++) {
            m[i] = 0.0f;
        }
        m00 = m11 = m22 = m33 = 1.0f;
    }

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

    static Mat4x4 orthographic(f32 left, f32 right, f32 bottom, f32 top, f32 near, f32 far) {
        Mat4x4 result = {};
        result.m00 = 2.0f / (right - left);
        result.m11 = 2.0f / (top - bottom);
        result.m22 = -2.0f / (far - near);
        result.m03 = -(right + left) / (right - left);
        result.m13 = -(top + bottom) / (top - bottom);
        result.m23 = -(far + near) / (far - near);
        result.m33 = 1.0f;
        return result;
    }

    static Mat4x4 perspective(f32 fovy, f32 aspect, f32 near, f32 far) {
        Mat4x4 result = {};
        f32 tan_half_fovy = tanf(fovy / 2.0f);
        
        result.m00 = 1.0f / (aspect * tan_half_fovy);
        result.m11 = 1.0f / tan_half_fovy;
        result.m22 = -(far + near) / (far - near);
        result.m23 = -(2.0f * far * near) / (far - near);
        result.m32 = -1.0f;
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
