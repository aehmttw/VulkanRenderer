#include <functional>
#include "vecmath2.h"

// Thanks to my friend Raine Lu for providing help with generics
template<typename T>
struct Operators
{
    static inline T add(const T &a, const T &b) {return a + b;}
    static inline T sub(const T &a, const T &b) {return a - b;}
    static inline T mul(const T &a, const T &b) {return a * b;}
    static inline T div(const T &a, const T &b) {return a / b;}
};

//template<typename T, typename V>
//struct vec
//{
//    V operator+(V o) { return V::template operate<Operators<T>::add>((const V&) *this, o); }
//    V operator-(V o) { return V::template operate<Operators<T>::sub>((const V&) *this, o); }
//    V operator*(V o) { return V::template operate<Operators<T>::mul>((const V&) *this, o); }
//    V operator/(V o) { return V::template operate<Operators<T>::div>((const V&) *this, o); }
//};

template<typename T, typename V>
V vec<T, V>::operator+(V o) { return V::template operate<Operators<T>::add>((const V&) *this, o); }
template<typename T, typename V>
V vec<T, V>::operator-(V o) { return V::template operate<Operators<T>::sub>((const V&) *this, o); }
template<typename T, typename V>
V vec<T, V>::operator*(V o) { return V::template operate<Operators<T>::mul>((const V&) *this, o); }
template<typename T, typename V>
V vec<T, V>::operator/(V o) { return V::template operate<Operators<T>::div>((const V&) *this, o); }

//template<typename T>
//struct Vec2 : vec<T, Vec2<T> >
//{
//    T x;
//    T y;
//
//    Vec2(T x_, T y_)
//    {
//        this->x = x;
//        this->y = y;
//    }
//
//    template<T op(const T&, const T&)>
//    static inline Vec2<T> operate(const Vec2<T> &a, const Vec2<T> &b)
//    {
//        return Vec2
//        (
//            op(a.x, b.x),
//            op(a.y, b.y)
//        );
//    }
//};

template<typename T>
Vec2<T>::Vec2(T x, T y)
{
    this->x = x;
    this->y = y;
}

template<typename T>
template<T op(const T&, const T&)>
Vec2<T> Vec2<T>::operate(const Vec2<T> &a, const Vec2<T> &b)
{
    return Vec2(op(a.x, b.x), op(a.y, b.y));
}

template<typename T>
Vec3<T>::Vec3(T x, T y, T z)
{
    this->x = x;
    this->y = y;
    this->z = z;
}

template<typename T>
template<T op(const T&, const T&)>
Vec3<T> Vec3<T>::operate(const Vec3<T> &a, const Vec3<T> &b)
{
    return Vec3(op(a.x, b.x), op(a.y, b.y), op(a.z, b.z));
}

template<typename T>
Vec4<T>::Vec4(T x, T y, T z, T w)
{
    this->x = x;
    this->y = y;
    this->z = z;
    this->w = w;
}

template<typename T>
template<T op(const T&, const T&)>
Vec4<T> Vec4<T>::operate(const Vec4<T> &a, const Vec4<T> &b)
{
    return Vec4(op(a.x, b.x), op(a.y, b.y), op(a.z, b.z), op(a.w, b.w));
}

//template<typename T>
//struct Vec3 : vec<T, Vec3<T> >
//{
//    T x;
//    T y;
//    T z;
//
//    Vec3(T x, T y, T z)
//    {
//        this->x = x;
//        this->y = y;
//        this->z = z;
//    }
//
//    template<T op(const T&, const T&)>
//    static inline Vec3<T> operate(const Vec3<T> &a, const Vec3<T> &b)
//    {
//        return Vec3
//        (
//            op(a.x, b.x),
//            op(a.y, b.y),
//            op(a.z, b.z)
//        );
//    }
//};
//
//template<typename T>
//struct Vec4 : vec<T, Vec4<T> >
//{
//    T x;
//    T y;
//    T z;
//    T w;
//
//    Vec4(T x, T y, T z, T w)
//    {
//        this->x = x;
//        this->y = y;
//        this->z = z;
//        this->w = w;
//    }
//
//    template<T op(const T&, const T&)>
//    static inline Vec4<T> operate(const Vec4<T> &a, const Vec4<T> &b)
//    {
//        return Vec4
//        (
//            op(a.x, b.x),
//            op(a.y, b.y),
//            op(a.z, b.z),
//            op(a.w, b.w)
//        );
//    }
//};

typedef Vec2<float> vec2;
typedef Vec3<float> vec3;
typedef Vec4<float> vec4;
typedef Vec2<double> dvec2;
typedef Vec3<double> dvec3;
typedef Vec4<double> dvec4;

typedef Vec2<uint8_t> u8vec2;
typedef Vec3<uint8_t> u8vec3;
typedef Vec4<uint8_t> u8vec4;
typedef Vec2<int8_t> i8vec2;
typedef Vec3<int8_t> i8vec3;
typedef Vec4<int8_t> i8vec4;

typedef Vec2<uint16_t> u16vec2;
typedef Vec3<uint16_t> u16vec3;
typedef Vec4<uint16_t> u16vec4;
typedef Vec2<int16_t> i16vec2;
typedef Vec3<int16_t> i16vec3;
typedef Vec4<int16_t> i16vec4;

typedef Vec2<uint32_t> u32vec2;
typedef Vec3<uint32_t> u32vec3;
typedef Vec4<uint32_t> u32vec4;
typedef Vec2<int32_t> i32vec2;
typedef Vec3<int32_t> i32vec3;
typedef Vec4<int32_t> i32vec4;

typedef Vec2<uint64_t> u64vec2;
typedef Vec3<uint64_t> u64vec3;
typedef Vec4<uint64_t> u64vec4;
typedef Vec2<int64_t> i64vec2;
typedef Vec3<int64_t> i64vec3;
typedef Vec4<int64_t> i64vec4;

//int randomInt()
//{
//    return std::rand();
//}
//
//int main()
//{
//    for (int i = 0; i < 10; i++)
//    {
//        vec4 r(randomInt(), randomInt(), randomInt(), randomInt());
//        vec4 r2(randomInt(), randomInt(), randomInt(), randomInt());
//        vec4 r3 = r * r2;
//        printf("%f %f %f %f\n", r3.x, r3.y, r3.z, r3.w);
//    }
//}


