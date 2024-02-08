//
// Created by Matei Budiu on 1/30/24.
//

#include "vecmath.hpp"
int randomInt()
{
    return std::rand();
}

int main()
{
    for (int i = 0; i < 10; i++)
    {
        ivec4 r = ivec4(randomInt(), randomInt(), randomInt(), randomInt());
        ivec4 r2 = ivec4(randomInt(), randomInt(), randomInt(), randomInt());
        ivec4 r3 = r * r2;
        //printf("%d %d %d %d\n", r3.x, r3.y, r3.z, r3.w);
    }

    mat<int, 1, 4> m = ivec4(0, 1, 4, 5);

    mat<int, 2, 2> m2 = mat<int, 2, 2>::D(0);
    m2.data[0][1] = 2;
    m2.data[1][0] = 2;

    mat<int, 2, 2> m3 = mat<int, 2, 2>::D(5);
    mat<int, 2, 2> m4 = m2 * m3;

    printf("%d %d %d %d\n", m4.data[0][0], m4.data[0][1], m4.data[1][0], m4.data[1][1]);
}

