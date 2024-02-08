
// Thanks to my friend Raine Lu for providing help with generics
template<typename T, typename V>
struct vec
{
    V operator+(V o);
    V operator-(V o);
    V operator*(V o);
    V operator/(V o);
};

template<typename T>
struct Vec2 : vec<T, Vec2<T> >
{
    T x;
    T y;

    Vec2(T x, T y);

    template<T op(const T&, const T&)>
    Vec2<T> operate(const Vec2<T> &a, const Vec2<T> &b);
};

template<typename T>
struct Vec3 : vec<T, Vec3<T> >
{
    T x;
    T y;
    T z;

    Vec3(T x, T y, T z);

    template<T op(const T&, const T&)>
    Vec3<T> operate(const Vec3<T> &a, const Vec3<T> &b);
};

template<typename T>
struct Vec4 : vec<T, Vec4<T> >
{
    T x;
    T y;
    T z;
    T w;

    Vec4(T x, T y, T z, T w);

    template<T op(const T&, const T&)>
    Vec4<T> operate(const Vec4<T> &a, const Vec4<T> &b);
};

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



