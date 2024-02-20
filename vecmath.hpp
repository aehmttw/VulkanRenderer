#include <functional>
#include <cmath>

// Thanks to my friend Raine Lu for providing help with generics
template<typename T>
struct Operators
{
    static inline T add(const T &a, const T &b) {return a + b;}
    static inline T sub(const T &a, const T &b) {return a - b;}
    static inline T mul(const T &a, const T &b) {return a * b;}
    static inline T div(const T &a, const T &b) {return a / b;}
};

template<typename T, typename V>
struct vec
{
    V operator+(V o) { return V::template operate<Operators<T>::add>((const V&) *this, o); }
    V operator-(V o) { return V::template operate<Operators<T>::sub>((const V&) *this, o); }
    V operator*(V o) { return V::template operate<Operators<T>::mul>((const V&) *this, o); }
    V operator/(V o) { return V::template operate<Operators<T>::div>((const V&) *this, o); }

    V operator*(T o) { return V::template operate<Operators<T>::mul>((const V&) *this, o); }
    V operator/(T o) { return V::template operate<Operators<T>::div>((const V&) *this, o); }
};

template<typename T, int x, int y>
struct Mat;

template<typename T>
struct Vec2 : vec<T, Vec2<T> >
{
    T x;
    T y;

    Vec2()
    {
        x = 0;
        y = 0;
    }

    Vec2(T x, T y)
    {
        this->x = x;
        this->y = y;
    }

    Vec2<T>(Mat<T, 1, 2> m)
    {
        this->x = m[0][0];
        this->y = m[0][1];
    }

    T length()
    {
        return sqrt(this->x * this->x + this->y * this->y);
    }

    Vec2<T> normalize()
    {
        return *this / this->length();
    }

    T dot(Vec2<T>o)
    {
        return this->x * o.x + this->y * o.y;
    }

    template<T op(const T&, const T&)>
    static inline Vec2<T> operate(Vec2<T>a, Vec2<T>b)
    {
        return Vec2
        (
            op(a.x, b.x),
            op(a.y, b.y)
        );
    }

    template<T op(const T&, const T&)>
    static inline Vec2<T> operate(Vec2<T>a, T b)
    {
        return Vec2
        (
            op(a.x, b),
            op(a.y, b)
        );
    }
};

template<typename T>
struct Vec3 : vec<T, Vec3<T> >
{
    T x;
    T y;
    T z;

    Vec3()
    {
        x = 0;
        y = 0;
        z = 0;
    }

    Vec3(T x, T y, T z)
    {
        this->x = x;
        this->y = y;
        this->z = z;
    }

    Vec3(Vec2<T>xy, T z)
    {
        this->x = xy.x;
        this->y = xy.y;
        this->z = z;
    }

    Vec3(T x, Vec2<T>yz)
    {
        this->x = x;
        this->y = yz.x;
        this->z = yz.y;
    }

    Vec3<T>(Mat<T, 1, 3> m)
    {
        this->x = m[0][0];
        this->y = m[0][1];
        this->z = m[0][2];
    }

    Vec2<T> xy()
    {
        return Vec2<T>(this->x, this->y);
    }

    T length()
    {
        return sqrt(this->x * this->x + this->y * this->y + this->z * this->z);
    }

    Vec3<T> normalize()
    {
        return *this / this->length();
    }

    Vec3<T> cross(Vec3<T>o)
    {
        return Vec3(this->y * o.z - this->z * o.y, o.x * this->z - o.z * this->x, this->x * o.y - this->y * o.x);
    }

    T dot(Vec3<T>o)
    {
        return this->x * o.x + this->y * o.y + this->z * o.z;
    }

    template<T op(const T&, const T&)>
    static inline Vec3<T> operate(Vec3<T>a, Vec3<T>b)
    {
        return Vec3
        (
            op(a.x, b.x),
            op(a.y, b.y),
            op(a.z, b.z)
        );
    }

    template<T op(const T&, const T&)>
    static inline Vec3<T> operate(Vec3<T>a, const T b)
    {
        return Vec3
        (
            op(a.x, b),
            op(a.y, b),
            op(a.z, b)
        );
    }
};

template<typename T>
struct Vec4 : vec<T, Vec4<T> >
{
    T x;
    T y;
    T z;
    T w;

    Vec4()
    {
        x = 0;
        y = 0;
        z = 0;
        w = 0;
    }

    Vec4(T x, T y, T z, T w)
    {
        this->x = x;
        this->y = y;
        this->z = z;
        this->w = w;
    }

    Vec4(Vec3<T>xyz, T w)
    {
        this->x = xyz.x;
        this->y = xyz.y;
        this->z = xyz.z;
        this->w = w;
    }

    Vec4(T x, Vec3<T>yzw)
    {
        this->x = x;
        this->y = yzw.x;
        this->z = yzw.y;
        this->w = yzw.z;
    }

    Vec4(Vec2<T>xy, Vec2<T>zw)
    {
        this->x = xy.x;
        this->y = xy.y;
        this->z = zw.x;
        this->w = zw.y;
    }

    Vec4(Vec2<T>xy, T z, T w)
    {
        this->x = xy.x;
        this->y = xy.y;
        this->z = z;
        this->w = w;
    }

    Vec4(T x, Vec2<T>xy, T w)
    {
        this->x = x;
        this->y = xy.x;
        this->z = xy.y;
        this->w = w;
    }

    Vec4(T x, T y, Vec2<T>xy)
    {
        this->x = x;
        this->y = y;
        this->z = xy.x;
        this->w = xy.y;
    }

    Vec4<T>(Mat<T, 1, 4> m)
    {
        this->x = m[0][0];
        this->y = m[0][1];
        this->z = m[0][2];
        this->w = m[0][3];
    }

    Vec2<T> xy()
    {
        return Vec2<T>(this->x, this->y);
    }

    Vec3<T> xyz()
    {
        return Vec3<T>(this->x, this->y, this->z);
    }

    T length()
    {
        return sqrt(this->x * this->x + this->y * this->y + this->z * this->z + this->w * this->w);
    }

    Vec4<T> normalize()
    {
        return *this / this->length();
    }

    T dot(Vec4<T>o)
    {
        return this->x * o.x + this->y * o.y + this->z * o.z + this->w * o.w;
    }

    template<T op(const T&, const T&)>
    static inline Vec4<T> operate(Vec4<T>a, Vec4<T>b)
    {
        return Vec4
        (
            op(a.x, b.x),
            op(a.y, b.y),
            op(a.z, b.z),
            op(a.w, b.w)
        );
    }

    template<T op(const T&, const T&)>
    static inline Vec4<T> operate(Vec4<T>a, const T b)
    {
        return Vec4
        (
            op(a.x, b),
            op(a.y, b),
            op(a.z, b),
            op(a.w, b)
        );
    }
};

template<typename T, int x, int y>
struct Mat
{
    T data[x][y];

    Mat<T, 1, 2>(Vec2<T> v)
    {
        data[0][0] = v.x;
        data[0][1] = v.y;
    }

    Mat<T, 1, 3>(Vec3<T> v)
    {
        data[0][0] = v.x;
        data[0][1] = v.y;
        data[0][2] = v.z;
    }

    Mat<T, 1, 4>(Vec4<T> v)
    {
        data[0][0] = v.x;
        data[0][1] = v.y;
        data[0][2] = v.z;
        data[0][3] = v.w;
    }

    Mat<T, x, y>()
    {
        for (int i = 0; i < x; i++)
        {
            for (int j = 0; j < y; j++)
            {
                data[i][j] = 0;
            }
        }
    }

    Mat<T, 2, 2>(T a, T b, T c, T d)
    {
        data[0][0] = a;
        data[0][1] = b;
        data[1][0] = c;
        data[1][1] = d;
    }

    Mat<T, 2, 2>(Vec2<T> a, Vec2<T> const &b)
    {
        data[0][0] = a.x;
        data[0][1] = a.y;
        data[1][0] = b.x;
        data[1][1] = b.y;
    }

    Mat<T, 3, 3>(T a, T b, T c, T d, T e, T f, T g, T h, T i)
    {
        data[0][0] = a;
        data[0][1] = b;
        data[0][2] = c;

        data[1][0] = d;
        data[1][1] = e;
        data[1][2] = f;

        data[2][0] = g;
        data[2][1] = h;
        data[2][2] = i;
    }

    Mat<T, 3, 3>(Vec3<T> a, Vec3<T> b, Vec3<T> c)
    {
        data[0][0] = a.x;
        data[0][1] = a.y;
        data[0][2] = a.z;

        data[1][0] = b.x;
        data[1][1] = b.y;
        data[1][2] = b.z;

        data[2][0] = c.x;
        data[2][1] = c.y;
        data[2][2] = c.z;
    }

    Mat<T, 4, 4>(T a, T b, T c, T d, T e, T f, T g, T h, T i, T j, T k, T l, T m, T n, T o, T p)
    {
        data[0][0] = a;
        data[0][1] = b;
        data[0][2] = c;
        data[0][3] = d;

        data[1][0] = e;
        data[1][1] = f;
        data[1][2] = g;
        data[1][3] = h;

        data[2][0] = i;
        data[2][1] = j;
        data[2][2] = k;
        data[2][3] = l;

        data[3][0] = m;
        data[3][1] = n;
        data[3][2] = o;
        data[3][3] = p;
    }

    Mat<T, 4, 4>(Vec4<T> a, Vec4<T> b, Vec4<T> c, Vec4<T> d)
    {
        data[0][0] = a.x;
        data[0][1] = a.y;
        data[0][2] = a.z;
        data[0][3] = a.w;

        data[1][0] = b.x;
        data[1][1] = b.y;
        data[1][2] = b.z;
        data[1][3] = b.w;

        data[2][0] = c.x;
        data[2][1] = c.y;
        data[2][2] = c.z;
        data[2][3] = c.w;

        data[3][0] = d.x;
        data[3][1] = d.y;
        data[3][2] = d.z;
        data[3][3] = d.w;
    }

    T *operator[](size_t e)
    {
        return data[e];
    }

    static inline Mat<T, x, y> Z()
    {
        return Mat<T, x, y>();
    }

    static inline Mat<T, x, y> I()
    {
        Mat m = Mat<T, x, y>();
        for (int i = 0; i < std::min(x, y); i++)
        {
            m[i][i] = 1;
        }
        return m;
    }

    static inline Mat<T, x, y> D(T el)
    {
        Mat m = Mat();
        for (int i = 0; i < std::min(x, y); i++)
        {
            m[i][i] = el;
        }
        return m;
    }

    static inline Mat<T, 4, 4> translation(T tx, T ty, T tz)
    {
        Mat<T, 4, 4> m = Mat::I();
        m[3][0] = tx;
        m[3][1] = ty;
        m[3][2] = tz;
        return m;
    }

    static inline Mat<T, 4, 4> translation(Vec3<T> trans)
    {
        return translation(trans.x, trans.y, trans.z);
    }

    static inline Mat<T, 4, 4> scale(T sx, T sy, T sz)
    {
        Mat<T, 4, 4> m = Mat<T, 4, 4>();
        m[0][0] = sx;
        m[1][1] = sy;
        m[2][2] = sz;
        m[3][3] = 1;
        return m;
    }

    static inline Mat<T, 4, 4> scale(Vec3<T> s)
    {
        return scale(s.x, s.y, s.z);
    }

    // Quaternion rotation
    // Adapted from https://www.euclideanspace.com/maths/geometry/rotations/conversions/quaternionToMatrix/index.htm
    static inline Mat<T, 4, 4> rotate(T rx, T ry, T rz, T rw)
    {
        Mat<T, 4, 4> m = Mat<T, 4, 4>();
        m[0][0] = 1 - 2 * ry * ry - 2 * rz * rz;
        m[0][1] = 2 * rx * ry + 2 * rz * rw;
        m[0][2] = 2 * rx * rz - 2 * ry * rw;

        m[1][0] = 2 * rx * ry - 2 * rz * rw;
        m[1][1] = 1 - 2 * rx * rx - 2 * rz * rz;
        m[1][2] = 2 * ry * rz + 2 * rx * rw;

        m[2][0] = 2 * rx * rz + 2 * ry * rw;
        m[2][1] = 2 * ry * rz - 2 * rx * rw;
        m[2][2] = 1 - 2 * rx * rx - 2 * ry * ry;

        m[3][3] = 1;
        return m;
    }

    static inline Mat<T, 4, 4> rotate(Vec4<T> r)
    {
        return rotate(r.x, r.y, r.z, r.w);
    }

    // Quaternion rotation
    // Adapted from https://www.euclideanspace.com/maths/geometry/rotations/conversions/angleToQuaternion/index.htm
    static inline Mat<T, 4, 4> rotateAxis(Vec3<T> axis, T angle)
    {
        return rotate(axis.x * sin(angle / 2), axis.y * sin(angle / 2), axis.z * sin(angle / 2), cos(angle / 2));
    }

    // Adapted from https://github.com/g-truc/glm/blob/4eb3fe1d7d8fd407cc7ccfa801a0311bb7dd281c/glm/ext/matrix_transform.inl#L153
    static inline Mat<T, 4, 4> lookAt(Vec3<T> eyePos, Vec3<T> centerLookAtPos, Vec3<T> upVec)
    {
        Vec3<T> f = (centerLookAtPos - eyePos).normalize();
        Vec3<T> up = upVec.normalize();
        Vec3<T> s = f.cross(up).normalize();
        Vec3<T> u = s.cross(f);

        return Mat<T, 4, 4>(s.x, u.x, -f.x, 0, s.y, u.y, -f.y, 0, s.z, u.z, -f.z, 0, -s.dot(eyePos), -u.dot(eyePos),
                            f.dot(eyePos), 1);
    }

    // Adapted from https://registry.khronos.org/OpenGL-Refpages/gl2.1/xhtml/gluPerspective.xml
    static inline Mat<T, 4, 4> perspective(T aspectRatio, T verticalFOV, T nearPlane, T farPlane)
    {
        Mat<T, 4, 4> m;
        T f = 1 / tan(verticalFOV / 2);
        m[0][0] = f / aspectRatio;
        m[1][1] = -f;
        m[2][2] = (nearPlane + farPlane) / (nearPlane - farPlane);
        m[3][2] = 2 * farPlane * nearPlane / (nearPlane - farPlane);
        m[2][3] = -1;
        return m;
    }

    // Adapted from http://www.terathon.com/gdc07_lengyel.pdf
    static inline Mat<T, 4, 4> infinitePerspective(T aspectRatio, T verticalFOV, T nearPlane)
    {
        Mat<T, 4, 4> m;
        T f = 1 / tan(verticalFOV / 2);
        m[0][0] = f / aspectRatio;
        m[1][1] = -f;
        m[2][2] = -1;
        m[3][2] = -2 * nearPlane;
        m[2][3] = -1;
        return m;
    }

    template<int z>
    Mat<T, z, y> operator*(Mat<T, z, x> m)
    {
        Mat<T, z, y> result;
        for (int iy = 0; iy < y; iy++)
        {
            for (int iz = 0; iz < z; iz++)
            {
                for (int ix = 0; ix < x; ix++)
                {
                    result[iz][iy] += (*this)[ix][iy] * m[iz][ix];
                }
            }
        }
        return result;
    }

    Vec4<T> operator*(Vec4<T> v)
    {
        return Vec4<T>((*this) * Mat<T, 1, 4>(v));
    }

    Vec3<T> operator*(Vec3<T> v)
    {
        return Vec4<T>((*this) * Mat<T, 1, 3>(v));
    }

    Vec2<T> operator*(Vec2<T> v)
    {
        return Vec4<T>((*this) * Mat<T, 1, 2>(v));
    }

    void print()
    {
        printf("[");
        for (int i = 0; i < x; i++)
        {
            for (int j = 0; j < y; j++)
            {
                printf("%f ", data[i][j]);
            }

            if (i == x - 1)
                printf("]");
            printf("\n");
        }
    }
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

typedef Vec2<uint32_t> uvec2;
typedef Vec3<uint32_t> uvec3;
typedef Vec4<uint32_t> uvec4;
typedef Vec2<int32_t> i32vec2;
typedef Vec3<int32_t> i32vec3;
typedef Vec4<int32_t> i32vec4;

typedef Vec2<int> ivec2;
typedef Vec3<int> ivec3;
typedef Vec4<int> ivec4;

typedef Vec2<uint64_t> u64vec2;
typedef Vec3<uint64_t> u64vec3;
typedef Vec4<uint64_t> u64vec4;
typedef Vec2<int64_t> i64vec2;
typedef Vec3<int64_t> i64vec3;
typedef Vec4<int64_t> i64vec4;

typedef Mat<float, 4, 4> mat4;
typedef Mat<float, 3, 3> mat3;
typedef Mat<float, 2, 2> mat2;

