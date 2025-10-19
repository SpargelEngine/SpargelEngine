#pragma once

#include "spargel/math/function.h"

namespace spargel::math {

    template <typename T>
    struct Vector2 {
        T x;
        T y;

        T dot(Vector2 const& other) const { return x * other.x + y * other.y; }
        T length() const { return math::sqrt(dot(*this)); }
    };
    template <typename T>
    constexpr Vector2<T> operator-(Vector2<T> lhs, Vector2<T> rhs) {
        return {lhs.x - rhs.x, lhs.y - rhs.y};
    }

    template <typename T>
    struct Vector3 {
        T x;
        T y;
        T z;

        T dot(Vector3 const& other) const {
            return x * other.x + y * other.y + z * other.z;
        }

        Vector3 cross(Vector3 const& other) const {
            return Vector3(y * other.z - z * other.y, z * other.x - x * other.z,
                           x * other.y - y * other.x);
        }

        T length() const { return math::sqrt(dot(*this)); }

        // TODO: What's the behaviour in edge cases?
        Vector3 normalize() const {
            T l = length();
            return Vector3(x / l, y / l, z / l);
        }

        Vector3& operator+=(Vector3 const& other) {
            x += other.x;
            y += other.y;
            z += other.z;
            return *this;
        }
    };

    template <typename T>
    struct Vector4 {
        T x;
        T y;
        T z;
        T w;

        T dot(Vector4 const& other) const {
            return x * other.x + y * other.y + z * other.z + w * other.w;
        }

        T length() const { return math::sqrt(dot(*this)); }

        // TODO: What's the behaviour in edge cases?
        Vector4 normalize() const {
            T l = length();
            return Vector3(x / l, y / l, z / l, w / l);
        }

        Vector4& operator+=(Vector4 const& other) {
            x += other.x;
            y += other.y;
            z += other.z;
            w += other.w;
            return *this;
        }
    };

    template <typename T>
    Vector3<T> operator+(Vector3<T> const& lhs, Vector3<T> const& rhs) {
        return Vector3<T>(lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z);
    }

    template <typename T>
    Vector3<T> operator-(Vector3<T> const& lhs, Vector3<T> const& rhs) {
        return Vector3<T>(lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z);
    }

    template <typename T>
    Vector4<T> operator+(Vector4<T> const& lhs, Vector4<T> const& rhs) {
        return Vector4<T>(lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z,
                          lhs.w + rhs.w);
    }

    template <typename T>
    Vector4<T> operator-(Vector4<T> const& lhs, Vector4<T> const& rhs) {
        return Vector3<T>(lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z,
                          lhs.w - rhs.w);
    }

    using Vector2f = Vector2<float>;
    using Vector3f = Vector3<float>;
    using Vector4f = Vector4<float>;

}  // namespace spargel::math
