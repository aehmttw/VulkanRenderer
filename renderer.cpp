#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define PI 3.1415926535897932384626433832795

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <optional>
#include <set>
#include <fstream>
#include <array>
#include <map>
#include "vecmath.hpp"
#include "json.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "libs/stb_image.h"

#define null nullptr

typedef uint32_t u32;

static size_t meshesDrawn;
static size_t meshesCulled;

const std::vector<const char*> validationLayers =
        {
            "VK_LAYER_KHRONOS_validation"
        };


const std::vector<const char*> deviceExtensions =
        {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        };

const std::vector<const char*> deviceExtensionsHeadless =
        {

        };

const bool enable_validation_layers =
    #ifdef NDEBUG
        false;
    #else
        true;
    #endif

const int max_frames_in_flight = 2;

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback (
            VkDebugUtilsMessageSeverityFlagBitsEXT severity,
            VkDebugUtilsMessageTypeFlagsEXT type,
            const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
            void* pUserData)
{
    printf("validation layer: %s\n", pCallbackData->pMessage);
    return VK_FALSE;
}

VkResult createDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger)
{
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != null)
    {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    }
    else
    {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void destroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator)
{
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != null)
    {
        func(instance, debugMessenger, pAllocator);
    }
}

static std::vector<char> readFile(const std::string& filename)
{
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open())
        throw std::runtime_error("opening file " + filename + " failed!");

    size_t fileSize = (size_t) file.tellg();
    std::vector<char> buffer(fileSize);
    file.seekg(0);
    file.read(buffer.data(), (long) fileSize);
    file.close();

    return buffer;
}

std::map<const std::string, std::vector<char>> cachedFileData;
static std::vector<char>* readFileWithCache(const std::string& filename)
{
    bool present = cachedFileData.contains(filename);

    if (!present)
    {
        std::ifstream file(filename, std::ios::ate | std::ios::binary);

        if (!file.is_open())
            throw std::runtime_error("opening file " + filename + " failed!");

        size_t fileSize = (size_t) file.tellg();

        cachedFileData.insert(std::pair(filename, std::vector<char>(fileSize)));

        std::vector<char>* buffer = &(cachedFileData[filename]);

        file.seekg(0);
        file.read(buffer->data(), (long) fileSize);
        file.close();

        return buffer;
    }

    std::vector<char>* buffer = &(cachedFileData[filename]);
    return buffer;
}

struct SimpleVertex
{
    vec3 pos;
    vec3 normal;
    u8vec4 color;

    static VkVertexInputBindingDescription getBindDesc()
    {
        VkVertexInputBindingDescription desc {};
        desc.binding = 0;
        desc.stride = sizeof(SimpleVertex);
        desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return desc;
    }

    static std::array<VkVertexInputAttributeDescription, 3> getAttributeDesc()
    {
        std::array<VkVertexInputAttributeDescription, 3> descs {};
        descs[0].binding = 0;
        descs[0].location = 0;
        descs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        descs[0].offset = offsetof(SimpleVertex, pos);

        descs[1].binding = 0;
        descs[1].location = 1;
        descs[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        descs[1].offset = offsetof(SimpleVertex, normal);

        descs[2].binding = 0;
        descs[2].location = 2;
        descs[2].format = VK_FORMAT_R8G8B8A8_UNORM;
        descs[2].offset = offsetof(SimpleVertex, color);
        return descs;
    }
};

struct ComplexVertex
{
    vec3 pos;
    vec3 normal;
    vec4 tangent;
    vec2 texCoord;
    u8vec4 color;

    static VkVertexInputBindingDescription getBindDesc()
    {
        VkVertexInputBindingDescription desc {};
        desc.binding = 0;
        desc.stride = sizeof(ComplexVertex);
        desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return desc;
    }

    static std::array<VkVertexInputAttributeDescription, 5> getAttributeDesc()
    {
        std::array<VkVertexInputAttributeDescription, 5> descs {};
        descs[0].binding = 0;
        descs[0].location = 0;
        descs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        descs[0].offset = offsetof(ComplexVertex, pos);

        descs[1].binding = 0;
        descs[1].location = 1;
        descs[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        descs[1].offset = offsetof(ComplexVertex, normal);

        descs[2].binding = 0;
        descs[2].location = 2;
        descs[2].format = VK_FORMAT_R32G32B32A32_SFLOAT;
        descs[2].offset = offsetof(ComplexVertex, tangent);

        descs[3].binding = 0;
        descs[3].location = 3;
        descs[3].format = VK_FORMAT_R32G32_SFLOAT;
        descs[3].offset = offsetof(ComplexVertex, texCoord);

        descs[4].binding = 0;
        descs[4].location = 4;
        descs[4].format = VK_FORMAT_R8G8B8A8_UNORM;
        descs[4].offset = offsetof(ComplexVertex, color);

        return descs;
    }
};

struct UniformBufferObject
{
    alignas(16) mat4 proj;
    alignas(16) mat4 camera;
    alignas(16) mat4 environment;
    alignas(16) vec4 cameraPos;
    bool hdr;
};

struct PushConstant
{
    alignas(16) mat4 modelView;
    int shadowMap;
};

// A good portion of this code is adapted from the official Vulkan tutorial: https://docs.vulkan.org/tutorial/latest/
class Renderer
{
public:
    void run()
    {
        if (!headless)
            initWindow();

        initialize();
        loop();
        cleanup();
    }

    struct GraphicsPipeline
    {
        VkPipeline pipeline;
        VkPipelineLayout layout;
    };

    struct Node;

    struct Attribute
    {
        void* data;
        size_t offset;
        size_t stride;
        VkFormat format;

        Attribute(std::string dataPath, size_t offset, size_t stride, std::string format)
        {
            this->data = readFileWithCache(dataPath)->data();
            this->offset = offset;
            this->stride = stride;

            if (format == "R32G32B32_SFLOAT")
                this->format = VK_FORMAT_R32G32B32_SFLOAT;
            else if (format == "R8G8B8A8_UNORM")
                this->format = VK_FORMAT_R8G8B8A8_UNORM;
        }
    };

    struct Material;

    struct Mesh
    {
        Renderer* renderer;

        std::string name;
        size_t count;
        VkPrimitiveTopology topology;
        std::map<std::string, Attribute> attributes;
        Material* material;

        vec3 corners[2][2][2];

        VkBuffer buffer;
        VkDeviceMemory bufferMemory;

        Mesh(Renderer* t, std::string name, size_t count, const std::string& topology)
        {
            this->material = &(t->defaultMaterial);
            this->name = name;
            this->count = count;

            if (topology == "POINT_LIST")
                this->topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
            else if (topology == "LINE_LIST")
                this->topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
            else if (topology == "LINE_STRIP")
                this->topology = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
            else if (topology == "TRIANGLE_LIST")
                this->topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            else if (topology == "TRIANGLE_STRIP")
                this->topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
            else if (topology == "TRIANGLE_FAN")
                this->topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
            else if (topology == "LINE_LIST_WITH_ADJACENCY")
                this->topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY;
            else if (topology == "LINE_STRIP_WITH_ADJACENCY")
                this->topology = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY;
            else if (topology == "TRIANGLE_LIST_WITH_ADJACENCY")
                this->topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY;
            else if (topology == "TRIANGLE_STRIP_WITH_ADJACENCY")
                this->topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY;
            else if (topology == "PATCH_LIST")
                this->topology = VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;

            renderer = t;
        }

        void initialize()
        {
            VkBuffer stagingBuffer;
            VkDeviceMemory stagingBufferMemory;

            VkDeviceSize size = this->material->type->stride * count;
            renderer->createBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

            Attribute pos = attributes.at("POSITION");
//            Attribute col = attributes.at("COLOR");
//
//            for (int i = 0; i < count; i++)
//            {
//                *(float*) &(((char*) (pos.data))[col.offset + col.stride * i + 0 * sizeof(float)]) *= 2;
//                *(float*) &(((char*) (pos.data))[col.offset + col.stride * i + 1 * sizeof(float)]) *= 2;
//                *(float*) &(((char*) (pos.data))[col.offset + col.stride * i + 2 * sizeof(float)]) *= 2;
//            }

            void* data;
            vkMapMemory(renderer->device, stagingBufferMemory, 0, size, 0, &data);
            memcpy(data, pos.data, size);
            vkUnmapMemory(renderer->device, stagingBufferMemory);

            char* datac = (char*) data;
            float minX = std::numeric_limits<float>::infinity();
            float minY = std::numeric_limits<float>::infinity();;
            float minZ = std::numeric_limits<float>::infinity();;

            float maxX = -std::numeric_limits<float>::infinity();;
            float maxY = -std::numeric_limits<float>::infinity();;
            float maxZ = -std::numeric_limits<float>::infinity();;
            for (size_t i = pos.offset; i < size; i += pos.stride)
            {
                float x = *((float*) (&(datac[i])));
                float y = *((float*) (&(datac[i + sizeof(float)])));
                float z = *((float*) (&(datac[i + sizeof(float) * 2])));

                if (x < minX)
                    minX = x;

                if (y < minY)
                    minY = y;

                if (z < minZ)
                    minZ = z;

                if (x > maxX)
                    maxX = x;

                if (y > maxY)
                    maxY = y;

                if (z > maxZ)
                    maxZ = z;
            }

            if (renderer->enableCulling)
            {
                for (int i = 0; i < 2; i++)
                {
                    for (int j = 0; j < 2; j++)
                    {
                        for (int k = 0; k < 2; k++)
                        {
                            this->corners[i][j][k] = vec3(i == 0 ? minX : maxX,
                                                          j == 0 ? minY : maxY,
                                                          k == 0 ? minZ : maxZ);
                        }
                    }
                }
            }

            renderer->createBuffer(size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, buffer, bufferMemory);
            renderer->copyBuffer(stagingBuffer, buffer, size);

            vkDestroyBuffer(renderer->device, stagingBuffer, null);
            vkFreeMemory(renderer->device, stagingBufferMemory, null);
        }

        void draw(VkCommandBuffer commandBuffer, mat4 m)
        {
            GraphicsPipeline* pipeline = this->material->type->getPipeline(renderer->scene);
            if (renderer->currentMaterialType != this->material->type)
            {
                renderer->currentMaterialType = this->material->type;
                vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipeline);
            }

            VkBuffer vertexBuffers[] = { buffer };
            VkDeviceSize offsets[] = { 0 };
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->layout,
                                    0, 1, &(renderer->globalDescriptorSets[renderer->currentFrame]), 0, null);
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->layout,
                                    1, 1, &(material->descriptorSets[renderer->currentFrame]), 0, null);

            renderer->updatePushConstants(m, renderer->currentFrame);

            vkCmdDraw(commandBuffer, count, 1, 0, 0);
        }

        ~Mesh()
        {
            vkDestroyBuffer(renderer->device, buffer, null);
            vkFreeMemory(renderer->device, bufferMemory, null);
        }
    };

    struct Camera
    {
        std::string name;

        // Frustum projection
        mat4 perspectiveTransform = mat4::I();
        // Derived from node scene graph transform
        mat4 worldToCameraBaseTransform = mat4::I();
        mat4 cameraBaseToWorldTransform = mat4::I();
        // Controlled by the user
        mat4 cameraUserOffsetTransform = mat4::I();
        mat4 cameraPosTransform = mat4::I();
        // Everything
        mat4 fullTransform;

        float aspectRatio;
        float verticalFOV;
        float verticalFOVScaled;
        float verticalFOVTan;
        float nearPlane;
        float farPlane;

        vec3 position = vec3(0, 0, 0);
        mat4 rotation = mat4::I();
        mat4 rotationInverse = mat4::I();
        float speed = 10.0f;
        float zoom = 0.0f;

        explicit Camera(const std::string &name,
                        float aspectRatio,
                        float verticalFOV,
                        float nearPlane,
                        float farPlane = std::numeric_limits<float>::infinity())
        {
            this->name = name;
            this->aspectRatio = aspectRatio;
            this->verticalFOV = verticalFOV;
            this->nearPlane = nearPlane;
            this->farPlane = farPlane;
            this->computeTransform();
        }

        void computeTransform()
        {
            if (farPlane == std::numeric_limits<float>::infinity())
                this->perspectiveTransform = mat4::infinitePerspective(aspectRatio, verticalFOV + zoom, nearPlane);
            else
                this->perspectiveTransform = mat4::perspective(aspectRatio, verticalFOV + zoom, nearPlane, farPlane);

            this->verticalFOVScaled = verticalFOV + zoom;
            this->verticalFOVTan = tan(verticalFOVScaled / 2);
        }

        void updateTransform()
        {
            cameraUserOffsetTransform = rotation * mat4::translation(position * -1) * worldToCameraBaseTransform;
            cameraPosTransform = cameraBaseToWorldTransform * mat4::translation(position) * rotationInverse;
            fullTransform = perspectiveTransform * cameraUserOffsetTransform;
        }
    };

    static const int step = 0;
    static const int linear = 1;
    static const int slerp = 2;

    template<typename V>
    struct Driver
    {
        std::string name;
        std::vector<double> times;
        std::vector<V> values;
        int interpolation;

        Driver(const std::string &name, const std::vector<double> &times, const std::vector<V> &values, const std::string& interpolation)
        {
            this->name = name;
            this->times = times;
            this->values = values;
            if (interpolation == "STEP")
                this->interpolation = step;
            else if (interpolation == "LINEAR")
                this->interpolation = linear;
            else if (interpolation == "SLERP")
                this->interpolation = slerp;
        }

        V get(float time)
        {
            float beforeTime = -1;
            V* before = null;
            float afterTime = -1;
            V* after = null;

            for (int i = 0; i < times.size(); i++)
            {
                if (times[i] <= time)
                {
                    before = &(values[i]);
                    beforeTime = times[i];
                }
                else
                {
                    after = &(values[i]);
                    afterTime = times[i];
                    break;
                }
            }

            if (after == null)
                return *before;
            else if (before == null)
                return *after;

            float frac = (time - beforeTime) / (afterTime - beforeTime);

            if (interpolation == step)
                return *before;
            else if (interpolation == linear)
                return *before * (1 - frac) + *after * frac;
            else
            {
                // adapted from glm: https://github.com/g-truc/glm/blob/master/glm/ext/quaternion_common.inl
                double cosTheta = before->dot(*after);
                V after2 = *after;
                if (cosTheta < 0)
                {
                    cosTheta = -cosTheta;
                    after2 = after2 * -1.0;
                }

                if (cosTheta > 0.99999)
                    return *before * (1 - frac) + after2 * frac;
                else
                {
                    double angle = std::acos(cosTheta);
                    return ((*before) * sin((1 - frac) * angle) + after2 * sin(frac * angle)) / sin(angle);
                }
            }
        }
    };

    struct Texture
    {
        Renderer* renderer;

        bool isCube = false;
        bool exponential = false;
        std::vector<unsigned char*> data;
        std::vector<float*> dataFloat;
        int width;
        int height;
        int channels;

        bool initialized = false;
        VkImage image;
        VkDeviceMemory imageMemory;
        VkImageView imageView;
        VkSampler sampler;

        Texture(Renderer* r, vec3 data, bool cube = false)
        {
            this->renderer = r;
            this->isCube = cube;
            this->exponential = false;

            int q = cube ? 6 : 1;
            this->data.emplace_back(new unsigned char[4 * q]);

            for (int i = 0; i < q; i++)
            {
                this->data[0][i * 4 + 0] = (unsigned char) (255 * data.x);
                this->data[0][i * 4 + 1] = (unsigned char) (255 * data.y);
                this->data[0][i * 4 + 2] = (unsigned char) (255 * data.z);
                this->data[0][i * 4 + 3] = 255;
            }

            this->width = 1;
            this->height = q;

            initTexture();
        }

        Texture(Renderer* r, jobject* o, int mipmapCount = 1, std::string suffix = "")
        {
            assert(o);
            this->renderer = r;
            std::string src = jstring::cast((*o)["src"])->value + suffix;
            if ((*o)["type"] != null)
                isCube = jstring::cast((*o)["type"])->value == "cube";
            if ((*o)["format"] != null)
                exponential = jstring::cast((*o)["format"])->value == "rgbe";

            data.emplace_back(stbi_load(src.c_str(), &width, &height, &channels, STBI_rgb_alpha));
            if (!data[0])
            {
                printf("source: %s\n", src.c_str());
                throw std::runtime_error("failed to create image!");
            }

            for (int i = 0; i < mipmapCount - 1; i++)
            {
                int w;
                int h;
                data.emplace_back(stbi_load((src + "." + std::to_string(i) + ".png").c_str(), &w, &h, &channels, STBI_rgb_alpha));
                if (!data[i + 1])
                    throw std::runtime_error("failed to create image mipmap!");
            }

            initTexture();
        }

        void createTextureImage()
        {
            VkDeviceSize imageSize = this->width * this->height * 4 * sizeof(float);

            renderer->createImage(this->width, this->height, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, this->image, this->imageMemory, this->dataFloat.size(), isCube);
            renderer->transitionImageLayout(this->image, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, this->dataFloat.size(), isCube);

            u32 w = this->width;
            u32 h = this->height;
            for (int i = 0; i < this->dataFloat.size(); i++)
            {
                VkBuffer stagingBuffer;
                VkDeviceMemory stagingBufferMemory;
                renderer->createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                       stagingBuffer, stagingBufferMemory);

                void *data;
                vkMapMemory(renderer->device, stagingBufferMemory, 0, imageSize, 0, &data);
                memcpy(data, this->dataFloat[i], static_cast<size_t>(imageSize));
                vkUnmapMemory(renderer->device, stagingBufferMemory);

                renderer->copyBufferToImage(stagingBuffer, this->image, w, h, i, isCube);
                vkDestroyBuffer(renderer->device, stagingBuffer, null);
                vkFreeMemory(renderer->device, stagingBufferMemory, null);

                w /= 2;
                h /= 2;
            }

            renderer->transitionImageLayout(this->image, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, this->dataFloat.size(), isCube);
        }

        void createTextureImageView()
        {
            this->imageView = renderer->createImageView(this->image, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT, this->dataFloat.size(), isCube);
        }

        void createTextureSampler()
        {
            VkPhysicalDeviceProperties properties {};
            vkGetPhysicalDeviceProperties(renderer->physicalDevice, &properties);

            VkSamplerCreateInfo samplerInfo {};
            samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            samplerInfo.magFilter = VK_FILTER_LINEAR;
            samplerInfo.minFilter = VK_FILTER_LINEAR;
            samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerInfo.anisotropyEnable = VK_TRUE;
            samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
            samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
            samplerInfo.unnormalizedCoordinates = VK_FALSE;
            samplerInfo.compareEnable = VK_FALSE;
            samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
            samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            samplerInfo.minLod = 0.0f;
            samplerInfo.maxLod = VK_LOD_CLAMP_NONE;

            auto result = vkCreateSampler(renderer->device, &samplerInfo, nullptr, &(this->sampler));

            if (result != VK_SUCCESS)
            {
                printf("failed: %d\n", result);
                throw std::runtime_error("texture sampler creation failed!");
            }
        }

        void convertTexture()
        {
            for (int m = 0; m < data.size(); m++)
            {
                this->dataFloat.emplace_back(new float[this->width * this->height * 4]);
                for (int i = 0; i < this->width * this->height; i++)
                {
                    unsigned char r = this->data[m][i * 4];
                    unsigned char g = this->data[m][i * 4 + 1];
                    unsigned char b = this->data[m][i * 4 + 2];
                    unsigned char a = this->data[m][i * 4 + 3];

                    if (exponential)
                    {
                        auto e = (float) pow(2, (int) a - 128);

                        if (r == 0 && g == 0 && b == 0 && a == 0)
                            e = 0;

                        this->dataFloat[m][i * 4] = ((float) r + 0.5f) / 256 * e;
                        this->dataFloat[m][i * 4 + 1] = ((float) g + 0.5f) / 256 * e;
                        this->dataFloat[m][i * 4 + 2] = ((float) b + 0.5f) / 256 * e;

                        this->dataFloat[m][i * 4 + 3] = 1.0f;
                    }
                    else
                    {
                        this->dataFloat[m][i * 4] = (float) r / 255.0f;
                        this->dataFloat[m][i * 4 + 1] = (float) g / 255.0f;
                        this->dataFloat[m][i * 4 + 2] = (float) b / 255.0f;
                        this->dataFloat[m][i * 4 + 3] = (float) a / 255.0f;
                    }
                }

                stbi_image_free(data[m]);
            }
        }

        void initTexture()
        {
            assert (!initialized);
            convertTexture();
            createTextureImage();
            createTextureImageView();
            createTextureSampler();

            for (int i = 0; i < dataFloat.size(); i++)
            {
                delete dataFloat[i];
            }

            initialized = true;
        }

        ~Texture()
        {
            vkDestroySampler(renderer->device, sampler, nullptr);
            vkDestroyImageView(renderer->device, imageView, nullptr);

            vkDestroyImage(renderer->device, image, nullptr);
            vkFreeMemory(renderer->device, imageMemory, nullptr);
        }
    };

    struct Environment
    {
        std::string name;
        Texture* texturePBR;
        Texture* textureLambertian;
        mat4 worldToEnvironmentTransform = mat4::I();

        Environment(std::string name, Texture* texPBR, Texture* texLam)
        {
            this->name = name;
            this->texturePBR = texPBR;
            this->textureLambertian = texLam;
        }

        ~Environment()
        {
            delete texturePBR;
            delete textureLambertian;
        }
    };

    struct Scene;
    struct MaterialType;

    static std::vector<MaterialType*> materialTypes;

    struct MaterialType
    {
        GraphicsPipeline pipeline;
        GraphicsPipeline shadowMapPipeline;

        GraphicsPipeline* getPipeline(Scene* scene)
        {
            if (scene->drawingShadow)
                return &shadowMapPipeline;
            else
                return &pipeline;
        }

        u32 stride;

        VkDescriptorPool descriptorPool;
        VkDescriptorSetLayout descriptorSetLayout;

        int textures;
        int instances = 0;

        MaterialType(int textures)
        {
            materialTypes.emplace_back(this);
            this->textures = textures;
        }

        void createDescriptorPool(VkDevice &device)
        {
            VkDescriptorPoolCreateInfo poolInfo {};

            std::vector<VkDescriptorPoolSize> poolSizes (1);

            int i = 1;
            if (instances > 0)
                i = instances;

            poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            poolSizes[0].descriptorCount = static_cast<uint32_t>(max_frames_in_flight * i);

            if (textures > 0)
            {
                poolSizes.resize(2);
                poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                poolSizes[1].descriptorCount = static_cast<uint32_t>(max_frames_in_flight * textures * i);
            }

            poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
            poolInfo.pPoolSizes = poolSizes.data();
            poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            poolInfo.maxSets = static_cast<uint32_t>(max_frames_in_flight * i);

            auto result = vkCreateDescriptorPool(device, &poolInfo, null, &descriptorPool);
            if (result != VK_SUCCESS)
            {
                printf("failed: %d\n", result);
                throw std::runtime_error("descriptor pool creation failed!");
            }
        }

        void createDescriptorSetLayout(VkDevice &device)
        {
            std::vector<VkDescriptorSetLayoutBinding> bindings(0);

            if (textures > 0)
            {
                VkDescriptorSetLayoutBinding samplerLayoutBinding{};
                samplerLayoutBinding.binding = 0;
                samplerLayoutBinding.descriptorCount = textures;
                samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                samplerLayoutBinding.pImmutableSamplers = null;
                samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
                bindings.emplace_back(samplerLayoutBinding);
            }

            VkDescriptorSetLayoutCreateInfo layoutInfo {};
            layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            layoutInfo.bindingCount = bindings.size();
            layoutInfo.pBindings = bindings.data();

//                VkDescriptorSetLayoutBinding uboLayoutBinding{};
//                uboLayoutBinding.binding = 0;
//                uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
//                uboLayoutBinding.descriptorCount = 1;
//                uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

            auto result = vkCreateDescriptorSetLayout(device, &layoutInfo, null, &descriptorSetLayout);
            if (result != VK_SUCCESS)
            {
                printf("failed: %d\n", result);
                throw std::runtime_error("descriptor set layout failed!");
            }
        }
    };

    static MaterialType materialTypeSimple;
    static MaterialType materialTypePBR;
    static MaterialType materialTypeLambertian;
    static MaterialType materialTypeMirror;
    static MaterialType materialTypeEnvironment;

    struct Material
    {
        std::string name;
        MaterialType* type;
        Texture* normalMap = null;

        std::vector<VkDescriptorSet> descriptorSets;

        virtual void createDescriptorSets(VkDevice &device)
        {
            std::vector<VkDescriptorSetLayout> layouts(max_frames_in_flight, type->descriptorSetLayout);
            VkDescriptorSetAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            allocInfo.descriptorPool = type->descriptorPool;
            allocInfo.descriptorSetCount = static_cast<u32>(max_frames_in_flight);
            allocInfo.pSetLayouts = layouts.data();

            descriptorSets.resize(max_frames_in_flight);
            auto result = vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data());
            if (result != VK_SUCCESS)
            {
                printf("failed: %d\n", result);
                throw std::runtime_error("descriptor sets creation failed!");
            }
        }

        virtual ~Material()
        {
            delete normalMap;
        }
    };

    struct MaterialSimple : Material
    {
        MaterialSimple()
        {
            this->type = &materialTypeSimple;
        }

        void createDescriptorSets(VkDevice &device) override
        {
            Material::createDescriptorSets(device);
            for (size_t i = 0; i < max_frames_in_flight; i++)
            {
                std::array<VkWriteDescriptorSet, 0> descriptorWrites {};
                vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, null);
            }
        }
    };

    struct MaterialPBR : Material
    {
        Texture* albedoTex = null;
        Texture* roughnessTex = null;
        Texture* metalnessTex = null;

        MaterialPBR()
        {
            this->type = &materialTypePBR;
        }

        ~MaterialPBR() override
        {
            delete albedoTex;
            delete roughnessTex;
            delete metalnessTex;
        }

        void createDescriptorSets(VkDevice &device) override
        {
            Material::createDescriptorSets(device);
            for (size_t i = 0; i < max_frames_in_flight; i++)
            {
                VkDescriptorImageInfo normalInfo {};
                normalInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                normalInfo.imageView = normalMap->imageView;
                normalInfo.sampler = normalMap->sampler;

                VkDescriptorImageInfo albedoInfo {};
                albedoInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                albedoInfo.imageView = albedoTex->imageView;
                albedoInfo.sampler = albedoTex->sampler;

                VkDescriptorImageInfo roughnessInfo {};
                roughnessInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                roughnessInfo.imageView = roughnessTex->imageView;
                roughnessInfo.sampler = roughnessTex->sampler;

                VkDescriptorImageInfo metalnessInfo {};
                metalnessInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                metalnessInfo.imageView = metalnessTex->imageView;
                metalnessInfo.sampler = metalnessTex->sampler;

                std::array<VkDescriptorImageInfo, 4> imageInfos {normalInfo, albedoInfo, roughnessInfo, metalnessInfo};

                std::array<VkWriteDescriptorSet, 1> descriptorWrites {};

                descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                descriptorWrites[0].dstSet = descriptorSets[i];
                descriptorWrites[0].dstBinding = 0;
                descriptorWrites[0].dstArrayElement = 0;
                descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                descriptorWrites[0].descriptorCount = 4;
                descriptorWrites[0].pImageInfo = imageInfos.data();

                vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, null);
            }
        }
    };

    struct MaterialLambertian : Material
    {
        Texture* albedoTex = null;

        MaterialLambertian()
        {
            this->type = &materialTypeLambertian;
        }

        ~MaterialLambertian() override
        {
            delete albedoTex;
        }

        void createDescriptorSets(VkDevice &device) override
        {
            Material::createDescriptorSets(device);
            for (size_t i = 0; i < max_frames_in_flight; i++)
            {
                VkDescriptorImageInfo normalInfo {};
                normalInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                normalInfo.imageView = normalMap->imageView;
                normalInfo.sampler = normalMap->sampler;

                VkDescriptorImageInfo imageInfo {};
                imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                imageInfo.imageView = albedoTex->imageView;
                imageInfo.sampler = albedoTex->sampler;

                std::array<VkDescriptorImageInfo, 2> imageInfos {normalInfo, imageInfo};

                std::array<VkWriteDescriptorSet, 1> descriptorWrites {};
                descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                descriptorWrites[0].dstSet = descriptorSets[i];
                descriptorWrites[0].dstBinding = 0;
                descriptorWrites[0].dstArrayElement = 0;
                descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                descriptorWrites[0].descriptorCount = imageInfos.size();
                descriptorWrites[0].pImageInfo = imageInfos.data();

                vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, null);
            }
        }
    };

    struct MaterialMirror : Material
    {
        MaterialMirror()
        {
            this->type = &materialTypeMirror;
        }

        void createDescriptorSets(VkDevice &device) override
        {
            Material::createDescriptorSets(device);
            for (size_t i = 0; i < max_frames_in_flight; i++)
            {
                VkDescriptorImageInfo normalInfo {};
                normalInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                normalInfo.imageView = normalMap->imageView;
                normalInfo.sampler = normalMap->sampler;

                std::array<VkDescriptorImageInfo, 1> imageInfos {normalInfo};

                std::array<VkWriteDescriptorSet, 1> descriptorWrites {};
                descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                descriptorWrites[0].dstSet = descriptorSets[i];
                descriptorWrites[0].dstBinding = 0;
                descriptorWrites[0].dstArrayElement = 0;
                descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                descriptorWrites[0].descriptorCount = imageInfos.size();
                descriptorWrites[0].pImageInfo = imageInfos.data();

                vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, null);
            }
        }
    };

    struct MaterialEnvironment : Material
    {
        MaterialEnvironment()
        {
            this->type = &materialTypeEnvironment;
        }

        void createDescriptorSets(VkDevice &device) override
        {
            Material::createDescriptorSets(device);
            for (size_t i = 0; i < max_frames_in_flight; i++)
            {
                VkDescriptorImageInfo normalInfo {};
                normalInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                normalInfo.imageView = normalMap->imageView;
                normalInfo.sampler = normalMap->sampler;

                std::array<VkDescriptorImageInfo, 1> imageInfos {normalInfo};

                std::array<VkWriteDescriptorSet, 1> descriptorWrites {};
                descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                descriptorWrites[0].dstSet = descriptorSets[i];
                descriptorWrites[0].dstBinding = 0;
                descriptorWrites[0].dstArrayElement = 0;
                descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                descriptorWrites[0].descriptorCount = imageInfos.size();
                descriptorWrites[0].pImageInfo = imageInfos.data();

                vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, null);
            }
        }
    };

    struct Light
    {
        bool isSun = false;
        std::string name;
        float power;
        vec3 tint = vec3(0, 0, 0);
        mat4 worldToLightTransform = mat4::I();
    };

    // Format of data sent to shader for lights
    struct alignas(16) ShaderLight
    {
        mat4 worldToLight;
        mat4 projection;
        vec4 tintPower;
        float radius;
        float limit;
        float fov;
        float blend;
        int isSun;
        int shadowRes;
    };

    struct ShadowMap
    {
        std::vector<VkFramebuffer> framebuffers {max_frames_in_flight};
        std::vector<VkImage> images {max_frames_in_flight};
        std::vector<VkDeviceMemory> imageMemories {max_frames_in_flight};
        std::vector<VkImageView> imageViews {max_frames_in_flight};
        std::vector<VkSampler> samplers {max_frames_in_flight};
        int resolution;

        ShadowMap(int l, int res)
        {
            this->resolution = res;
        }
    };

    struct SunLight : Light
    {
        float angle;

        SunLight(std::string name, vec3 tint, float angle, float power)
        {
            this->isSun = true;
            this->name = name;
            this->tint = tint;
            this->angle = angle;
            this->power = power;
        }
    };

    struct SpotLight : Light
    {
        float radius;
        float limit;

        float fov;
        float blend;

        int shadowRes = 0;

        SpotLight(std::string name, vec3 tint, float radius, float power, float limit = std::numeric_limits<float>::infinity(), float fov = -1, float blend = -1, int shadowRes = 0)
        {
            this->name = name;
            this->tint = tint;
            this->radius = radius;
            this->power = power;
            this->limit = limit;
            this->fov = fov;
            this->blend = blend;
            this->shadowRes = shadowRes;
        }
    };

    struct Node
    {
        std::string name;

        mat4 transform;
        mat4 invTransform;

        vec3 translation;
        vec4 rotation;
        vec3 scale;

        Driver<dvec3>* translationDriver = null;
        Driver<dvec4>* rotationDriver = null;
        Driver<dvec3>* scaleDriver = null;

        std::vector<Node*> children;
        Camera* camera = null;
        Mesh* mesh = null;
        Environment* environment = null;
        Light* light = null;

        explicit Node(const std::string &name,
                      vec3 translation = vec3(0.0f, 0.0f, 0.0f),
                      vec4 rotation = vec4(0.0f, 0.0f, 0.0f, 1.0f),
                      vec3 scale = vec3(1.0f, 1.0f, 1.0f))
        {
            this->translation = translation;
            this->rotation = rotation;
            this->scale = scale;

            this->computeTransform();
            this->name = name;
        }

        void computeDriverTransforms(float time)
        {
            if (this->translationDriver == null && this->scaleDriver == null && this->rotationDriver == null)
                return;

            if (this->translationDriver != null)
                this->translation = vec3(this->translationDriver->get(time).x, this->translationDriver->get(time).y, this->translationDriver->get(time).z);

            if (this->rotationDriver != null)
                this->rotation = vec4(this->rotationDriver->get(time).x, this->rotationDriver->get(time).y, this->rotationDriver->get(time).z, this->rotationDriver->get(time).w);

            if (this->scaleDriver != null)
                this->scale = vec3(this->scaleDriver->get(time).x, this->scaleDriver->get(time).y, this->scaleDriver->get(time).z);

            this->computeTransform();
        }

        void computeTransform()
        {
            this->transform = mat4::translation(translation) * mat4::rotate(rotation) * mat4::scale(scale);
            this->invTransform = mat4::scale(vec3(1.0f / scale.x, 1.0f / scale.y, 1.0f / scale.z)) * mat4::rotate(vec4(rotation.xyz(), -rotation.w)) * mat4::translation(translation * -1.0f);
        }

        void draw(VkCommandBuffer b, mat4 m, Camera* camera, bool enableCulling, float time)
        {
            assert(this);
            this->computeDriverTransforms(time);

            mat4 m2 = m * transform;
            if (mesh != null)
            {
                mat4 t = camera->cameraUserOffsetTransform * m2;
                bool culled = false;

                if (enableCulling)
                {
                    vec3 minPos;
                    vec3 maxPos;

                    for (int i = 0; i < 2; i++)
                    {
                        for (int j = 0; j < 2; j++)
                        {
                            for (int k = 0; k < 2; k++)
                            {
                                vec4 pos = (t * vec4(mesh->corners[i][j][k], 1.0f));

                                if (i == 0 && j == 0 && k == 0)
                                {
                                    minPos = pos.xyz();
                                    maxPos = pos.xyz();
                                    minPos.z = -pos.z;
                                    maxPos.z = -pos.z;
                                }
                                else
                                {
                                    minPos = vec3(std::min(pos.x, minPos.x), std::min(pos.y, minPos.y),
                                                  std::min(-pos.z, minPos.z));
                                    maxPos = vec3(std::max(pos.x, maxPos.x), std::max(pos.y, maxPos.y),
                                                  std::max(-pos.z, maxPos.z));
                                }
                            }
                        }
                    }

                    if (maxPos.z < camera->nearPlane || minPos.z > camera->farPlane)
                        culled = true;
                    else
                    {
                        float z = std::min(maxPos.z, camera->farPlane);
                        float y = camera->verticalFOVTan * z;
                        float x = y * camera->aspectRatio;
                        culled = maxPos.y < -y || minPos.y > y || maxPos.x < -x || minPos.x > x;
                    }
                }

                if (culled)
                    meshesCulled++;
                else
                {
                    meshesDrawn++;
                    mesh->draw(b, m2);
                }
            }

            for (Node* n: children)
            {
                n->draw(b, m2, camera, enableCulling, time);
            }
        }

        // Computes transforms for cameras and environments
        void precomputeTransforms(mat4 m, mat4 mi)
        {
            mat4 m2 = m * transform;
            mat4 m2i = invTransform * mi;
            if (camera != null)
            {
                camera->cameraBaseToWorldTransform = m2;
                camera->worldToCameraBaseTransform = m2i;
                camera->updateTransform();
            }

            if (environment != null)
                environment->worldToEnvironmentTransform = m2i;

            for (Node* n: children)
            {
                n->precomputeTransforms(m2, m2i);
            }
        }

        // Computes shader information for lights
        void precomputeLights(std::vector<ShaderLight> &shaderLights, std::vector<ShadowMap> &shadowMaps, bool init, int &shadowIndex, mat4 m, mat4 mi)
        {
            mat4 m2 = m * transform;
            mat4 m2i = invTransform * mi;

            if (light != null)
            {
                if (light->isSun)
                {
                    auto* s = (SunLight*) light;
                    ShaderLight sl = {m2i, mat4::I(), vec4(s->tint, s->power), s->angle, -1, -1, -1, true};
                    shaderLights.emplace_back(sl);
                }
                else
                {
                    auto* s = (SpotLight*) light;
                    ShaderLight sl = {m2i, mat4::I(), vec4(s->tint, s->power), s->radius, s->limit, s->fov, s->blend, false};
                    if (s->fov > 0)
                    {
                        if (isinf(s->limit))
                            sl.projection = mat4::infinitePerspective(1, s->fov, s->radius);
                        else
                            sl.projection = mat4::perspective(1, s->fov, s->radius, s->limit);

                        sl.shadowRes = s->shadowRes;
                        if (sl.shadowRes > 0)
                        {
                            shaderLights.emplace_back(sl);
                            shadowIndex++;

                            if (!init)
                                shadowMaps.emplace_back(ShadowMap(shaderLights.size() - 1, sl.shadowRes));
                        }
                        else
                            shaderLights.emplace_back(sl);
                    }
                    else
                        shaderLights.emplace_back(sl);
                }
            }

            for (Node* n: children)
            {
                n->precomputeLights(shaderLights, shadowMaps, init, shadowIndex, m2, m2i);
            }
        }
    };

    struct Scene
    {
        std::string name;
        std::vector<Node*> roots;

        std::map<int, Node*> nodes;
        std::map<int, Mesh*> meshes;
        std::map<int, Camera*> cameras;
        std::map<int, Driver<dvec3>*> translationDrivers;
        std::map<int, Driver<dvec4>*> rotationDrivers;
        std::map<int, Driver<dvec3>*> scaleDrivers;
        std::map<int, Environment*> environments;
        std::map<int, Material*> materials;
        std::map<int, Light*> lights;

        std::vector<ShaderLight> shaderLights {};
        std::vector<ShadowMap> shadowMaps {};
        int shadowCount = 0;

        bool drawingShadow = false;
        int currentShadowMapIndex;
        ShadowMap* currentShadowMap = null;

        Environment* defaultEnvironment;
        Environment* environment;

        // Cameras enumerated sequentially
        std::map<int, Camera*> camerasEnumerated;
        int currentCameraIndex = 0;

        int cameraCount = 1;
        Camera* detachedCamera = new Camera("default", 1.5, PI / 2, 0.1);
        Camera* debugCamera = new Camera("debug", 1.5, PI / 2, 0.1);

        Camera* currentCamera = detachedCamera;
        bool debugCameraMode = false;

        bool initialized = false;

        Scene(std::string name)
        {
            this->name = name;
        }

        ~Scene()
        {
            for (auto n: nodes)
                delete n.second;

            for (auto n: meshes)
                delete n.second;

            for (auto n: cameras)
                delete n.second;

            for (auto n: translationDrivers)
                delete n.second;

            for (auto n: rotationDrivers)
                delete n.second;

            for (auto n: scaleDrivers)
                delete n.second;

            for (auto n: environments)
                delete n.second;

            for (auto n: materials)
                delete n.second;

            delete detachedCamera;
            delete debugCamera;
            delete defaultEnvironment;
        }
    };

    Scene* parseScene(jarray* sceneArray)
    {
        Scene* scene;
        std::map<int, Node*> nodes;
        std::map<int, Mesh*> meshes;
        std::map<int, Camera*> cameras;

        std::map<int, Driver<dvec3>*> translations;
        std::map<int, Driver<dvec4>*> rotations;
        std::map<int, Driver<dvec3>*> scales;

        std::map<int, Environment*> environments;
        std::map<int, Material*> materials;
        std::map<int, Light*> lights;

        Environment* env = null;

        int cameraCount = 1;
        std::map<int, Camera*> camerasEnumerated;

        Camera* currentCam = null;
        int currentCamIndex = 0;

        // Initialize
        int i = 0;
        for (jvalue* v: sceneArray->elements)
        {
            if (i == 0)
            {
                jstring* s = jstring::cast(v);
                assert(s->value == "s72-v1");
                i++;
                continue;
            }

            jobject* o = jobject::cast(v);
            std::string type = jstring::cast((*o)["type"])->value;
            if (type == "SCENE")
            {
                scene = new Scene(jstring::cast((*o)["name"])->value);
            }
            else if (type == "NODE")
            {
                std::string name = jstring::cast((*o)["name"])->value;
                vec3 translation = vec3(0.0f, 0.0f, 0.0f);
                vec4 rotation = vec4(0.0f, 0.0f, 0.0f, 1.0f);
                vec3 scale = vec3(1.0f, 1.0f, 1.0f);

                jarray* jtranslation = jarray::cast((*o)["translation"]);
                if (jtranslation != null)
                    translation = vec3(jnumber::cast((*jtranslation)[0])->value, jnumber::cast((*jtranslation)[1])->value, jnumber::cast((*jtranslation)[2])->value);

                jarray* jrotation = jarray::cast((*o)["rotation"]);
                if (jrotation != null)
                    rotation = vec4(jnumber::cast((*jrotation)[0])->value, jnumber::cast((*jrotation)[1])->value, jnumber::cast((*jrotation)[2])->value, jnumber::cast((*jrotation)[3])->value);

                jarray* jscale = jarray::cast((*o)["scale"]);
                if (jscale != null)
                    scale = vec3(jnumber::cast((*jscale)[0])->value, jnumber::cast((*jscale)[1])->value, jnumber::cast((*jscale)[2])->value);

                nodes.insert(std::pair(i, new Node(name, translation, rotation, scale)));
            }
            else if (type == "CAMERA")
            {
                std::string name = jstring::cast((*o)["name"])->value;
                jobject* perspective = jobject::cast((*o)["perspective"]);
                float aspect = jnumber::cast((*perspective)["aspect"])->value;
                float vfov = jnumber::cast((*perspective)["vfov"])->value;
                float near = jnumber::cast((*perspective)["near"])->value;
                jnumber* far = jnumber::cast((*perspective)["far"]);

                Camera* camera;

                if (far != null)
                    camera = new Camera(name, aspect, vfov, near, far->value);
                else
                    camera = new Camera(name, aspect, vfov, near);

                cameras.insert(std::pair(i, camera));
                camerasEnumerated.insert(std::pair(cameraCount, camera));

                if (requestedCameraName == null || strcmp(name.c_str(), requestedCameraName) == 0)
                {
                    currentCamIndex = cameraCount;
                    currentCam = camera;
                }

                cameraCount++;
            }
            else if (type == "MESH")
            {
                std::string name = jstring::cast((*o)["name"])->value;
                std::string topology = jstring::cast((*o)["name"])->value;
                int count = (int) jnumber::cast((*o)["count"])->value;
                jobject* attributes = jobject::cast((*o)["attributes"]);
                Mesh* mesh = new Mesh(this, name, count, topology);

                for (auto ap: attributes->elements)
                {
                    jobject* a = jobject::cast(ap.second);
                    std::string src = jstring::cast((*a)["src"])->value;
                    auto offset = (size_t) jnumber::cast((*a)["offset"])->value;
                    auto stride = (size_t) jnumber::cast((*a)["stride"])->value;
                    std::string format = jstring::cast((*a)["format"])->value;

                    mesh->attributes.insert(std::pair(ap.first, Attribute(src, offset, stride, format)));
                }

                meshes.insert(std::pair(i, mesh));
            }
            else if (type == "DRIVER")
            {
                std::string name = jstring::cast((*o)["name"])->value;
                std::string channel = jstring::cast((*o)["channel"])->value;
                jarray* times = jarray::cast((*o)["times"]);
                jarray* values = jarray::cast((*o)["values"]);
                std::string interpolation = jstring::cast((*o)["interpolation"])->value;

                std::vector<double> timesF (times->elements.size());
                for (int i = 0; i < times->elements.size(); i++)
                {
                    timesF[i] = (float) jnumber::cast((*times)[i])->value;
                }

                if (channel == "translation")
                {
                    std::vector<dvec3> valuesF (times->elements.size());
                    for (int i = 0; i < times->elements.size(); i++)
                    {
                        valuesF[i] = dvec3((float) jnumber::cast((*values)[3 * i])->value, (float) jnumber::cast((*values)[3 * i + 1])->value, (float) jnumber::cast((*values)[3 * i + 2])->value);
                    }
                    translations.insert(std::pair(i, new Driver<dvec3>(name, timesF, valuesF, interpolation)));
                }
                else if (channel == "rotation")
                {
                    std::vector<dvec4> valuesF (times->elements.size());
                    for (int i = 0; i < times->elements.size(); i++)
                    {
                        valuesF[i] = dvec4((float) jnumber::cast((*values)[4 * i])->value, (float) jnumber::cast((*values)[4 * i + 1])->value, (float) jnumber::cast((*values)[4 * i + 2])->value, (float) jnumber::cast((*values)[4 * i + 3])->value);
                    }
                    rotations.insert(std::pair(i, new Driver<dvec4>(name, timesF, valuesF, interpolation)));
                }
                else if (channel == "scale")
                {
                    std::vector<dvec3> valuesF (times->elements.size());
                    for (int i = 0; i < times->elements.size(); i++)
                    {
                        valuesF[i] = dvec3((float) jnumber::cast((*values)[3 * i])->value, (float) jnumber::cast((*values)[3 * i + 1])->value, (float) jnumber::cast((*values)[3 * i + 2])->value);
                    }
                    scales.insert(std::pair(i, new Driver<dvec3>(name, timesF, valuesF, interpolation)));
                }
            }
            else if (type == "MATERIAL")
            {
                std::string name = jstring::cast((*o)["name"])->value;
                jobject* normalMap = jobject::cast((*o)["normalMap"]);
                Texture* normalTex = null;
                if (normalMap != null)
                    normalTex = new Texture(this, normalMap);
                else
                    normalTex = new Texture(this, vec3(0.5, 0.5, 1));

                Material* material;

                if ((*o)["pbr"] != null)
                {
                    auto m = new MaterialPBR();
                    jobject* pbr = jobject::cast((*o)["pbr"]);

                    jvalue* albedo = (*pbr)["albedo"];
                    if (albedo->type == type_array)
                    {
                        jarray* a = jarray::cast(albedo);
                        m->albedoTex = new Texture(this, vec3((float) jnumber::cast((*a)[0])->value, (float) jnumber::cast((*a)[1])->value, (float) jnumber::cast((*a)[2])->value));
                    }
                    else
                        m->albedoTex = new Texture(this, jobject::cast(albedo));

                    jvalue* roughness = (*pbr)["roughness"];
                    if (roughness->type == type_number)
                    {
                        auto v = (float) jnumber::cast(roughness)->value;
                        m->roughnessTex = new Texture(this, vec3(v, v, v));
                    }
                    else
                        m->roughnessTex = new Texture(this, jobject::cast(roughness));


                    jvalue* metalness = (*pbr)["metalness"];
                    if (metalness->type == type_number)
                    {
                        auto v = (float) jnumber::cast(metalness)->value;
                        m->metalnessTex = new Texture(this, vec3(v, v, v));
                    }
                    else
                        m->metalnessTex = new Texture(this, jobject::cast(metalness));

                    material = m;
                }
                else if ((*o)["lambertian"] != null)
                {
                    auto m = new MaterialLambertian();
                    jobject* lamb = jobject::cast((*o)["lambertian"]);

                    jvalue* albedo = (*lamb)["albedo"];
                    if (albedo->type == type_array)
                    {
                        jarray* a = jarray::cast(albedo);
                        m->albedoTex = new Texture(this, vec3((float) jnumber::cast((*a)[0])->value, (float) jnumber::cast((*a)[1])->value, (float) jnumber::cast((*a)[2])->value));
                    }
                    else
                        m->albedoTex = new Texture(this, jobject::cast(albedo));

                    material = m;
                }
                else if ((*o)["mirror"] != null)
                    material = new MaterialMirror();
                else if ((*o)["environment"] != null)
                    material = new MaterialEnvironment();
                else
                    material = new MaterialSimple();

                material->type->instances++;
                material->name = name;
                material->normalMap = normalTex;
                materials.insert(std::pair(i, material));
            }
            else if (type == "ENVIRONMENT")
            {
                std::string name = jstring::cast((*o)["name"])->value;
                jobject* ob = jobject::cast((*o)["radiance"]);
                env = new Environment(name, new Texture(this, ob, 5), new Texture(this, ob, 1, ".l.png"));
                environments.insert(std::pair(i, env));
            }
            else if (type == "LIGHT")
            {
                std::string name = jstring::cast((*o)["name"])->value;
                jarray* a = jarray::cast((*o)["tint"]);
                vec3 tint = vec3((float) jnumber::cast((*a)[0])->value, (float) jnumber::cast((*a)[1])->value, (float) jnumber::cast((*a)[2])->value);

                if ((*o)["sun"] != null)
                {
                    jobject* sun = jobject::cast((*o)["sun"]);
                    float angle = (float) (jnumber::cast((*sun)["angle"])->value);
                    float strength = (float) (jnumber::cast((*sun)["strength"])->value);
                    SunLight* l = new SunLight(name, tint, angle, strength);
                    lights.insert(std::pair(i, l));
                }
                else if ((*o)["sphere"] != null)
                {
                    jobject* sphere = jobject::cast((*o)["sphere"]);
                    float radius = (float) (jnumber::cast((*sphere)["radius"])->value);
                    float power = (float) (jnumber::cast((*sphere)["power"])->value);

                    auto lim = jnumber::cast((*sphere)["limit"]);
                    SpotLight* l;

                    if (lim == null)
                        l = new SpotLight(name, tint, radius, power);
                    else
                        l = new SpotLight(name, tint, radius, power, (float) (lim->value));

                    lights.insert(std::pair(i, l));
                }
                else if ((*o)["spot"] != null)
                {
                    jobject* spot = jobject::cast((*o)["spot"]);
                    float radius = (float) (jnumber::cast((*spot)["radius"])->value);
                    float power = (float) (jnumber::cast((*spot)["power"])->value);
                    auto lim = jnumber::cast((*spot)["limit"]);
                    float limit;
                    float fov = (float) (jnumber::cast((*spot)["fov"])->value);
                    float blend = (float) (jnumber::cast((*spot)["blend"])->value);
                    int shadow;

                    auto jshadow = jnumber::cast((*o)["shadow"]);

                    if (lim != null)
                        limit = (float) (lim->value);
                    else
                        limit = std::numeric_limits<float>::infinity();

                    if (jshadow != null)
                        shadow = (int) (jshadow->value);
                    else
                        shadow = 0;

                    SpotLight* l = new SpotLight(name, tint, radius, power, limit, fov, blend, shadow);
                    lights.insert(std::pair(i, l));
                }
            }

            i++;
        }

        //Populate pointers
        i = 0;
        for (jvalue* v: sceneArray->elements)
        {
            if (i == 0)
            {
                i++;
                continue;
            }

            jobject *o = static_cast<jobject *>(v);
            std::string type = jstring::cast((*o)["type"])->value;
            if (type == "SCENE")
            {
                jarray* roots = jarray::cast((*o)["roots"]);
                for (jvalue* v: roots->elements)
                {
                    int n = (int) jnumber::cast(v)->value;
                    scene->roots.emplace_back(nodes.at(n));
                }
            }
            else if (type == "NODE")
            {
                jarray* kids = jarray::cast((*o)["children"]);
                if (kids != null)
                {
                    for (jvalue *v: kids->elements)
                    {
                        int n = (int) jnumber::cast(v)->value;
                        nodes[i]->children.emplace_back(nodes.at(n));
                    }
                }

                jnumber* camera = jnumber::cast((*o)["camera"]);
                if (camera != null)
                    nodes[i]->camera = cameras.at((int) camera->value);

                jnumber* mesh = jnumber::cast((*o)["mesh"]);
                if (mesh != null)
                    nodes[i]->mesh = meshes.at((int) mesh->value);

                jnumber* environment = jnumber::cast((*o)["environment"]);
                if (environment != null)
                    nodes[i]->environment = environments.at((int) environment->value);

                jnumber* light = jnumber::cast((*o)["light"]);
                if (light != null)
                    nodes[i]->light = lights.at((int) light->value);
            }
            else if (type == "DRIVER")
            {
                int node = (int) jnumber::cast((*o)["node"])->value;
                std::string channel = jstring::cast((*o)["channel"])->value;
                if (channel == "translation")
                    nodes[node]->translationDriver = translations[i];
                else if (channel == "rotation")
                    nodes[node]->rotationDriver = rotations[i];
                else if (channel == "scale")
                    nodes[node]->scaleDriver = scales[i];
            }
            else if (type == "MESH")
            {
                jnumber* material = jnumber::cast((*o)["material"]);
                if (material != null)
                    meshes[i]->material = materials.at((int) material->value);
            }

            i++;
        }

        camerasEnumerated[0] = scene->detachedCamera;

        scene->nodes = nodes;
        scene->cameras = cameras;
        scene->camerasEnumerated = camerasEnumerated;
        scene->meshes = meshes;
        scene->currentCameraIndex = currentCamIndex;
        scene->cameraCount = cameraCount;
        scene->translationDrivers = translations;
        scene->rotationDrivers = rotations;
        scene->scaleDrivers = scales;
        scene->environments = environments;
        scene->materials = materials;
        scene->lights = lights;

        scene->defaultEnvironment = new Environment("default", new Texture(this, vec3(0, 0, 0), true), new Texture(this, vec3(0, 0, 0), true));

        if (env != null)
            scene->environment = env;
        else
            scene->environment = scene->defaultEnvironment;

        if (currentCam != null)
            scene->currentCamera = currentCam;
        else if (requestedCameraName != null)
        {
            printf("Did not find any cameras matching requested camera name \"%s\". Try one of these:\n", requestedCameraName);

            for (auto p: cameras)
            {
                printf("camera: \"%s\"\n", p.second->name.c_str());
            }

            abort();
        }

        return scene;
    }

    void initMeshes()
    {
        for (auto m: scene->meshes)
        {
            m.second->initialize();
        }
    }

    char* sceneName = null;
    bool enableCulling = true;
    bool headless = false;
    int displayWidth = 1280;
    int displayHeight = 720;
    char* requestedCameraName = null;
    char* requestedPhysicalDeviceName = null;
    bool logStats = false;
    bool hdr = false;

    int headlessIndex = 0;
    long swapchainTime = 0;

    struct HeadlessEvent
    {
        long time;
        int type;

        float animTime;
        float animRate;
        std::string text;

        HeadlessEvent(long time)
        {
            this->time = time;
            this->type = 0;
        }

        HeadlessEvent(long time, float t, float r)
        {
            this->time = time;
            this->type = 1;
        }

        HeadlessEvent(long time, std::string text, bool save)
        {
            this->time = time;
            this->type = 2;
            this->text = text;

            if (!save)
                this->type = 3;
        }
    };

    std::vector<HeadlessEvent> headlessEvents {};

    void readHeadlessEvents(std::string file)
    {
        auto text = readFile(file);
        std::string textStr (text.begin(), text.end());
        int lineStart = 0;
        int mode = 0;
        long time;
        int type = -1;
        float animTime;
        for (int i = 0; i < text.size(); i++)
        {
            if (mode == 0 && text[i] == ' ')
            {
                std::string s = textStr.substr(lineStart, i - lineStart);
                time = std::stoi(s);
                lineStart = i + 1;
                mode = 1;
            }
            else if (mode == 1 && text[i] == ' ')
            {
                std::string s = textStr.substr(lineStart, i - lineStart);
                if (s == "PLAY")
                    type = 1;
                else if (s == "SAVE")
                    type = 2;
                else
                    type = 3;

                lineStart = i + 1;

                mode = 2;
            }
            else if (mode == 2 && type == 1 && text[i] == ' ')
            {
                std::string s = textStr.substr(lineStart, i - lineStart);
                mode = 3;
                animTime = std::stof(s);
                lineStart = i + 1;
            }

            if (text[i] == '\n')
            {
                if (mode == 1)
                {
                    std::string s = textStr.substr(lineStart, i - lineStart);
                    if (s == "AVAILABLE")
                        headlessEvents.emplace_back(HeadlessEvent(time));
                }
                else if (mode == 2 && type == 3)
                {
                    std::string s = textStr.substr(lineStart, i - lineStart);
                    headlessEvents.emplace_back(HeadlessEvent(time, std::string(s), false));
                }
                else if (mode == 2 && type == 2)
                {
                    std::string s = textStr.substr(lineStart, i - lineStart);
                    headlessEvents.emplace_back(HeadlessEvent(time, std::string(s), true));
                }
                else if (mode == 3 && type == 1)
                {
                    std::string s = textStr.substr(lineStart, i - lineStart);
                    headlessEvents.emplace_back(HeadlessEvent(time, animTime, std::stof(s)));
                }

                mode = 0;
                lineStart = i + 1;
            }
        }
    }

private:
    GLFWwindow* window;

    VkInstance instance;
    VkDebugUtilsMessengerEXT debugMessenger;

    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device;

    VkQueue graphicsQueue;
    VkQueue presentQueue;

    VkSurfaceKHR surface;

    VkSwapchainKHR swapChain = VK_NULL_HANDLE;
    std::vector<VkImage> swapChainImages;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;
    std::vector<VkImageView> swapChainImageViews;

    std::vector<VkDeviceMemory> swapChainImageMemory;

    VkImage depthImage;
    VkDeviceMemory depthImageMemory;
    VkImageView depthImageView;

    VkRenderPass renderPass;
    VkRenderPass shadowRenderPass;

    std::vector<VkFramebuffer> swapChainFramebuffers;

    std::vector<VkBuffer> globalUniformBuffers;
    std::vector<VkDeviceMemory> globalUniformBuffersMemory;
    std::vector<void*> globalUniformBuffersMapped;

    std::vector<VkBuffer> shaderStorageBuffers;
    std::vector<VkDeviceMemory> shaderStorageBuffersMemory;

    VkDescriptorPool globalDescriptorPool;
    VkDescriptorSetLayout globalDescriptorSetLayout;
    std::vector<VkDescriptorSet> globalDescriptorSets;

    VkCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;

    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;

    std::vector<int> keysPressed;

    int mouseGrabbed = 0;
    bool rightMouseDown = false;

    double mouseGrabX;
    double mouseGrabY;
    double mouseLastGrabX;
    double mouseLastGrabY;

    MaterialType* currentMaterialType;
    Material defaultMaterial = MaterialSimple();

    u32 currentFrame = 0;
    bool framebufferResized = false;

    Scene* scene = null;

    float frameTime = 0;

    bool timeRecorded = false;
    std::chrono::steady_clock::time_point lastRecordedTime;
    float currentTime = 0;
    float timeRate = 1.0;

    void initWindow()
    {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

        if (displayWidth <= 0 || displayHeight <= 0)
        {
            printf("Window size requested (%dx%d) must be positve!\n", displayWidth, displayHeight);
            abort();
        }

        window = glfwCreateWindow(displayWidth, displayHeight, "Very Cool Renderer", null, null);

        int w;
        int h;

        glfwGetWindowSize(window, &w, &h);
        if (w != displayWidth || h != displayHeight)
        {
            printf("Window size requested (%dx%d) is not supported (got %dx%d)!\n", displayWidth, displayHeight, swapChainExtent.width, swapChainExtent.height);
            abort();
        }
        glfwSetWindowUserPointer(window, this);
        glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
        glfwSetKeyCallback(window, keyCallback);
        glfwSetMouseButtonCallback(window, mouseCallback);
        glfwSetScrollCallback(window, scrollCallback);
    }

    static void framebufferResizeCallback(GLFWwindow* window, int width, int height)
    {
        auto app = reinterpret_cast<Renderer*>(glfwGetWindowUserPointer(window));
        app->framebufferResized = true;
    }

    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
    {
        auto app = reinterpret_cast<Renderer*>(glfwGetWindowUserPointer(window));
        if (action == GLFW_PRESS)
            app->keysPressed.emplace_back(key);
        else if (action == GLFW_RELEASE)
            std::erase(app->keysPressed, key);
    }

    static void mouseCallback(GLFWwindow* window, int mouse, int action, int mods)
    {
        auto app = reinterpret_cast<Renderer*>(glfwGetWindowUserPointer(window));
        if (action == GLFW_PRESS)
        {
            if (mouse == GLFW_MOUSE_BUTTON_2)
                app->rightMouseDown = true;

            app->mouseGrabbed = 1;
            glfwGetCursorPos(window, &app->mouseGrabX, &app->mouseGrabY);
            glfwSetCursorPos(window, app->swapChainExtent.width / 2, app->swapChainExtent.height / 2);
            app->mouseLastGrabX = app->swapChainExtent.width / 2;
            app->mouseLastGrabY = app->swapChainExtent.height / 2;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
        else if (action == GLFW_RELEASE)
        {
            if (mouse == GLFW_MOUSE_BUTTON_2)
                app->rightMouseDown = false;

            app->mouseGrabbed = 0;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            glfwSetCursorPos(window, app->mouseGrabX, app->mouseGrabY);
        }
    }

    static void scrollCallback(GLFWwindow* window, double x, double y)
    {
        auto app = reinterpret_cast<Renderer*>(glfwGetWindowUserPointer(window));
        Camera* c = app->scene->currentCamera;

        if (app->scene->debugCameraMode)
            c = app->scene->debugCamera;

        if (app->keyDown(GLFW_KEY_LEFT_CONTROL))
            c->speed += (float) y;
        else
        {
            c->zoom -= (float) y / 500.0f;
            c->computeTransform();
        }
    }

    bool keyDown(int key)
    {
        return (std::find(keysPressed.begin(), keysPressed.end(), key) != keysPressed.end());
    }

    void initialize()
    {
        createInstance();
        setupDebugMessenger();

        if (!headless)
            createSurface();

        pickPhysicalDevice();
        createLogicalDevice();
        createSwapChain();
        createImageViews();
        createRenderPass();
        createShadowMapRenderPass();

        for (MaterialType* t: materialTypes)
        {
            t->createDescriptorSetLayout(device);
        }

        createCommandPool();

        load();

        createDescriptorSetLayout();
        createGraphicsPipelines();
        createDepthResources();
        createFramebuffers();
        createShadowMapFramebuffers();
        createUniformBuffers();
        createDescriptorPool();
        createCommandBuffers();
        createSyncObjects();

        createSSBOs();
        createDescriptorSets();
        initMeshes();
    }

    void createInstance()
    {
        if (enable_validation_layers && !checkValidationLayerSupport())
            throw std::runtime_error("requested but didn't find validation layers");

        VkApplicationInfo appInfo {};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Renderer";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "No Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_2;

        VkInstanceCreateInfo createInfo {};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;

        auto extensions = getRequiredExtensions();

        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo {};
        if (enable_validation_layers)
        {
            createInfo.enabledLayerCount = static_cast<u32>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();

            populateDebugMessengerCreateInfo(debugCreateInfo);
            createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;
        }
        else
        {
            createInfo.enabledLayerCount = 0;
            createInfo.pNext = null;
        }

        // mac support
        extensions.emplace_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
        extensions.emplace_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);

        createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
        createInfo.enabledExtensionCount = (u32) extensions.size();
        createInfo.ppEnabledExtensionNames = extensions.data();

        createInfo.enabledLayerCount = 0;
        if (enable_validation_layers)
        {
            createInfo.enabledLayerCount = static_cast<u32>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();
        }

        VkResult result = vkCreateInstance(&createInfo, null, &instance);
        if (result != VK_SUCCESS)
        {
            printf("failed: %d\n", result);
            throw std::runtime_error("instance creation failed!");
        }
    }

    bool checkValidationLayerSupport()
    {
        u32 layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, null);
        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        for (const char* layerName: validationLayers)
        {
            bool found = false;
            for (const auto& layerProps : availableLayers)
            {
                if (strcmp(layerName, layerProps.layerName) == 0)
                {
                    found = true;
                    break;
                }
            }

            if (!found)
                return false;
        }

        return true;
    }

    std::vector<const char *> getRequiredExtensions()
    {
        if (!headless)
        {
            u32 glfwExtensionCount = 0;
            const char **glfwExtensions;
            glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

            std::vector<const char *> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

            if (enable_validation_layers)
                extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

            if (hdr)
                extensions.push_back(VK_EXT_SWAPCHAIN_COLOR_SPACE_EXTENSION_NAME);

            return extensions;
        }
        else
        {
            std::vector<const char *> extensions;
            if (enable_validation_layers)
                extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

            return extensions;
        }
    }

    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
    {
        createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = debugCallback;
    }

    void setupDebugMessenger()
    {
        if (!enable_validation_layers)
            return;

        VkDebugUtilsMessengerCreateInfoEXT createInfo;
        populateDebugMessengerCreateInfo(createInfo);

        auto result = createDebugUtilsMessengerEXT(instance, &createInfo, null, &debugMessenger);
        if (result != VK_SUCCESS)
        {
            printf("failed: %d\n", result);
            throw std::runtime_error("failed to setup debug messenger!");
        }
    }

    void createSurface()
    {
        auto result = glfwCreateWindowSurface(instance, window, null, &surface);
        if (result != VK_SUCCESS)
        {
            printf("failed: %d\n", result);
            throw std::runtime_error("window surface creation failed!");
        }
    }

    void pickPhysicalDevice()
    {
        u32 devicesCount = 0;
        vkEnumeratePhysicalDevices(instance, &devicesCount, null);

        if (devicesCount == 0)
            throw std::runtime_error("no capable GPUs found!");

        std::vector<VkPhysicalDevice> devices(devicesCount);
        vkEnumeratePhysicalDevices(instance, &devicesCount, devices.data());

        for (const auto & d : devices)
        {
            if (isDeviceSuitable(d))
            {
                physicalDevice = d;
                break;
            }
        }

        if (physicalDevice == VK_NULL_HANDLE)
        {
            if (requestedPhysicalDeviceName == null)
                printf("No suitable physical devices found!\n");
            else
                printf("No physical devices matched requested name \"%s\" - try one of the ones printed above!\n", requestedPhysicalDeviceName);

            abort();
        }
    }

    struct QueueFamilyIndices
    {
        std::optional<u32> graphicsFamily;
        std::optional<u32> presentFamily;

        bool isComplete()
        {
            return graphicsFamily.has_value() && presentFamily.has_value();
        }
    };

    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice d)
    {
        QueueFamilyIndices indices;

        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(d, &queueFamilyCount, null);

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(d, &queueFamilyCount, queueFamilies.data());

        int i = 0;
        for (const auto & qf: queueFamilies)
        {
            if (qf.queueFlags & VK_QUEUE_GRAPHICS_BIT)
                indices.graphicsFamily = i;

            VkBool32 presentSupport = false;

            if (!headless)
                vkGetPhysicalDeviceSurfaceSupportKHR(d, i, surface, &presentSupport);

            if (presentSupport || headless)
                indices.presentFamily = i;

            if (indices.isComplete())
                break;

            i++;
        }

        return indices;
    }

    bool checkDeviceExtensionSupport(VkPhysicalDevice d)
    {
        u32 extensionCount;
        vkEnumerateDeviceExtensionProperties(d, null, &extensionCount, null);

        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(d, null, &extensionCount, availableExtensions.data());

        auto de = deviceExtensions;
        if (headless)
            de = deviceExtensionsHeadless;

        std::set<std::string> requiredExtensions(de.begin(), de.end());

        for (const auto & e: availableExtensions)
        {
            requiredExtensions.erase(e.extensionName);
        }

        return requiredExtensions.empty();
    }

    struct SwapChainSupportDetails
    {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };

    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice d)
    {
        SwapChainSupportDetails details;

        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(d, surface, &details.capabilities);

        u32 formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(d, surface, &formatCount, null);

        if (formatCount != 0)
        {
            details.formats.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(d, surface, &formatCount, details.formats.data());
        }

        u32 presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(d, surface, &presentModeCount, null);

        if (presentModeCount != 0)
        {
            details.presentModes.resize(formatCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(d, surface, &presentModeCount, details.presentModes.data());
        }

        return details;
    }

    bool isDeviceSuitable(VkPhysicalDevice d)
    {
        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(d, &props);

        if (headless)
            return true;

        if (requestedPhysicalDeviceName != null)
        {
            bool match = strcmp(requestedPhysicalDeviceName, props.deviceName) == 0;
            printf("device found: \"%s\"\n", props.deviceName);

            if (match)
                printf("device matched name!\n");

            return match;
        }

        QueueFamilyIndices indices = findQueueFamilies(d);

        bool extensionsSupported = checkDeviceExtensionSupport(d);

        bool swapChainAdequate = false;
        if (extensionsSupported)
        {
            SwapChainSupportDetails s = querySwapChainSupport(d);

            VkPhysicalDeviceFeatures supportedFeatures;
            vkGetPhysicalDeviceFeatures(d, &supportedFeatures);

            swapChainAdequate = !s.formats.empty() && !s.presentModes.empty() && supportedFeatures.samplerAnisotropy;
        }

        return indices.isComplete() && extensionsSupported && swapChainAdequate;
    }

    void createLogicalDevice()
    {
        QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::set<u32> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };

        float queuePriority = 1.0f;

        for (u32 queueFamily : uniqueQueueFamilies)
        {
            VkDeviceQueueCreateInfo queueCreateInfo {};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = indices.graphicsFamily.value();
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;
            queueCreateInfos.push_back(queueCreateInfo);
        }

        VkPhysicalDeviceVulkan12Features device12Features {};
        device12Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
        device12Features.runtimeDescriptorArray = true;

        VkPhysicalDeviceFeatures2 deviceFeatures {};
        deviceFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        deviceFeatures.features.samplerAnisotropy = VK_TRUE;
        deviceFeatures.pNext = &device12Features;

        VkDeviceCreateInfo createInfo {};

        std::vector<const char*> extensions;

        if (!headless)
            extensions.emplace_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

        // Fix for mac: https://stackoverflow.com/questions/66659907/vulkan-validation-warning-catch-22-about-vk-khr-portability-subset-on-moltenvk
        extensions.emplace_back("VK_KHR_portability_subset");

        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.pQueueCreateInfos = queueCreateInfos.data();
        createInfo.queueCreateInfoCount = static_cast<u32>(queueCreateInfos.size());
        createInfo.pNext = &deviceFeatures;

        createInfo.enabledExtensionCount = extensions.size();
        createInfo.ppEnabledExtensionNames = extensions.data();

        if (enable_validation_layers)
        {
            createInfo.enabledLayerCount = validationLayers.size();
            createInfo.ppEnabledLayerNames = validationLayers.data();
        }
        else
            createInfo.enabledLayerCount = 0;

        auto result = vkCreateDevice(physicalDevice, &createInfo, null, &device);
        if (result != VK_SUCCESS)
        {
            printf("failed: %d\n", result);
            throw std::runtime_error("creating logical device failed!");
        }

        vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
        vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
    }

    void createHeadlessSwapChainImages(int width, int height, int count)
    {
        VkDeviceSize imageSize = width * height * 4;

        swapChainImageFormat = VK_FORMAT_R8G8B8A8_SRGB;
        for (int i = 0; i < count; i++)
        {
            void *data;
            createImage(width, height, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
                        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                        swapChainImages[i], swapChainImageMemory[i]);

            createSwapChainImageViews(i);
        }
    }

    void createSwapChainImageViews(int i)
    {
        swapChainImageViews[i] = createImageView(swapChainImages[i], VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);
    }


    void createSwapChainHeadless(VkExtent2D extent, int count)
    {
        swapChainImages.resize(count);
        swapChainImageViews.resize(count);
        swapChainImageMemory.resize(count);

        swapChainExtent = extent;

        createHeadlessSwapChainImages(extent.width, extent.height, count);
    }

    void createSwapChain()
    {
        if (headless)
        {
            createSwapChainHeadless({1000, 1000}, 3);
            return;
        }

        SwapChainSupportDetails s = querySwapChainSupport(physicalDevice);
        VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(s.formats);
        VkPresentModeKHR presentMode = chooseSwapPresentMode(s.presentModes);
        VkExtent2D extent = chooseSwapExtent(s.capabilities);

        u32 imageCount = s.capabilities.minImageCount + 1;

        if (s.capabilities.maxImageCount > 0 && imageCount > s.capabilities.maxImageCount)
            imageCount = s.capabilities.maxImageCount;

        VkSwapchainCreateInfoKHR createInfo {};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = surface;
        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
        u32 queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

        if (indices.graphicsFamily != indices.presentFamily)
        {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        }
        else
        {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            createInfo.queueFamilyIndexCount = 0;
            createInfo.pQueueFamilyIndices = null;
        }

        createInfo.preTransform = s.capabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;

        auto result = vkCreateSwapchainKHR(device, &createInfo, null, &swapChain);
        if (result != VK_SUCCESS)
        {
            printf("failed: %d\n", result);
            throw std::runtime_error("swap chain creation failed!");
        }

        vkGetSwapchainImagesKHR(device, swapChain, &imageCount, null);
        swapChainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

        swapChainImageFormat = surfaceFormat.format;
        swapChainExtent = extent;
    }

    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
    {
        for (const auto & f: availableFormats)
        {
            if (hdr && f.format == VK_FORMAT_R16G16B16A16_SFLOAT && f.colorSpace == VK_COLOR_SPACE_HDR10_ST2084_EXT)
                return f;

            if (!hdr && f.format == VK_FORMAT_B8G8R8A8_UNORM && f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
                return f;
        }

        return availableFormats[0];
    }

    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& available)
    {
        for (const auto & m : available)
        {
            if (m == VK_PRESENT_MODE_MAILBOX_KHR)
                return m;
        }

        // Guaranteed
        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& caps)
    {
        if (caps.currentExtent.width != std::numeric_limits<u32>::max())
            return caps.currentExtent;
        else
        {
            int w;
            int h;
            glfwGetFramebufferSize(window, &w, &h);
            VkExtent2D extent = {static_cast<u32> (w), static_cast<u32> (h)};
            extent.width = std::clamp(extent.width, caps.minImageExtent.width, caps.maxImageExtent.width);
            extent.height = std::clamp(extent.height, caps.minImageExtent.height, caps.maxImageExtent.height);
            return extent;
        }
    }

    void createImageViews()
    {
        swapChainImageViews.resize(swapChainImages.size());

        for (int i = 0; i < swapChainImages.size(); i++)
        {
            VkImageViewCreateInfo createInfo {};
            createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            createInfo.image = swapChainImages[i];
            createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            createInfo.format = swapChainImageFormat;
            createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            createInfo.subresourceRange.baseMipLevel = 0;
            createInfo.subresourceRange.levelCount = 1;
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.layerCount = 1;

            auto result = vkCreateImageView(device, &createInfo, null, &swapChainImageViews[i]);
            if (result != VK_SUCCESS)
            {
                printf("failed: %d\n", result);
                throw std::runtime_error("image view creation failed!");
            }
        }
    }

    void createRenderPass()
    {
        VkAttachmentDescription colorAttachment {};
        colorAttachment.format = swapChainImageFormat;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        if (!headless)
            colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        else
            colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentDescription depthAttachment{};
        depthAttachment.format = findDepthFormat();
        depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentReference colorAttachmentRef {};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentReference depthAttachmentRef{};
        depthAttachmentRef.attachment = 1;
        depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass {};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;
        subpass.pDepthStencilAttachment = &depthAttachmentRef;

        VkSubpassDependency dependency {};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        dependency.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        std::array<VkAttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};
        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        renderPassInfo.pAttachments = attachments.data();
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

        auto result = vkCreateRenderPass(device, &renderPassInfo, null, &renderPass);
        if (result != VK_SUCCESS)
        {
            printf("failed: %d\n", result);
            throw std::runtime_error("render pass creation failed!");
        }
    }

    // Adapted from https://github.com/SaschaWillems/Vulkan/blob/master/examples/shadowmapping/shadowmapping.cpp#L192
    void createShadowMapRenderPass()
    {
        VkAttachmentDescription depthAttachment{};
        depthAttachment.format = findDepthFormat();
        depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

        VkAttachmentReference depthAttachmentRef{};
        depthAttachmentRef.attachment = 0;
        depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass {};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 0;
        subpass.pDepthStencilAttachment = &depthAttachmentRef;

        std::array<VkSubpassDependency, 2> dependencies {};
        dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[0].dstSubpass = 0;
        dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        dependencies[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        dependencies[1].srcSubpass = 0;
        dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[1].srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        dependencies[1].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        std::array<VkAttachmentDescription, 1> attachments = {depthAttachment};
        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        renderPassInfo.pAttachments = attachments.data();
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = dependencies.size();
        renderPassInfo.pDependencies = dependencies.data();

        auto result = vkCreateRenderPass(device, &renderPassInfo, null, &shadowRenderPass);
        if (result != VK_SUCCESS)
        {
            printf("failed: %d\n", result);
            throw std::runtime_error("shadow map render pass creation failed!");
        }
    }

    template<typename T>
    void createGraphicsPipeline(MaterialType &material, std::string vert, std::string frag, std::vector<VkPushConstantRange> uniforms)
    {
        auto vertShaderCode = readFile(vert);
        VkShaderModule vertSM = createShaderModule(vertShaderCode);
        VkPipelineShaderStageCreateInfo vertSSI {};
        vertSSI.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertSSI.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertSSI.module = vertSM;
        vertSSI.pName = "main";

        auto fragShaderCode = readFile(frag);
        VkShaderModule fragSM = createShaderModule(fragShaderCode);
        VkPipelineShaderStageCreateInfo fragSSI {};
        fragSSI.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragSSI.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragSSI.module = fragSM;
        fragSSI.pName = "main";

        VkPipelineShaderStageCreateInfo shaderStages[] = { vertSSI, fragSSI };

        auto bindDesc = T::getBindDesc();
        auto attribDesc = T::getAttributeDesc();
        material.stride = sizeof(T);

        VkPipelineVertexInputStateCreateInfo vertII {};
        vertII.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertII.vertexBindingDescriptionCount = 1;
        vertII.vertexAttributeDescriptionCount = static_cast<u32>(attribDesc.size());
        vertII.pVertexBindingDescriptions = &bindDesc;
        vertII.pVertexAttributeDescriptions = attribDesc.data();

        VkPipelineInputAssemblyStateCreateInfo inputAssembly {};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        VkViewport viewport {};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float) swapChainExtent.width;
        viewport.height = (float) swapChainExtent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissor {};
        scissor.offset = {0, 0};
        scissor.extent = swapChainExtent;

        std::vector<VkDynamicState> dynamicStates =
                {
                        VK_DYNAMIC_STATE_VIEWPORT,
                        VK_DYNAMIC_STATE_SCISSOR
                };

        VkPipelineDynamicStateCreateInfo dynamicState {};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = static_cast<u32>(dynamicStates.size());
        dynamicState.pDynamicStates = dynamicStates.data();

        VkPipelineViewportStateCreateInfo viewportState {};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.pViewports = &viewport;
        viewportState.scissorCount = 1;
        viewportState.pScissors = &scissor;

        VkPipelineRasterizationStateCreateInfo rasterizer {};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthBiasEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;

        if (scene->drawingShadow)
            rasterizer.cullMode = VK_CULL_MODE_FRONT_BIT;
        else
            rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;

        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;

        VkPipelineMultisampleStateCreateInfo multisampling {};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        VkPipelineColorBlendAttachmentState colorBlendAttachment {};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_TRUE;
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

        VkPipelineColorBlendStateCreateInfo colorBlending {};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;

        std::array<VkDescriptorSetLayout, 2> layouts {globalDescriptorSetLayout, material.descriptorSetLayout};
        VkPipelineLayoutCreateInfo pipelineLayoutInfo {};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = layouts.size();
        pipelineLayoutInfo.pSetLayouts = layouts.data();

        pipelineLayoutInfo.pPushConstantRanges = uniforms.data();
        pipelineLayoutInfo.pushConstantRangeCount = uniforms.size();

        auto pipeline = (material.getPipeline(scene));
        auto result = vkCreatePipelineLayout(device, &pipelineLayoutInfo, null, &(pipeline->layout));
        if (result != VK_SUCCESS)
        {
            printf("failed: %d\n", result);
            throw std::runtime_error("pipeline layout creation failed!");
        }

        VkPipelineDepthStencilStateCreateInfo depthStencil{};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = VK_TRUE;
        depthStencil.depthWriteEnable = VK_TRUE;
        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.stencilTestEnable = VK_FALSE;

        VkGraphicsPipelineCreateInfo pipelineInfo {};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;

        pipelineInfo.pVertexInputState = &vertII;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.layout = pipeline->layout;

        if (scene->drawingShadow)
            pipelineInfo.renderPass = shadowRenderPass;
        else
            pipelineInfo.renderPass = renderPass;

        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
        pipelineInfo.basePipelineIndex = -1;
        pipelineInfo.pDepthStencilState = &depthStencil;

        auto result1 = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, null, &(pipeline->pipeline));
        if (result1 != VK_SUCCESS)
        {
            printf("failed: %d\n", result);
            throw std::runtime_error("graphics pipeline creation failed!");
        }

        vkDestroyShaderModule(device, vertSM, null);
        vkDestroyShaderModule(device, fragSM, null);
    }

    VkShaderModule createShaderModule(const std::vector<char>& code)
    {
        VkShaderModuleCreateInfo createInfo {};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<const u32*>(code.data());

        VkShaderModule shaderModule;
        auto result = vkCreateShaderModule(device, &createInfo, null, &shaderModule);
        if (result != VK_SUCCESS)
        {
            printf("failed: %d\n", result);
            throw std::runtime_error("shader module creation failed!");
        }

        return shaderModule;
    }

    void createFramebuffers()
    {
        swapChainFramebuffers.resize(swapChainImageViews.size());

        for (size_t i = 0; i < swapChainImageViews.size(); i++)
        {
            VkImageView attachments[] = { swapChainImageViews[i], depthImageView };
            VkFramebufferCreateInfo framebufferInfo {};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = renderPass;
            framebufferInfo.attachmentCount = 2;
            framebufferInfo.pAttachments = attachments;
            framebufferInfo.width = swapChainExtent.width;
            framebufferInfo.height = swapChainExtent.height;
            framebufferInfo.layers = 1;

            auto result = vkCreateFramebuffer(device, &framebufferInfo, null, &swapChainFramebuffers[i]);
            if (result != VK_SUCCESS)
            {
                printf("failed: %d\n", result);
                throw std::runtime_error("framebuffer creation failed!");
            }
        }
    }

    // Adapted from https://github.com/SaschaWillems/Vulkan/blob/master/examples/shadowmapping/shadowmapping.cpp#L192
    void createShadowMapFramebuffers()
    {
        for (ShadowMap &sm: scene->shadowMaps)
        {
            for (size_t i = 0; i < sm.framebuffers.size(); i++)
            {
                createImage(sm.resolution, sm.resolution, findDepthFormat(), VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                            sm.images[i], sm.imageMemories[i]);
                sm.imageViews[i] = createImageView(sm.images[i], findDepthFormat(), VK_IMAGE_ASPECT_DEPTH_BIT);

                VkSamplerCreateInfo samplerCreateInfo {};
                samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
                samplerCreateInfo.magFilter = VK_FILTER_NEAREST;
                samplerCreateInfo.minFilter = VK_FILTER_NEAREST;
                samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
                samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
                samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
                samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
                samplerCreateInfo.mipLodBias = 0.0f;
                samplerCreateInfo.maxAnisotropy = 1.0f;
                samplerCreateInfo.minLod = 0.0f;
                samplerCreateInfo.maxLod = 1.0f;
                samplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
                auto result = vkCreateSampler(device, &samplerCreateInfo, null, &(sm.samplers[i]));
                if (result != VK_SUCCESS)
                {
                    printf("failed: %d\n", result);
                    throw std::runtime_error("shadow map sampler creation failed!");
                }

                VkFramebufferCreateInfo framebufferCreateInfo {};
                framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
                framebufferCreateInfo.renderPass = shadowRenderPass;
                framebufferCreateInfo.attachmentCount = 1;
                framebufferCreateInfo.pAttachments = &(sm.imageViews[i]);
                framebufferCreateInfo.width = sm.resolution;
                framebufferCreateInfo.height = sm.resolution;
                framebufferCreateInfo.layers = 1;

                result = vkCreateFramebuffer(device, &framebufferCreateInfo, null, &(sm.framebuffers[i]));
                if (result != VK_SUCCESS)
                {
                    printf("failed: %d\n", result);
                    throw std::runtime_error("framebuffer creation failed!");
                }
            }
        }
    }

    void createGraphicsPipelines()
    {
        // Adapted from https://vkguide.dev/docs/chapter-3/push_constants/
        VkPushConstantRange matrices;
        matrices.offset = 0;
        matrices.size = sizeof(PushConstant);
        matrices.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        createGraphicsPipeline<SimpleVertex>(materialTypeSimple, "spv/simple.vert.spv",  "spv/simple.frag.spv", {matrices});
        createGraphicsPipeline<ComplexVertex>(materialTypeEnvironment, "spv/complex.vert.spv", "spv/environment.frag.spv", {matrices});
        createGraphicsPipeline<ComplexVertex>(materialTypeMirror, "spv/complex.vert.spv", "spv/mirror.frag.spv", {matrices});
        createGraphicsPipeline<ComplexVertex>(materialTypeLambertian, "spv/complex.vert.spv", "spv/lambertian.frag.spv", {matrices});
        createGraphicsPipeline<ComplexVertex>(materialTypePBR, "spv/complex.vert.spv", "spv/pbr.frag.spv", {matrices});

        scene->drawingShadow = true;
        createGraphicsPipeline<SimpleVertex>(materialTypeSimple, "spv/simple.vert.spv",  "spv/depth_simple.frag.spv", {matrices});
        createGraphicsPipeline<ComplexVertex>(materialTypeEnvironment, "spv/complex.vert.spv", "spv/depth_complex.frag.spv", {matrices});
        createGraphicsPipeline<ComplexVertex>(materialTypeMirror, "spv/complex.vert.spv", "spv/depth_complex.frag.spv", {matrices});
        createGraphicsPipeline<ComplexVertex>(materialTypeLambertian, "spv/complex.vert.spv", "spv/depth_complex.frag.spv", {matrices});
        createGraphicsPipeline<ComplexVertex>(materialTypePBR, "spv/complex.vert.spv", "spv/depth_complex.frag.spv", {matrices});
        scene->drawingShadow = false;
    }

    void destroyGraphicsPipelines()
    {
        for (MaterialType* t: materialTypes)
        {
            vkDestroyPipeline(device, t->pipeline.pipeline, null);
            vkDestroyPipelineLayout(device, t->pipeline.layout, null);
            vkDestroyPipeline(device, t->shadowMapPipeline.pipeline, null);
            vkDestroyPipelineLayout(device, t->shadowMapPipeline.layout, null);
        }
    }

    void createCommandPool()
    {
        QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);
        VkCommandPoolCreateInfo poolInfo {};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
        auto result = vkCreateCommandPool(device, &poolInfo, null, &commandPool);
        if (result != VK_SUCCESS)
        {
            printf("failed: %d\n", result);
            throw std::runtime_error("command pool creation failed!");
        }
    }

    void createSSBOs()
    {
        shaderStorageBuffers.resize(max_frames_in_flight);
        shaderStorageBuffersMemory.resize(max_frames_in_flight);

        for (int i = 0; i < max_frames_in_flight; i++)
        {
            VkDeviceSize size = scene->shaderLights.size() * sizeof(ShaderLight) + 16;
            createBuffer(size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                               VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                         shaderStorageBuffers[i], shaderStorageBuffersMemory[i]);
        }
    }

    VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
    {
        for (VkFormat format: candidates)
        {
            VkFormatProperties props;
            vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);
            if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
                return format;
            else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
                return format;
        }

        throw std::runtime_error("did not find a supported format!");
    }

    VkFormat findDepthFormat()
    {
        return findSupportedFormat(
                { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
                VK_IMAGE_TILING_OPTIMAL,
                VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
        );
    }

    bool hasStencilComponent(VkFormat format)
    {
        return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
    }

    void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory, size_t mips = 1, bool cube = false)
    {
        VkImageCreateInfo imageInfo {};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = width;
        imageInfo.extent.height = cube ? width : height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = mips;
        imageInfo.arrayLayers = cube ? 6 : 1;
        imageInfo.format = format;
        imageInfo.tiling = tiling;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = usage;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (cube)
            imageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

        auto result = vkCreateImage(device, &imageInfo, null, &image);
        if (result != VK_SUCCESS)
        {
            printf("failed: %d\n", result);
            throw std::runtime_error("image creation failed!");
        }

        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(device, image, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

        result = vkAllocateMemory(device, &allocInfo, null, &imageMemory);
        if (result != VK_SUCCESS)
        {
            printf("failed: %d\n", result);
            throw std::runtime_error("memory allocation failed!");
        }

        vkBindImageMemory(device, image, imageMemory, 0);
    }

    VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, size_t mips = 1, bool cube = false)
    {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = image;

        if (cube)
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
        else
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;

        viewInfo.format = format;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = mips;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = cube ? 6 : 1;
        viewInfo.subresourceRange.aspectMask = aspectFlags;

        VkImageView imageView;
        auto result = vkCreateImageView(device, &viewInfo, null, &imageView);
        if (result != VK_SUCCESS)
        {
            printf("failed: %d\n", result);
            throw std::runtime_error("image view creation failed!");
        }

        return imageView;
    }

    VkCommandBuffer beginSingleTimeCommands()
    {
        VkCommandBufferAllocateInfo allocInfo {};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = commandPool;
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer;
        vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(commandBuffer, &beginInfo);

        return commandBuffer;
    }

    void endSingleTimeCommands(VkCommandBuffer commandBuffer)
    {
        vkEndCommandBuffer(commandBuffer);

        VkSubmitInfo submitInfo {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(graphicsQueue);

        vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
    }

    void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, size_t mips = 1, bool cube = false)
    {
        VkCommandBuffer commandBuffer = beginSingleTimeCommands();

        VkImageMemoryBarrier barrier {};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image;

        if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
        {
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

            if (hasStencilComponent(format))
                barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }
        else
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = mips;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = cube ? 6 : 1;

        VkPipelineStageFlags sourceStage;
        VkPipelineStageFlags destinationStage;

        if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
        {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        }
        else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
        {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }
        else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
        {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        }
        else
            throw std::invalid_argument("unsupported layout transition!");

        vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, null,0, null,1, &barrier);

        endSingleTimeCommands(commandBuffer);
    }

    void createDepthResources()
    {
        VkFormat depthFormat = findDepthFormat();
        createImage(swapChainExtent.width, swapChainExtent.height, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage, depthImageMemory);
        depthImageView = createImageView(depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
        transitionImageLayout(depthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    }

    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags props, VkBuffer& buffer, VkDeviceMemory& bufMem)
    {
        VkBufferCreateInfo bufferInfo {};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        auto result = vkCreateBuffer(device, &bufferInfo, null, &buffer);
        if (result != VK_SUCCESS)
        {
            printf("failed: %d\n", result);
            throw std::runtime_error("vertex buffer creation failed!");
        }

        VkMemoryRequirements memReq;
        vkGetBufferMemoryRequirements(device, buffer, &memReq);

        VkMemoryAllocateInfo allocInfo {};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memReq.size;
        allocInfo.memoryTypeIndex = findMemoryType(memReq.memoryTypeBits, props);

        result = vkAllocateMemory(device, &allocInfo, null, &bufMem);
        if (result != VK_SUCCESS)
        {
            printf("failed: %d\n", result);
            abort();
            throw std::runtime_error("buffer memory allocation failed!");
        }

        vkBindBufferMemory(device, buffer, bufMem, 0);
    }

    void copyBuffer(VkBuffer src, VkBuffer dest, VkDeviceSize size)
    {
        VkCommandBufferAllocateInfo allocInfo {};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = commandPool;
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer;
        vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

        VkCommandBufferBeginInfo beginInfo {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(commandBuffer, &beginInfo);

        VkBufferCopy copyRegion {};
        copyRegion.size = size;
        vkCmdCopyBuffer(commandBuffer, src, dest, 1, &copyRegion);

        vkEndCommandBuffer(commandBuffer);

        VkSubmitInfo submitInfo {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(graphicsQueue);

        vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
    }

    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, int mip, bool cube = false)
    {
        VkCommandBuffer commandBuffer = beginSingleTimeCommands();

        std::vector<VkBufferImageCopy> regions {};

        for (int i = 0; i < (cube ? 6 : 1); i++)
        {
            VkBufferImageCopy region {};
            region.bufferOffset = width * width * 4 * i * sizeof(float);
            region.bufferRowLength = width;
            region.bufferImageHeight = cube ? width : height;
            region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            region.imageSubresource.mipLevel = mip;
            region.imageSubresource.baseArrayLayer = i;
            region.imageSubresource.layerCount = 1;
            region.imageOffset = {0, 0, 0};
            region.imageExtent = {width, cube ? width : height, 1};
            regions.emplace_back(region);
        }

        vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, regions.size(), regions.data());

        endSingleTimeCommands(commandBuffer);
    }

    u32 findMemoryType(u32 typeFilter, VkMemoryPropertyFlags props)
    {
        VkPhysicalDeviceMemoryProperties memProps;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProps);

        for (u32 i = 0; i < memProps.memoryTypeCount; i++)
        {
            if ((typeFilter & (1 << i)) && (memProps.memoryTypes[i].propertyFlags & props) == props)
                return i;
        }

        throw std::runtime_error("finding suitable memory type failed!");
    }

    void createUniformBuffers()
    {
        VkDeviceSize size = sizeof(UniformBufferObject);

        globalUniformBuffers.resize(max_frames_in_flight);
        globalUniformBuffersMemory.resize(max_frames_in_flight);
        globalUniformBuffersMapped.resize(max_frames_in_flight);

        for (size_t i = 0; i < max_frames_in_flight; i++)
        {
            createBuffer(size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, globalUniformBuffers[i], globalUniformBuffersMemory[i]);
            vkMapMemory(device, globalUniformBuffersMemory[i], 0, size, 0, &globalUniformBuffersMapped[i]);
        }
    }

    void createDescriptorPool()
    {
        std::vector<VkDescriptorPoolSize> poolSizes (4);

        poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSizes[0].descriptorCount = static_cast<uint32_t>(max_frames_in_flight);
        poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSizes[1].descriptorCount = static_cast<uint32_t>(max_frames_in_flight * 2);
        poolSizes[2].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        poolSizes[2].descriptorCount = static_cast<uint32_t>(max_frames_in_flight);
        poolSizes[3].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSizes[3].descriptorCount = std::max(1, scene->shadowCount) * max_frames_in_flight;

        VkDescriptorPoolCreateInfo poolInfo {};
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.maxSets = static_cast<uint32_t>(max_frames_in_flight);

        auto result = vkCreateDescriptorPool(device, &poolInfo, null, &globalDescriptorPool);
        if (result != VK_SUCCESS)
        {
            printf("failed: %d\n", result);
            throw std::runtime_error("descriptor pool creation failed!");
        }
    }

    void createDescriptorSetLayout()
    {
        std::vector<VkDescriptorSetLayoutBinding> bindings(4);

        VkDescriptorSetLayoutBinding uboLayoutBinding {};
        uboLayoutBinding.binding = 0;
        uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboLayoutBinding.descriptorCount = 1;
        uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        bindings[0] = uboLayoutBinding;

        VkDescriptorSetLayoutBinding samplerLayoutBinding {};
        samplerLayoutBinding.binding = 1;
        samplerLayoutBinding.descriptorCount = 2;
        samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        samplerLayoutBinding.pImmutableSamplers = null;
        samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        bindings[1] = samplerLayoutBinding;

        VkDescriptorSetLayoutBinding ssboBinding {};
        ssboBinding.binding = 3;
        ssboBinding.descriptorCount = 1;
        ssboBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        ssboBinding.pImmutableSamplers = null;
        ssboBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        bindings[2] = ssboBinding;

        VkDescriptorSetLayoutBinding shadowMapsBinding {};
        shadowMapsBinding.binding = 4;
        shadowMapsBinding.descriptorCount = std::max(1, scene->shadowCount);
        shadowMapsBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        shadowMapsBinding.pImmutableSamplers = null;
        shadowMapsBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        bindings[3] = shadowMapsBinding;

        VkDescriptorSetLayoutCreateInfo layoutInfo {};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = bindings.size();
        layoutInfo.pBindings = bindings.data();

        auto result = vkCreateDescriptorSetLayout(device, &layoutInfo, null, &globalDescriptorSetLayout);
        if (result != VK_SUCCESS)
        {
            printf("failed: %d\n", result);
            throw std::runtime_error("descriptor set layout failed!");
        }
    }

    void createDescriptorSets()
    {
        std::vector<VkDescriptorSetLayout> layouts(max_frames_in_flight, globalDescriptorSetLayout);
        VkDescriptorSetAllocateInfo allocInfo {};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = globalDescriptorPool;
        allocInfo.descriptorSetCount = static_cast<u32>(max_frames_in_flight);
        allocInfo.pSetLayouts = layouts.data();

        globalDescriptorSets.resize(max_frames_in_flight);
        auto result = vkAllocateDescriptorSets(device, &allocInfo, globalDescriptorSets.data());
        if (result != VK_SUCCESS)
        {
            printf("failed: %d\n", result);
            throw std::runtime_error("descriptor sets creation failed!");
        }

        for (size_t i = 0; i < max_frames_in_flight; i++)
        {
            VkDescriptorBufferInfo bufferInfo {};
            bufferInfo.buffer = globalUniformBuffers[i];
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(UniformBufferObject);

            VkDescriptorImageInfo envInfoPBR {};
            envInfoPBR.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            envInfoPBR.imageView = scene->environment->texturePBR->imageView;
            envInfoPBR.sampler = scene->environment->texturePBR->sampler;

            VkDescriptorImageInfo envInfoLambertian {};
            envInfoLambertian.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            envInfoLambertian.imageView = scene->environment->textureLambertian->imageView;
            envInfoLambertian.sampler = scene->environment->textureLambertian->sampler;

            std::array<VkWriteDescriptorSet, 4> descriptorWrites {};
            descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[0].dstSet = globalDescriptorSets[i];
            descriptorWrites[0].dstBinding = 0;
            descriptorWrites[0].dstArrayElement = 0;
            descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrites[0].descriptorCount = 1;
            descriptorWrites[0].pBufferInfo = &bufferInfo;

            std::array<VkDescriptorImageInfo, 2> imageInfos {envInfoPBR, envInfoLambertian};
            descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[1].dstSet = globalDescriptorSets[i];
            descriptorWrites[1].dstBinding = 1;
            descriptorWrites[1].dstArrayElement = 0;
            descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptorWrites[1].descriptorCount = imageInfos.size();
            descriptorWrites[1].pImageInfo = imageInfos.data();

            VkDescriptorBufferInfo lights {};
            lights.buffer = shaderStorageBuffers[i];
            lights.offset = 0;
            lights.range = sizeof(ShaderLight) * scene->shaderLights.size() + 16;

            descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[2].dstSet = globalDescriptorSets[i];
            descriptorWrites[2].dstBinding = 3;
            descriptorWrites[2].dstArrayElement = 0;
            descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            descriptorWrites[2].descriptorCount = 1;
            descriptorWrites[2].pBufferInfo = &lights;

            std::vector<VkDescriptorImageInfo> shadowInfos (scene->shadowCount);
            for (int s = 0; s < scene->shadowMaps.size(); s++)
            {
                VkDescriptorImageInfo descriptorImageInfo {};
                descriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
                descriptorImageInfo.imageView = scene->shadowMaps[s].imageViews[i];
                descriptorImageInfo.sampler = scene->shadowMaps[s].samplers[i];
                shadowInfos[s] = descriptorImageInfo;
            }

            descriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[3].dstSet = globalDescriptorSets[i];
            descriptorWrites[3].dstBinding = 4;
            descriptorWrites[3].dstArrayElement = 0;
            descriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptorWrites[3].descriptorCount = shadowInfos.size();
            descriptorWrites[3].pImageInfo = shadowInfos.data();

            int count = descriptorWrites.size();
            if (scene->shadowMaps.empty())
                count--;

            vkUpdateDescriptorSets(device, count, descriptorWrites.data(), 0, null);
        }
    }

    void createCommandBuffers()
    {
        commandBuffers.resize(max_frames_in_flight);

        VkCommandBufferAllocateInfo allocInfo {};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = (u32) commandBuffers.size();

        auto result = vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data());
        if (result != VK_SUCCESS)
        {
            printf("failed: %d\n", result);
            throw std::runtime_error("command buffer creation failed!");
        }
    }

    void recordCommandBufferShadowPasses(VkCommandBuffer &commandBuffer)
    {
        scene->drawingShadow = true;
        VkClearValue clearDepth {};
        clearDepth.depthStencil = {1.0, 0};

        int in = 0;
        for (ShadowMap &sm: scene->shadowMaps)
        {
            scene->currentShadowMapIndex = in;
            in++;
            scene->currentShadowMap = &sm;

            assert(sm.resolution > 0);
            VkRenderPassBeginInfo renderPassInfo {};
            renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderPassInfo.renderPass = shadowRenderPass;
            renderPassInfo.framebuffer = sm.framebuffers[currentFrame];
            renderPassInfo.renderArea.offset = {0, 0};
            renderPassInfo.renderArea.extent.width = sm.resolution;
            renderPassInfo.renderArea.extent.height = sm.resolution;
            renderPassInfo.clearValueCount = 1;
            renderPassInfo.pClearValues = &clearDepth;

            vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

            VkViewport viewport {};
            viewport.x = 0.0f;
            viewport.y = 0.0f;
            viewport.width = static_cast<float>(sm.resolution);
            viewport.height = static_cast<float>(sm.resolution);
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;
            vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

            VkRect2D scissor {};
            scissor.offset = {0, 0};
            scissor.extent = renderPassInfo.renderArea.extent;
            vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

            // Constants recommended from https://github.com/SaschaWillems/Vulkan/blob/master/examples/shadowmapping/shadowmapping.cpp
            vkCmdSetDepthBias(commandBuffer, 1.25f, 0, 1.75f);

            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, materialTypeSimple.getPipeline(scene)->pipeline);
            currentMaterialType = &materialTypeSimple;

            draw(commandBuffer);

            vkCmdEndRenderPass(commandBuffer);
        }

        scene->currentShadowMap = null;
        scene->drawingShadow = false;
    }

    float lastHeadlessTime = 0;
    void recordCommandBuffer(VkCommandBuffer commandBuffer, u32 imageIndex)
    {
        VkCommandBufferBeginInfo beginInfo {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        auto result = vkBeginCommandBuffer(commandBuffer, &beginInfo);
        if (result != VK_SUCCESS)
        {
            printf("failed: %d\n", result);
            throw std::runtime_error("beginning to record command buffer failed!");
        }

        recordCommandBufferShadowPasses(commandBuffer);

        VkRenderPassBeginInfo renderPassInfo {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = renderPass;
        renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = swapChainExtent;

        std::array<VkClearValue, 2> clearValues {};
        clearValues[0].color = {scene->debugCameraMode ? 0.2f : 0.0f, 0.0f, 0.0f, 1.0f};
        clearValues[1].depthStencil = {1.0f, 0};
        renderPassInfo.clearValueCount = clearValues.size();
        renderPassInfo.pClearValues = clearValues.data();

        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, materialTypeSimple.getPipeline(scene)->pipeline);
        currentMaterialType = &materialTypeSimple;

        VkViewport viewport {};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(swapChainExtent.width);
        viewport.height = static_cast<float>(swapChainExtent.height);

        if (scene != null && scene->currentCamera != null)
        {
            float ar = scene->currentCamera->aspectRatio;
            if (viewport.width > ar * viewport.height)
            {
                viewport.width = ar * viewport.height;
                viewport.x = ((float) swapChainExtent.width - viewport.width) / 2.0f;
            }
            else
            {
                viewport.height = viewport.width / ar;
                viewport.y = ((float) swapChainExtent.height - viewport.height) / 2.0f;
            }
        }

        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

        VkRect2D scissor {};
        scissor.offset = {0, 0};
        scissor.extent = swapChainExtent;
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        static int i = 0;
        i++;

        draw(commandBuffer);

        vkCmdEndRenderPass(commandBuffer);

        auto result1 = vkEndCommandBuffer(commandBuffer);
        if (result1 != VK_SUCCESS)
        {
            printf("failed: %d\n", result1);
            throw std::runtime_error("ending to record command buffer failed!");
        }
    }

    void draw(VkCommandBuffer &commandBuffer)
    {
        mat4 transform = mat4::I();

        scene->detachedCamera->updateTransform();

        if (scene->debugCameraMode)
            scene->debugCamera->updateTransform();

        if (scene != null)
        {
            for (auto r: scene->roots)
            {
                r->precomputeTransforms(mat4::I(), mat4::I());
            }

            scene->shaderLights.clear();
            scene->shadowCount = 0;
            for (auto r: scene->roots)
            {
                r->precomputeLights(scene->shaderLights, scene->shadowMaps, scene->initialized, scene->shadowCount, mat4::I(), mat4::I());
            }

            updateUniformBuffer();
            updateLightSSBOs();

            Camera* cam = scene->currentCamera;
            if (scene->drawingShadow)
            {
                ShaderLight sl = scene->shaderLights[scene->currentShadowMapIndex];
                cam = new Camera("shadow", 1, sl.fov, sl.radius, sl.limit);
                cam->cameraUserOffsetTransform = sl.worldToLight;
            }

            for (auto r: scene->roots)
            {
                r->draw(commandBuffer, transform, cam, enableCulling, currentTime);
            }

            if (scene->drawingShadow)
                delete cam;
        }
    }

    void createSyncObjects()
    {
        imageAvailableSemaphores.resize(max_frames_in_flight);
        renderFinishedSemaphores.resize(max_frames_in_flight);
        inFlightFences.resize(max_frames_in_flight);

        VkSemaphoreCreateInfo semaphoreInfo {};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo {};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (size_t i = 0; i < max_frames_in_flight; i++)
        {
            if (vkCreateSemaphore(device, &semaphoreInfo, null, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
                vkCreateSemaphore(device, &semaphoreInfo, null, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
                vkCreateFence(device, &fenceInfo, null, &inFlightFences[i]) != VK_SUCCESS)
            {
                throw std::runtime_error("semaphore and fence creation failed!");
            }
        }
    }

    void load()
    {
        std::string s = std::string(readFileWithCache(this->sceneName)->data());
        jarray* o = jparse_array(s);
        this->scene = parseScene(o);

        // Add default material
        materialTypeSimple.instances++;

        for (MaterialType* t: materialTypes)
        {
            t->createDescriptorPool(device);
        }

        for (auto m: scene->materials)
        {
            m.second->createDescriptorSets(device);
        }

        defaultMaterial.createDescriptorSets(device);

        for (auto r: scene->roots)
        {
            r->precomputeLights(scene->shaderLights, scene->shadowMaps, scene->initialized, scene->shadowCount, mat4::I(), mat4::I());
        }
        scene->initialized = true;
    }

    void loop()
    {
        while (headless || !glfwWindowShouldClose(window))
        {
            if (!headless)
                glfwPollEvents();

            handleControls();
            drawFrame();

            if (headless && headlessIndex >= headlessEvents.size())
                break;
        }

        vkDeviceWaitIdle(device);
    }

    void handleControls()
    {
        if (headless)
            return;

        Camera* c = scene->currentCamera;
        if (scene->debugCameraMode)
            c = scene->debugCamera;

        vec3 x = (c->rotationInverse * vec4(1, 0, 0, 1)).xyz();
        vec3 y = (c->rotationInverse * vec4(0, 1, 0, 1)).xyz();
        vec3 z = (c->rotationInverse * vec4(0, 0, 1, 1)).xyz();

        if (keyDown(GLFW_KEY_W))
            c->position = c->position - z * c->speed * frameTime;

        if (keyDown(GLFW_KEY_S))
            c->position = c->position + z * c->speed * frameTime;

        if (keyDown(GLFW_KEY_A))
            c->position = c->position - x * c->speed * frameTime;

        if (keyDown(GLFW_KEY_D))
            c->position = c->position + x * c->speed * frameTime;

        if (keyDown(GLFW_KEY_SPACE))
            c->position = c->position + y * c->speed * frameTime;

        if (keyDown(GLFW_KEY_LEFT_SHIFT))
            c->position = c->position - y * c->speed * frameTime;

        if (keyDown(GLFW_KEY_ENTER))
        {
            if (scene->debugCameraMode)
            {
                c->position = scene->currentCamera->position;
                c->rotation = scene->currentCamera->rotation;
                c->rotationInverse = scene->currentCamera->rotationInverse;
                c->zoom = scene->currentCamera->zoom;
            }
            else
            {
                c->position = vec3(0, 0, 0);
                c->rotation = mat4::I();
                c->rotationInverse = mat4::I();
                c->zoom = 0;
            }
        }

        if (mouseGrabbed)
        {
            double x;
            double y;
            glfwGetCursorPos(window, &x, &y);

            float scale = tan((c->verticalFOV + c->zoom) / 2);

            if (mouseGrabbed > 5)
            {
                if (keyDown(GLFW_KEY_LEFT_CONTROL) || rightMouseDown)
                {
                    c->rotation = mat4::rotateAxis(vec3(0, 0, 1), (float) (((int) x - (int) mouseLastGrabX) / 500.0)) * c->rotation;
                    c->rotationInverse = c->rotationInverse * mat4::rotateAxis(vec3(0, 0, 1), (float) (-((int) x - (int) mouseLastGrabX) / 500.0));
                }
                else
                {
                    c->rotation =
                            mat4::rotateAxis(vec3(0, 1, 0),
                                             scale * (float) (((int) x - (int) mouseLastGrabX) / 500.0)) *
                            mat4::rotateAxis(vec3(1, 0, 0),
                                             scale * (float) (((int) y - (int) mouseLastGrabY) / 500.0)) *
                            c->rotation;
                    c->rotationInverse =
                            c->rotationInverse * mat4::rotateAxis(vec3(1, 0, 0),
                                             scale * (float) (-((int) y - (int) mouseLastGrabY) / 500.0)) *
                            mat4::rotateAxis(vec3(0, 1, 0),
                                             scale * (float) (-((int) x - (int) mouseLastGrabX) / 500.0));
                }
            }

            mouseLastGrabX = x;
            mouseLastGrabY = y;

            mouseGrabbed++;
        }

        if (keyDown(GLFW_KEY_N))
            timeRate -= frameTime;

        if (keyDown(GLFW_KEY_M))
            timeRate += frameTime;

        if (keyDown(GLFW_KEY_COMMA))
            timeRate = 1;

        if (keyDown(GLFW_KEY_SEMICOLON))
            timeRate = 0;

        if (keyDown(GLFW_KEY_PERIOD))
        {
            std::erase(keysPressed, GLFW_KEY_PERIOD);
            currentTime = 0;
        }

        if (keyDown(GLFW_KEY_0))
        {
            std::erase(keysPressed, GLFW_KEY_0);
            scene->debugCameraMode = !scene->debugCameraMode;
            scene->debugCamera->rotation = scene->currentCamera->rotation;
            scene->debugCamera->position = scene->currentCamera->position;
            scene->debugCamera->speed = scene->currentCamera->speed;
            scene->debugCamera->aspectRatio = scene->currentCamera->aspectRatio;
            scene->debugCamera->verticalFOV = scene->currentCamera->verticalFOV;
            scene->debugCamera->nearPlane = scene->currentCamera->nearPlane;
            scene->debugCamera->farPlane = scene->currentCamera->farPlane;
            scene->debugCamera->worldToCameraBaseTransform = scene->currentCamera->worldToCameraBaseTransform;
            scene->debugCamera->zoom = scene->currentCamera->zoom;
            scene->debugCamera->computeTransform();
            scene->debugCamera->updateTransform();

            if (scene->debugCameraMode)
                printf("Entered debug camera\n");
            else
                printf("Exited debug camera\n");
        }

        if (keyDown(GLFW_KEY_9) && scene->debugCameraMode)
        {
            std::erase(keysPressed, GLFW_KEY_9);
            scene->debugCameraMode = false;
            scene->currentCamera->rotation = scene->debugCamera->rotation;
            scene->currentCamera->position = scene->debugCamera->position;
            scene->currentCamera->speed = scene->debugCamera->speed;
            scene->currentCamera->aspectRatio = scene->debugCamera->aspectRatio;
            scene->currentCamera->verticalFOV = scene->debugCamera->verticalFOV;
            scene->currentCamera->nearPlane = scene->debugCamera->nearPlane;
            scene->currentCamera->farPlane = scene->debugCamera->farPlane;
            scene->currentCamera->worldToCameraBaseTransform = scene->debugCamera->worldToCameraBaseTransform;
            scene->currentCamera->zoom = scene->debugCamera->zoom;
            scene->currentCamera->computeTransform();
            scene->currentCamera->updateTransform();

            printf("Exited debug camera\n");
        }

        if (keyDown(GLFW_KEY_EQUAL) && !scene->debugCameraMode)
        {
            std::erase(keysPressed, GLFW_KEY_EQUAL);
            scene->currentCameraIndex = (scene->currentCameraIndex + 1) % scene->cameraCount;
            scene->currentCamera = scene->camerasEnumerated[scene->currentCameraIndex];

            printf("Switched to camera \"%s\"\n", scene->currentCamera->name.c_str());
        }

        if (keyDown(GLFW_KEY_MINUS) && !scene->debugCameraMode)
        {
            std::erase(keysPressed, GLFW_KEY_MINUS);
            scene->currentCameraIndex = (scene->currentCameraIndex - 1 + scene->cameraCount) % scene->cameraCount;
            scene->currentCamera = scene->camerasEnumerated[scene->currentCameraIndex];

            printf("Switched to camera \"%s\"\n", scene->currentCamera->name.c_str());
        }
    }

    void drawFrame()
    {
        meshesDrawn = 0;
        meshesCulled = 0;

        if (headless)
        {
            float t = (float) headlessEvents[headlessIndex].time / 1000000.0f;
            currentTime += (t - lastHeadlessTime) * timeRate;
            lastHeadlessTime = t;

            if (headlessEvents[headlessIndex].type == 1)
            {
                currentTime = headlessEvents[headlessIndex].animTime;
                timeRate = headlessEvents[headlessIndex].animRate;
            }
            else if (headlessEvents[headlessIndex].type == 3)
            {
                printf("MARK %s\n", headlessEvents[headlessIndex].text.c_str());
            }

            headlessIndex++;
            if (headlessEvents[headlessIndex - 1].type != 0)
            {
                return;
            }
        }

        auto time = std::chrono::high_resolution_clock::now();

        vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

        u32 imageIndex;

        if (!headless)
        {
            auto result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame],
                                                VK_NULL_HANDLE, &imageIndex);

            if (result == VK_ERROR_OUT_OF_DATE_KHR)
            {
                recreateSwapChain();
                return;
            }
            else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
            {
                printf("failed: %d\n", result);
                throw std::runtime_error("acquiring next image failed!");
            }

            if (!timeRecorded)
            {
                lastRecordedTime = std::chrono::high_resolution_clock::now();
                timeRecorded = true;
                currentTime = 0.0f;
            }
            else
            {
                auto now = std::chrono::high_resolution_clock::now();
                currentTime += timeRate * std::chrono::duration<float, std::chrono::seconds::period>(
                        now - lastRecordedTime).count();
                lastRecordedTime = now;
            }
        }
        else
        {
            imageIndex = currentFrame;
        }

        vkResetFences(device, 1, &inFlightFences[currentFrame]);

        vkResetCommandBuffer(commandBuffers[currentFrame], 0);
        recordCommandBuffer(commandBuffers[currentFrame], imageIndex);

        VkSubmitInfo submitInfo {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        std::vector<VkSemaphore> waitSemaphores = {};

        if (!headless)
            waitSemaphores = { imageAvailableSemaphores[currentFrame] };

        VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        submitInfo.waitSemaphoreCount = waitSemaphores.size();
        submitInfo.pWaitSemaphores = waitSemaphores.data();
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffers[currentFrame];

        VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame] };
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        if (headless)
            submitInfo.signalSemaphoreCount = 0;

        auto result1 = vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]);
        if (result1 != VK_SUCCESS)
        {
            printf("failed: %d\n", result1);
            throw std::runtime_error("submitting draw command buffer failed!");
        }

        if (!headless)
        {
            VkPresentInfoKHR presentInfo {};
            presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
            presentInfo.waitSemaphoreCount = 1;
            presentInfo.pWaitSemaphores = signalSemaphores;

            VkSwapchainKHR swapChains[] = {swapChain};
            presentInfo.swapchainCount = 1;
            presentInfo.pSwapchains = swapChains;
            presentInfo.pImageIndices = &imageIndex;

            auto result = vkQueuePresentKHR(presentQueue, &presentInfo);
            if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized)
            {
                framebufferResized = false;
                recreateSwapChain();
            }
            else if (result != VK_SUCCESS)
            {
                printf("failed: %d\n", result);
                throw std::runtime_error("failed to present swap chain image!");
            }
        }

        auto end = std::chrono::high_resolution_clock::now();
        currentFrame = (currentFrame + 1) % max_frames_in_flight;
        frameTime = (end - time).count() / 1000000000.0f;

        if (logStats)
            printf("Drew %lu meshes in %lld (%lu culled)\n", meshesDrawn, (end - time).count(), meshesCulled);

//        printf("%lu, %lld\n", meshesDrawn, (end - time).count());
    };

    void updateUniformBuffer()
    {
        UniformBufferObject ubo {};

        if (scene->debugCameraMode)
        {
            ubo.proj = scene->debugCamera->perspectiveTransform;
            ubo.camera = scene->debugCamera->cameraUserOffsetTransform;
            ubo.cameraPos = scene->debugCamera->cameraPosTransform * vec4(0, 0, 0, 1);
        }
        else
        {
            ubo.proj = scene->currentCamera->perspectiveTransform;
            ubo.camera = scene->currentCamera->cameraUserOffsetTransform;
            ubo.cameraPos = scene->currentCamera->cameraPosTransform * vec4(0, 0, 0, 1);
        }

        ubo.environment = scene->environment->worldToEnvironmentTransform;
        ubo.hdr = hdr;
        memcpy(globalUniformBuffersMapped[currentFrame], &ubo, sizeof(ubo));
    }

    void updateLightSSBOs()
    {
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        u32 size = scene->shaderLights.size() * sizeof(ShaderLight) + 16;
        createBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer,
                     stagingBufferMemory);

        std::vector<ShaderLight> lights {};
        for (ShaderLight sl: scene->shaderLights)
        {
            if (sl.shadowRes > 0)
                lights.emplace_back(sl);
        }

        for (ShaderLight sl: scene->shaderLights)
        {
            if (sl.shadowRes <= 0)
                lights.emplace_back(sl);
        }

        scene->shaderLights.clear();
        for (ShaderLight sl: lights)
        {
            scene->shaderLights.emplace_back(sl);
        }

        void *data;
        vkMapMemory(device, stagingBufferMemory, 0, size, 0, &data);
        memcpy((char*) data + 16, lights.data(), size - 16);
        u32 s = lights.size();
        memcpy(data, &s, 4);
        vkUnmapMemory(device, stagingBufferMemory);

        copyBuffer(stagingBuffer, shaderStorageBuffers[currentFrame], size);

        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingBufferMemory, nullptr);
    }

    void updatePushConstants(mat4 m, u32 currentImage)
    {
        PushConstant pc {};
        pc.modelView = m;

        if (scene->drawingShadow)
            pc.shadowMap = scene->currentShadowMapIndex;
        else
            pc.shadowMap = -1;

        // adapted from https://vkguide.dev/docs/chapter-3/push_constants/
        vkCmdPushConstants(commandBuffers[currentFrame], currentMaterialType->getPipeline(scene)->layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(pc), &pc);
    }

    void cleanup()
    {
        for (ShadowMap &s: this->scene->shadowMaps)
        {
            for (int i = 0; i < s.framebuffers.size(); i++)
            {
                vkDestroySampler(device, s.samplers[i], nullptr);
                vkDestroyImageView(device, s.imageViews[i], nullptr);

                vkDestroyImage(device, s.images[i], nullptr);
                vkFreeMemory(device, s.imageMemories[i], nullptr);

                vkDestroyFramebuffer(device, s.framebuffers[i], null);
            }
        }

        if (this->scene != null)
            delete this->scene;

        cleanupSwapChain();

        for (size_t i = 0; i < max_frames_in_flight; i++)
        {
            vkDestroyBuffer(device, globalUniformBuffers[i], null);
            vkFreeMemory(device, globalUniformBuffersMemory[i], null);
            vkDestroyBuffer(device, shaderStorageBuffers[i], null);
            vkFreeMemory(device, shaderStorageBuffersMemory[i], null);
        }

        for (MaterialType* m: materialTypes)
        {
            vkDestroyDescriptorPool(device, m->descriptorPool, null);
            vkDestroyDescriptorSetLayout(device, m->descriptorSetLayout, null);
        }

        vkDestroyDescriptorPool(device, globalDescriptorPool, null);
        vkDestroyDescriptorSetLayout(device, globalDescriptorSetLayout, null);

        destroyGraphicsPipelines();

        vkDestroyRenderPass(device, renderPass, null);
        vkDestroyRenderPass(device, shadowRenderPass, null);

        for (size_t i = 0; i < max_frames_in_flight; i++)
        {
            vkDestroySemaphore(device, imageAvailableSemaphores[i], null);
            vkDestroySemaphore(device, renderFinishedSemaphores[i], null);
            vkDestroyFence(device, inFlightFences[i], null);
        }

        vkDestroyCommandPool(device, commandPool, null);
        vkDestroyDevice(device, null);

        if (enable_validation_layers)
            destroyDebugUtilsMessengerEXT(instance, debugMessenger, null);

        if (!headless)
            vkDestroySurfaceKHR(instance, surface, null);

        vkDestroyInstance(instance, null);

        if (!headless)
            glfwDestroyWindow(window);

        glfwTerminate();
    }

    void cleanupSwapChain()
    {
        vkDestroyImageView(device, depthImageView, null);
        vkDestroyImage(device, depthImage, null);
        vkFreeMemory(device, depthImageMemory, null);
        
        for (auto framebuffer: swapChainFramebuffers)
        {
            vkDestroyFramebuffer(device, framebuffer, null);
        }

        for (auto imageView: swapChainImageViews)
        {
            vkDestroyImageView(device, imageView, null);
        }

        if (!headless)
            vkDestroySwapchainKHR(device, swapChain, null);
    }

    void recreateSwapChain()
    {
        int w = 0;
        int h = 0;

        if (!headless)
        {
            glfwGetFramebufferSize(window, &w, &h);
            while (w == 0 && h == 0)
            {
                glfwGetFramebufferSize(window, &w, &h);
                glfwWaitEvents();
            }
        }

        vkDeviceWaitIdle(device);

        cleanupSwapChain();
        createSwapChain();
        createImageViews();
        createDepthResources();
        createFramebuffers();
    }

};

int main(int argc, char** args)
{
    char* scene = null;
    char* camera = null;
    char* physicalDevice = null;

    int width = 1280;
    int height = 720;
    bool culling = true;
    bool stats = false;
    bool headless = false;
    bool hdr = false;
    std::string events;

    for (int i = 0; i < argc; i++)
    {
        if (strncmp(args[i], "--scene", 7) == 0 && i + 1 < argc)
            scene = args[i + 1];

        if (strncmp(args[i], "--camera", 8) == 0 && i + 1 < argc)
            camera = args[i + 1];

        if (strncmp(args[i], "--physical-device", 17) == 0 && i + 1 < argc)
            physicalDevice = args[i + 1];

        if (strncmp(args[i], "--drawing-size", 14) == 0 && i + 2 < argc)
        {
            width = std::stoi(args[i + 1]);
            height = std::stoi(args[i + 2]);
        }

        if (strncmp(args[i], "--culling", 9) == 0 && i + 1 < argc)
        {
            if (strncmp(args[i + 1], "none", 4) == 0)
                culling = false;
            else if (strncmp(args[i + 1], "frustum", 7) == 0)
                culling = true;
            else
            {
                printf("Invalid culling mode \"%s\" - please select \"none\" or \"frustum\"!\n", args[i + 1]);
                abort();
            }
        }

        if (strncmp(args[i], "--headless", 10) == 0 && i + 1 < argc)
        {
            headless = true;
            events = args[i + 1];
        }

        if (strncmp(args[i], "--log-stats", 11) == 0)
            stats = true;

        if (strncmp(args[i], "--hdr", 5) == 0)
            hdr = true;
    }

    if (scene == null)
    {
        printf("Please specify scene with --scene <scene>!\n");
        abort();
    }

    Renderer app;
    app.sceneName = scene;
    app.displayWidth = width;
    app.displayHeight = height;
    app.enableCulling = culling;
    app.requestedCameraName = camera;
    app.requestedPhysicalDeviceName = physicalDevice;
    app.logStats = stats;
    app.headless = headless;
    app.hdr = hdr;
    if (app.headless)
        app.readHeadlessEvents(events);

    try
    {
        app.run();
    }
    catch (const std::exception& how)
    {
        printf("%s\n", how.what());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

// I hate the c++ linker sometimes >:(
std::vector<Renderer::MaterialType*> Renderer::materialTypes;
Renderer::MaterialType Renderer::materialTypeSimple = MaterialType(0);
Renderer::MaterialType Renderer::materialTypePBR = MaterialType(4);
Renderer::MaterialType Renderer::materialTypeLambertian = MaterialType(2);
Renderer::MaterialType Renderer::materialTypeMirror = MaterialType(1);
Renderer::MaterialType Renderer::materialTypeEnvironment = MaterialType(1);
