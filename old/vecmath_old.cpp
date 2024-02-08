#include <functional>

template<typename T>
struct Operators
{
    static inline T add(T a, T b) {return a + b;}
    static inline T sub(T a, T b) {return a - b;}
    static inline T mul(T a, T b) {return a * b;}
    static inline T div(T a, T b) {return a / b;}
};

template<typename T>
struct vec
{
    struct Operators<T> ops;

    struct vec2 : vec
    {
        T data[2];

        vec2(T x, T y)
        {
            this->data[0] = x;
            this->data[1] = y;
        }

        vec2 operate(T op(T, T), vec2 o)
        {
            return vec2
            (
                op(this->data[0], o.data[0]),
                op(this->data[1], o.data[1])
            );
        }

        vec2 operator+(vec2 o) { return operate(ops.add, o); }
        vec2 operator-(vec2 o) { return operate(ops.sub, o); }
        vec2 operator*(vec2 o) { return operate(ops.mul, o); }
        vec2 operator/(vec2 o) { return operate(ops.div, o); }
    };

    struct vec3 : vec
    {
        T data[3];

        vec3(T x, T y, T z)
        {
            this->data[0] = x;
            this->data[1] = y;
            this->data[2] = z;
        }

        vec3 operate(T op(T, T), vec3 o)
        {
            return vec3
            (
                op(this->data[0], o.data[0]),
                op(this->data[1], o.data[1]),
                op(this->data[2], o.data[2])
            );
        }

        vec3 operator+(vec3 o) { return operate(ops.add, o); }
        vec3 operator-(vec3 o) { return operate(ops.sub, o); }
        vec3 operator*(vec3 o) { return operate(ops.mul, o); }
        vec3 operator/(vec3 o) { return operate(ops.div, o); }
    };

    struct vec4 : vec
    {
        T data[4];

        vec4(T x, T y, T z, T w)
        {
            this->data[0] = x;
            this->data[1] = y;
            this->data[2] = z;
            this->data[3] = w;
        }

        vec4 operate(T op(T, T), vec4 o)
        {
            return vec4
            (
                op(this->data[0], o.data[0]),
                op(this->data[1], o.data[1]),
                op(this->data[2], o.data[2]),
                op(this->data[3], o.data[3])
            );
        }

        vec4 operator+(vec4 o) { return operate(ops.add, o); }
        vec4 operator-(vec4 o) { return operate(ops.sub, o); }
        vec4 operator*(vec4 o) { return operate(ops.mul, o); }
        vec4 operator/(vec4 o) { return operate(ops.div, o); }
    };

    //virtual inline V operate(T op(T, T), V o);
};

typedef vec<float>::vec2 vec2;
typedef vec<float>::vec3 vec3;
typedef vec<float>::vec4 vec4;
typedef vec<double>::vec2 dvec2;
typedef vec<double>::vec3 dvec3;
typedef vec<double>::vec4 dvec4;

typedef vec<uint8_t>::vec2 u8vec2;
typedef vec<uint8_t>::vec3 u8vec3;
typedef vec<uint8_t>::vec4 u8vec4;
typedef vec<int8_t>::vec2 i8vec2;
typedef vec<int8_t>::vec3 i8vec3;
typedef vec<int8_t>::vec4 i8vec4;

typedef vec<uint16_t>::vec2 u16vec2;
typedef vec<uint16_t>::vec3 u16vec3;
typedef vec<uint16_t>::vec4 u16vec4;
typedef vec<int16_t>::vec2 i16vec2;
typedef vec<int16_t>::vec3 i16vec3;
typedef vec<int16_t>::vec4 i16vec4;

typedef vec<uint32_t>::vec2 u32vec2;
typedef vec<uint32_t>::vec3 u32vec3;
typedef vec<uint32_t>::vec4 u32vec4;
typedef vec<int32_t>::vec2 i32vec2;
typedef vec<int32_t>::vec3 i32vec3;
typedef vec<int32_t>::vec4 i32vec4;

typedef vec<uint64_t>::vec2 u64vec2;
typedef vec<uint64_t>::vec3 u64vec3;
typedef vec<uint64_t>::vec4 u64vec4;
typedef vec<int64_t>::vec2 i64vec2;
typedef vec<int64_t>::vec3 i64vec3;
typedef vec<int64_t>::vec4 i64vec4;

int randomInt()
{
    return std::rand();
}

int main()
{
    for (int i = 0; i < 10; i++)
    {
        vec4 r = vec4(randomInt(), randomInt(), randomInt(), randomInt());
        vec4 r2 = vec4(randomInt(), randomInt(), randomInt(), randomInt());
        vec4 r3 = r * r2;
        printf("%f %f %f %f\n", r3.data[0], r3.data[1], r3.data[2], r3.data[3]);
    }
}


