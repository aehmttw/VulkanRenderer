#include "../vecmath.hpp"
#include <string>
#include <iostream>
#include <fstream>

#ifndef STB_IMAGE_IMPLEMENTATION
    #define STB_IMAGE_IMPLEMENTATION
#endif

#ifndef STB_IMAGE_WRITE_IMPLEMENTATION
    #define STB_IMAGE_WRITE_IMPLEMENTATION
#endif
#include "../libs/stb_image.h"
#include "../libs/stb_image_write.h"

int posX = 0;
int negX = 1;
int posY = 2;
int negY = 3;
int posZ = 4;
int negZ = 5;

struct CubeSample
{
    int face;
    vec2 point;

    vec3 toVec3()
    {
        if (face == posX)
            return vec3(0.5, 0.5f - point.x, 0.5f - point.y);
        else if (face == negX)
            return vec3(-0.5, 0.5f - point.x, point.y - 0.5f);
        else if (face == posY)
            return vec3(point.y - 0.5f, 0.5, point.x - 0.5f);
        else if (face == negY)
            return vec3(point.y - 0.5f, -0.5, 0.5f - point.x);
        else if (face == posZ)
            return vec3(point.y - 0.5f, 0.5f - point.x, 0.5);
        else if (face == negZ)
            return vec3(0.5f - point.y, 0.5f - point.x, -0.5);

        printf("bad face\n");
        abort();
    }

};

struct Tex
{
    int res;
    unsigned char* data;
    bool exponent = true;

    Tex(std::string src)
    {
        int w;
        int h;
        int comp;
        data = stbi_load(src.c_str(), &w, &h, &comp, STBI_rgb_alpha);
        assert (h == 6 * w);
        res = w;
    }

    Tex(int res, bool exp)
    {
        this->res = res;
        this->exponent = exp;
        this->data = new unsigned char[res * res * 6 * 4];
    }

    vec3 sample(CubeSample c)
    {
        int faceOffset = res * res * c.face;
        int x = (int) (c.point.x * res);
        int y = (int) (c.point.y * res);
        int index = faceOffset + x * res + y;

        if (exponent)
        {
            if (data[index * 4] == 0 && data[index * 4 + 1] == 0 && data[index * 4 + 2] == 0 && data[index * 4 + 3] == 0)
                return vec3(0, 0, 0);
            else
                return (vec3(0.5, 0.5, 0.5) + vec3(data[index * 4], data[index * 4 + 1], data[index * 4 + 2])) / 256 * pow(2, (int) (data[index * 4 + 3]) - 128);
        }
        else
            return vec3(data[index * 4], data[index * 4 + 1], data[index * 4 + 2]) / 255.0f;
    }

    vec3 getPixel(int index)
    {
        if (exponent)
        {
            if (data[index * 4] == 0 && data[index * 4 + 1] == 0 && data[index * 4 + 2] == 0 && data[index * 4 + 3] == 0)
                return vec3(0, 0, 0);
            else
                return (vec3(0.5, 0.5, 0.5) + vec3(data[index * 4], data[index * 4 + 1], data[index * 4 + 2])) / 256 * pow(2, (int) (data[index * 4 + 3]) - 128);
        }
        else
            return vec3(data[index * 4], data[index * 4 + 1], data[index * 4 + 2]) / 255.0f;
    }

    void writePixel(int index, vec3 l)
    {
        if (exponent)
        {
            float max = l.x;
            if (l.y > max) max = l.y;
            if (l.z > max) max = l.z;
            //printf("%f\n", max);

            if (max <= 0.0f)
            {
                data[index * 4] = 0;
                data[index * 4 + 1] = 0;
                data[index * 4 + 2] = 0;
                data[index * 4 + 3] = 0;
                return;
            }

            int lmax = (int) log2(max);

            float p = pow(2, lmax);
            //printf("%f %f %f %d\n", l.x, l.y, l.z, lmax);
            u8vec4 colOut = u8vec4((unsigned char) (l.x * 256 / p), (unsigned char) (l.y * 256 / p),
                                   (unsigned char) (l.z * 256 / p), lmax + 128);
            data[index * 4] = colOut.x;
            data[index * 4 + 1] = colOut.y;
            data[index * 4 + 2] = colOut.z;
            data[index * 4 + 3] = colOut.w;
        }
        else
        {
            data[index * 4] = (unsigned char)(l.x * 255);
            data[index * 4 + 1] = (unsigned char)(l.y * 255);
            data[index * 4 + 2] = (unsigned char)(l.z * 255);
            data[index * 4 + 3] = 255;
        }
    }

    Tex* downscale()
    {
        Tex* r = new Tex(res / 2, exponent);

        for (int face = 0; face < 6; face++)
        {
            for (int i = 0; i < res / 2; i++)
            {
                for (int j = 0; j < res / 2; j++)
                {
                    int faceOffset = res * res * face;
                    int index = faceOffset + i * 2 * res + j * 2;

                    int faceOffset2 = res * res * face / 4;
                    int index2 = faceOffset2 + i * res / 2 + j;

                    vec3 p1 = getPixel(index);
                    vec3 p2 = getPixel(index + 1);
                    vec3 p3 = getPixel(index + res);
                    vec3 p4 = getPixel(index + res + 1);

                    r->writePixel(index2, (p1 + p2 + p3 + p4) / 4);
                }
            }
        }

        return r;
    }
};

float getReflectanceLambertian(vec3 in, vec3 out)
{
    float dot = in.dot(out);

    if (dot < 0)
        return 0;

    return dot;
}

// Vaguely adapted from https://learnopengl.com/PBR/Theory
float getReflectance(vec3 in, vec3 out, float roughness)
{
    float dot = in.dot(out);

    if (dot < 0)
        return 0;

    float r2 = roughness * roughness;

    if (roughness <= 0)
        return in.x == out.x && in.y == out.y && in.z == out.z;

    float v = dot * dot * (r2 - 1.0f) + 1.0f;
    v = v * v * M_PI;

    if (v > 0)
        return r2 / v;

    return 0;
}

vec3 getLight(Tex* t, CubeSample c, float roughness, bool lam)
{
    if (roughness <= 0)
        return t->sample(c);

    float totalPower = 0;
    vec3 totalColor = vec3(0, 0, 0);
    vec3 cn = c.toVec3().normalize();
    for (int face = 0; face < 6; face++)
    {
        for (int i = 0; i < t->res; i++)
        {
            for (int j = 0; j < t->res; j++)
            {
                float x = (i + 0.5f) / t->res;
                float y = (j + 0.5f) / t->res;
                CubeSample in = {face, vec2(x, y)};
                vec3 in3 = in.toVec3();
                float len = in3.length();
                float power;
                if (lam)
                    power = getReflectanceLambertian(in3 / len, cn) / len;
                else
                    power = getReflectance(in3 / len, cn, roughness) / len;
                totalPower += power;
                totalColor = totalColor + t->sample(in) * power;
            }
        }
    }

    return totalColor / totalPower;
}

void process(std::string in, std::string out, bool expMode, bool lambertian)
{
    Tex *t = new Tex(in);
    t->exponent = expMode;

    int count = lambertian ? 1 : 5;

    for (int m = 0; m < count; m++)
    {
        t = t->downscale();

        Tex* result = new Tex(t->res, expMode);
        for (int face = 0; face < 6; face++)
        {
            for (int i = 0; i < t->res; i++)
            {
                for (int j = 0; j < t->res; j++)
                {
                    int faceOffset = t->res * t->res * face;
                    int index = faceOffset + i * t->res + j;

                    vec2 pos = vec2((i + 0.5) / t->res, (j + 0.5) / t->res);
                    vec3 l = getLight(t, {face, pos}, pow((m + 1.0) / count, 2.0f), lambertian);

                    result->writePixel(index, l);
                }
                printf("face: %d; row: %d / %d\n", face, i, t->res);
            }
        }

        if (lambertian)
            stbi_write_png((out).c_str(), t->res, t->res * 6, 4, result->data, t->res * 4);
        else
            stbi_write_png((out + "." + std::to_string(m) ).c_str(), t->res, t->res * 6, 4, result->data, t->res * 4);
    }
}

int main(int argc, char** argv)
{
    process(argv[1], argv[2], true, false);
}

