//
// Created by Matei Budiu on 1/30/24.
//

#include "../vecmath.hpp"
//#include <glm/glm.hpp>
//#include <glm/ext/matrix_transform.hpp>

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

//    mat<int, 1, 4> m = ivec4(0, 1, 4, 5);
//
//    mat<int, 2, 2> m2 = mat<int, 2, 2>::D(0);
//    m2.data[0][1] = 2;
//    m2.data[1][0] = 2;
//
//    mat<int, 2, 2> m3 = mat<int, 2, 2>::D(5);
//    mat<int, 2, 2> m4 = m2 * m3;
      //glm::lookAt(glm::vec3(4,3,-3), glm::vec3(0,0,0),glm::vec3(0,1,0))


    vec4 v = vec4(1.0f, 2.0f, 3.0f, 4.0f);
    mat4 m = mat4::rotate(v.normalize());

    vec4 v2 = vec4(1.0f, 2.0f, 3.0f, -4.0f);
    mat4 m2 = mat4::rotate(v2.normalize());


    m = mat4::rotateAxis(vec3(0.0f, 0.0f, 1.0f), 0.0);
    //m = m * m2;

    printf("%f %f %f %f \n %f %f %f %f \n %f %f %f %f \n %f %f %f %f\n",
           m.data[0][0], m.data[0][1], m.data[0][2], m.data[0][3],
           m.data[1][0], m.data[1][1], m.data[1][2], m.data[1][3],
           m.data[2][0], m.data[2][1], m.data[2][2], m.data[2][3],
           m.data[3][0], m.data[3][1], m.data[3][2], m.data[3][3]);
//    printf("%d %d %d %d\n", m4.data[0][0], m4.data[0][1], m4.data[1][0], m4.data[1][1]);
}

