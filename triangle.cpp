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

#define null nullptr
//#define null NULL

typedef uint32_t u32;

static size_t meshesDrawn;

const std::vector<const char*> validationLayers =
        {
            "VK_LAYER_KHRONOS_validation"
        };


const std::vector<const char*> deviceExtensions =
        {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
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

struct Vertex
{
    vec3 pos;
    vec3 normal;
    u8vec4 color;

    static VkVertexInputBindingDescription getBindDesc()
    {
        VkVertexInputBindingDescription desc {};
        desc.binding = 0;
        desc.stride = sizeof(Vertex);
        desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return desc;
    }

    static std::array<VkVertexInputAttributeDescription, 3> getAttributeDesc()
    {
        std::array<VkVertexInputAttributeDescription, 3> descs {};
        descs[0].binding = 0;
        descs[0].location = 0;
        descs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        descs[0].offset = offsetof(Vertex, pos);

        descs[1].binding = 0;
        descs[1].location = 1;
        descs[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        descs[1].offset = offsetof(Vertex, normal);

        descs[2].binding = 0;
        descs[2].location = 2;
        descs[2].format = VK_FORMAT_R8G8B8A8_UNORM;
        descs[2].offset = offsetof(Vertex, color);
        return descs;
    }
};

struct UniformBufferObject
{
    alignas(16) mat4 modelView;
    alignas(16) mat4 proj;
};

// A good portion of this code is adapted from the official Vulkan tutorial: https://docs.vulkan.org/tutorial/latest/
class Renderer
{
public:
    void run()
    {
        initWindow();
        initVulkan();
        load();
        loop();
        cleanup();
    }

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

    struct Mesh
    {
        Renderer* renderer;

        std::string name;
        size_t count;
        VkPrimitiveTopology topology;
        std::map<std::string, Attribute> attributes;

        VkBuffer buffer;
        VkDeviceMemory bufferMemory;

        Mesh(Renderer* t, std::string name, size_t count, const std::string& topology)
        {
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

            VkDeviceSize size = sizeof(Vertex) * count;
            renderer->createBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

            void* data;
            vkMapMemory(renderer->device, stagingBufferMemory, 0, size, 0, &data);
            memcpy(data, (attributes.at("POSITION")).data, size);
            vkUnmapMemory(renderer->device, stagingBufferMemory);

            renderer->createBuffer(size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, buffer, bufferMemory);
            renderer->copyBuffer(stagingBuffer, buffer, size);

            vkDestroyBuffer(renderer->device, stagingBuffer, null);
            vkFreeMemory(renderer->device, stagingBufferMemory, null);
        }

        void draw(VkCommandBuffer commandBuffer, mat4 m)
        {
            VkBuffer vertexBuffers[] = { buffer };
            VkDeviceSize offsets[] = { 0 };
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderer->pipelineLayout,
                                    0, 1, &(renderer->descriptorSets[renderer->currentFrame]), 0, null);

            renderer->updateUniformBuffer(m, renderer->currentFrame);
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
        mat4 transform;
        mat4 toTransform;
        float aspectRatio;
        float verticalFOV;
        float nearPlane;
        float farPlane;

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
        }

        void computeTransform(float fovScale)
        {
            if (farPlane == std::numeric_limits<float>::infinity())
                this->transform = mat4::infinitePerspective(aspectRatio, verticalFOV + fovScale, nearPlane);
            else
                this->transform = mat4::perspective(aspectRatio, verticalFOV + fovScale, nearPlane, farPlane);
        }
    };

    struct Node
    {
        std::string name;
        mat4 transform;
        mat4 invTransform;
        std::vector<Node*> children;
        Camera* camera = null;
        Mesh* mesh = null;

        explicit Node(const std::string &name,
                      vec3 translation = vec3(0.0f, 0.0f, 0.0f),
                      vec4 rotation = vec4(0.0f, 0.0f, 0.0f, 1.0f),
                      vec3 scale = vec3(1.0f, 1.0f, 1.0f))
        {
            this->name = name;
            this->transform = mat4::translation(translation) * mat4::rotate(rotation) * mat4::scale(scale);
            this->invTransform = mat4::scale(vec3(1.0f / scale.x, 1.0f / scale.y, 1.0f / scale.z)) * mat4::rotate(vec4(rotation.xyz(), -rotation.w)) * mat4::translation(translation * -1.0f);
        }

        void draw(VkCommandBuffer b, mat4 m)
        {
            assert(this);
            mat4 m2 = m * transform;
            if (mesh != null)
                mesh->draw(b, m2);

            for (Node* n: children)
            {
                n->draw(b, m2);
            }

            meshesDrawn++;
        }

        void computeCamera(mat4 m)
        {
            mat4 m2 = invTransform * m;
            if (camera != null)
                camera->toTransform = m2;

            for (Node* n: children)
            {
                n->computeCamera(m2);
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

        // todo free this
        Camera* currentCamera = new Camera("default", 1.5, PI / 2, 1);

        Scene(std::string name)
        {
            this->name = name;
        }

        ~Scene()
        {
            for (auto n: nodes)
            {
                delete n.second;
            }

            for (auto n: meshes)
            {
                delete n.second;
            }

            for (auto n: cameras)
            {
                delete n.second;
            }
        }
    };

    Scene* parseScene(jarray* sceneArray)
    {
        Scene* scene;
        std::map<int, Node*> nodes;
        std::map<int, Mesh*> meshes;
        std::map<int, Camera*> cameras;
        Camera* currentCam;

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
                currentCam = camera;
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

                mesh->initialize();
                meshes.insert(std::pair(i, mesh));
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
            }

            i++;
        }

        scene->nodes = nodes;
        scene->cameras = cameras;
        scene->meshes = meshes;
        scene->currentCamera = currentCam;
        return scene;
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

    VkImage depthImage;
    VkDeviceMemory depthImageMemory;
    VkImageView depthImageView;

    VkRenderPass renderPass;

    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;

    //VkPipelineLayout uniformLayout;

    std::vector<VkFramebuffer> swapChainFramebuffers;

    struct VertexBuffer;
//    VertexBuffer* vertexBuffer;
//    VertexBuffer* vertexBuffer2;

    std::vector<VkBuffer> uniformBuffers;
    std::vector<VkDeviceMemory> uniformBuffersMemory;
    std::vector<void*> uniformBuffersMapped;

    VkDescriptorSetLayout descriptorSetLayout;
    std::vector<VkDescriptorSet> descriptorSets;
    VkDescriptorPool descriptorPool;

    VkCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;

    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;

    std::vector<int> keysPressed;
    float cameraSpeedMod = 10.0;
    float zoom = 0.0;

    int mouseGrabbed = 0;
    bool rightMouseDown = false;

    double mouseGrabX;
    double mouseGrabY;
    double mouseLastGrabX;
    double mouseLastGrabY;

    u32 currentFrame = 0;
    bool framebufferResized = false;

    Scene* currentScene = null;

    float frameTime = 0;
    vec3 cameraPos = vec3(0, 0, 0);
    mat4 cameraRot = mat4::I();
    mat4 cameraRotInv = mat4::I();
    float cameraSpeed = 1.0f;

    void initWindow()
    {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
        window = glfwCreateWindow(1960, 1080, "I love Vulkan!!!!!", null, null);
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

        if (app->keyDown(GLFW_KEY_LEFT_CONTROL))
            app->cameraSpeedMod += (float) y;
        else
            app->zoom -= (float) y / 500.0f;
    }

    bool keyDown(int key)
    {
        return (std::find(keysPressed.begin(), keysPressed.end(), key) != keysPressed.end());
    }

    void initVulkan()
    {
        createInstance();
        setupDebugMessenger();
        createSurface();
        pickPhysicalDevice();
        createLogicalDevice();
        createSwapChain();
        createImageViews();
        createRenderPass();
        createDescriptorSetLayout();
        createGraphicsPipeline();
        createCommandPool();
        createDepthResources();
        createFramebuffers();
        createUniformBuffers();
        createDescriptorPool();
        createDescriptorSets();
        createCommandBuffers();
        createSyncObjects();
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
        appInfo.apiVersion = VK_API_VERSION_1_0;

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
        u32 glfwExtensionCount = 0;
        const char** glfwExtensions;
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

        if (enable_validation_layers)
        {
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        return extensions;
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
            throw std::runtime_error("no suitable GPUs found!");
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
            vkGetPhysicalDeviceSurfaceSupportKHR(d, i, surface, &presentSupport);

            if (presentSupport)
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

        std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

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
        QueueFamilyIndices indices = findQueueFamilies(d);

        bool extensionsSupported = checkDeviceExtensionSupport(d);

        bool swapChainAdequate = false;
        if (extensionsSupported)
        {
            SwapChainSupportDetails s = querySwapChainSupport(d);
            swapChainAdequate = !s.formats.empty() && !s.presentModes.empty();
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

        VkPhysicalDeviceFeatures deviceFeatures {};

        VkDeviceCreateInfo createInfo {};

        std::vector<const char*> extensions;

        extensions.emplace_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
        // Fix for mac: https://stackoverflow.com/questions/66659907/vulkan-validation-warning-catch-22-about-vk-khr-portability-subset-on-moltenvk
        extensions.emplace_back("VK_KHR_portability_subset");

        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.pQueueCreateInfos = queueCreateInfos.data();
        createInfo.queueCreateInfoCount = static_cast<u32>(queueCreateInfos.size());
        createInfo.pEnabledFeatures = &deviceFeatures;

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

    void createSwapChain()
    {
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
            if (f.format == VK_FORMAT_B8G8R8A8_SRGB && f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            {
                return f;
            }
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
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

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

    void createDescriptorSetLayout()
    {
        VkDescriptorSetLayoutBinding uboLayoutBinding {};
        uboLayoutBinding.binding = 0;
        uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboLayoutBinding.descriptorCount = 1;
        uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

        VkDescriptorSetLayoutCreateInfo layoutInfo {};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = 1;
        layoutInfo.pBindings = &uboLayoutBinding;

        auto result = vkCreateDescriptorSetLayout(device, &layoutInfo, null, &descriptorSetLayout);
        if (result != VK_SUCCESS)
        {
            printf("failed: %d\n", result);
            throw std::runtime_error("descriptor set layout failed!");
        }
    }

    void createGraphicsPipeline()
    {
        auto vertShaderCode = readFile("spv/shader.vert.spv");
        auto fragShaderCode = readFile("spv/shader.frag.spv");

        VkShaderModule vertSM = createShaderModule(vertShaderCode);
        VkShaderModule fragSM = createShaderModule(fragShaderCode);

        VkPipelineShaderStageCreateInfo vertSSI {};
        vertSSI.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertSSI.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertSSI.module = vertSM;
        vertSSI.pName = "main";

        VkPipelineShaderStageCreateInfo fragSSI {};
        fragSSI.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragSSI.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragSSI.module = fragSM;
        fragSSI.pName = "main";

        VkPipelineShaderStageCreateInfo shaderStages[] = { vertSSI, fragSSI };

        auto bindDesc = Vertex::getBindDesc();
        auto attribDesc = Vertex::getAttributeDesc();

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

        VkPipelineLayoutCreateInfo pipelineLayoutInfo {};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;

        // Adapted from https://vkguide.dev/docs/chapter-3/push_constants/
        //VkPipelineLayoutCreateInfo uniformsPipelineInfo {};
        //uniformsPipelineInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        VkPushConstantRange consts;
        consts.offset = 0;
        consts.size = sizeof(UniformBufferObject);
        consts.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        pipelineLayoutInfo.pPushConstantRanges = &consts;
        pipelineLayoutInfo.pushConstantRangeCount = 1;

        auto result = vkCreatePipelineLayout(device, &pipelineLayoutInfo, null, &pipelineLayout);
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
        pipelineInfo.layout = pipelineLayout;
        pipelineInfo.renderPass = renderPass;
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
        pipelineInfo.basePipelineIndex = -1;
        pipelineInfo.pDepthStencilState = &depthStencil;

        auto result1 = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, null, &graphicsPipeline);
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

    void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory)
    {
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = width;
        imageInfo.extent.height = height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = format;
        imageInfo.tiling = tiling;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = usage;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

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

    VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags)
    {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = format;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;
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

    void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout)
    {
        VkCommandBuffer commandBuffer = beginSingleTimeCommands();

        VkImageMemoryBarrier barrier{};
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
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

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
            throw std::runtime_error("vertex buffer memory allocation failed!");
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

        uniformBuffers.resize(max_frames_in_flight);
        uniformBuffersMemory.resize(max_frames_in_flight);
        uniformBuffersMapped.resize(max_frames_in_flight);

        for (size_t i = 0; i < max_frames_in_flight; i++)
        {
            createBuffer(size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffers[i], uniformBuffersMemory[i]);
            vkMapMemory(device, uniformBuffersMemory[i], 0, size, 0, &uniformBuffersMapped[i]);
        }
    }

    void createDescriptorPool()
    {
        VkDescriptorPoolSize size;
        size.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        size.descriptorCount = static_cast<u32>(max_frames_in_flight);

        VkDescriptorPoolCreateInfo poolInfo {};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = 1;
        poolInfo.pPoolSizes = &size;
        poolInfo.maxSets = static_cast<u32>(max_frames_in_flight);

        auto result = vkCreateDescriptorPool(device, &poolInfo, null, &descriptorPool);
        if (result != VK_SUCCESS)
        {
            printf("failed: %d\n", result);
            throw std::runtime_error("descriptor pool creation failed!");
        }
    }

    void createDescriptorSets()
    {
        std::vector<VkDescriptorSetLayout> layouts(max_frames_in_flight, descriptorSetLayout);
        VkDescriptorSetAllocateInfo allocInfo {};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = descriptorPool;
        allocInfo.descriptorSetCount = static_cast<u32>(max_frames_in_flight);
        allocInfo.pSetLayouts = layouts.data();

        descriptorSets.resize(max_frames_in_flight);
        auto result = vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data());

        if (result != VK_SUCCESS)
        {
            printf("failed: %d\n", result);
            throw std::runtime_error("descriptor sets creation failed!");
        }

        for (size_t i = 0; i < max_frames_in_flight; i++)
        {
            VkDescriptorBufferInfo bufferInfo {};
            bufferInfo.buffer = uniformBuffers[i];
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(UniformBufferObject);

            VkWriteDescriptorSet descriptorWrite {};
            descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrite.dstSet = descriptorSets[i];
            descriptorWrite.dstBinding = 0;
            descriptorWrite.dstArrayElement = 0;

            descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrite.descriptorCount = 1;

            descriptorWrite.pBufferInfo = &bufferInfo;

            vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, null);

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

        VkRenderPassBeginInfo renderPassInfo {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = renderPass;
        renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = swapChainExtent;

        std::array<VkClearValue, 2> clearValues {};
        clearValues[0].color = {0.0f, 0.0f, 0.0f, 1.0f};
        clearValues[1].depthStencil = {1.0f, 0};
        renderPassInfo.clearValueCount = clearValues.size();
        renderPassInfo.pClearValues = clearValues.data();

        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

        VkViewport viewport {};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(swapChainExtent.width);
        viewport.height = static_cast<float>(swapChainExtent.height);

        if (currentScene != null && currentScene->currentCamera != null)
        {
            float ar = currentScene->currentCamera->aspectRatio;
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

        mat4 transform = cameraRot
                         * mat4::translation(cameraPos * -1)
                         * currentScene->currentCamera->toTransform;

        currentScene->currentCamera->computeTransform(zoom);
        if (currentScene != null)
        {
            for (auto r: currentScene->roots)
            {
                r->computeCamera(mat4::I());
            }

            for (auto r: currentScene->roots)
            {
                r->draw(commandBuffer,transform);
            }
        }
//        vertexBuffer->draw(commandBuffer);
//        vertexBuffer2->draw(commandBuffer);

        vkCmdEndRenderPass(commandBuffer);

        auto result1 = vkEndCommandBuffer(commandBuffer);
        if (result1 != VK_SUCCESS)
        {
            printf("failed: %d\n", result1);
            throw std::runtime_error("ending to record command buffer failed!");
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
        std::string s = std::string(readFileWithCache("sg-Support.s72")->data());
        jarray* o = jparse_array(s);
        this->currentScene = parseScene(o);
    }

    void loop()
    {
        while (!glfwWindowShouldClose(window))
        {
            glfwPollEvents();
            handleControls();
            drawFrame();
        }

        vkDeviceWaitIdle(device);
    }

    void handleControls()
    {
        cameraSpeed = cameraSpeedMod;

        //printf("%f\n", cameraSpeed);

        vec3 x = (cameraRotInv * vec4(1, 0, 0, 1)).xyz();
        vec3 y = (cameraRotInv * vec4(0, 1, 0, 1)).xyz();
        vec3 z = (cameraRotInv * vec4(0, 0, 1, 1)).xyz();

        if (keyDown(GLFW_KEY_W))
            cameraPos = cameraPos - z * cameraSpeed * frameTime;

        if (keyDown(GLFW_KEY_S))
            cameraPos = cameraPos + z * cameraSpeed * frameTime;

        if (keyDown(GLFW_KEY_A))
            cameraPos = cameraPos - x * cameraSpeed * frameTime;

        if (keyDown(GLFW_KEY_D))
            cameraPos = cameraPos + x * cameraSpeed * frameTime;

        if (keyDown(GLFW_KEY_SPACE))
            cameraPos = cameraPos + y * cameraSpeed * frameTime;

        if (keyDown(GLFW_KEY_LEFT_SHIFT))
            cameraPos = cameraPos - y * cameraSpeed * frameTime;

        if (keyDown(GLFW_KEY_ENTER))
            cameraPos = vec3(0, 0, 0);

        if (mouseGrabbed)
        {
            double x;
            double y;
            glfwGetCursorPos(window, &x, &y);

            float scale = tan((currentScene->currentCamera->verticalFOV + zoom) / 2);
            if (mouseGrabbed > 5)
            {
                if (keyDown(GLFW_KEY_LEFT_CONTROL) || rightMouseDown)
                {
                    cameraRot = mat4::rotateAxis(vec3(0, 0, 1), (float) (((int) x - (int) mouseLastGrabX) / 500.0)) * cameraRot;
                    cameraRotInv = cameraRotInv * mat4::rotateAxis(vec3(0, 0, 1), (float) (-((int) x - (int) mouseLastGrabX) / 500.0));
                }
                else
                {
                    cameraRot =
                            mat4::rotateAxis(vec3(0, 1, 0),
                                             scale * (float) (((int) x - (int) mouseLastGrabX) / 500.0)) *
                            mat4::rotateAxis(vec3(1, 0, 0),
                                             scale * (float) (((int) y - (int) mouseLastGrabY) / 500.0)) *
                            cameraRot;
                    cameraRotInv =
                            cameraRotInv * mat4::rotateAxis(vec3(1, 0, 0),
                                             scale * (float) (-((int) y - (int) mouseLastGrabY) / 500.0)) *
                            mat4::rotateAxis(vec3(0, 1, 0),
                                             scale * (float) (-((int) x - (int) mouseLastGrabX) / 500.0));
                }
            }

            mouseLastGrabX = x;
            mouseLastGrabY = y;

            mouseGrabbed++;
        }
    }

    void drawFrame()
    {
        meshesDrawn = 0;
        auto time = std::chrono::high_resolution_clock::now();
        vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

        u32 imageIndex;
        auto result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

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

        vkResetFences(device, 1, &inFlightFences[currentFrame]);

        vkResetCommandBuffer(commandBuffers[currentFrame], 0);
        recordCommandBuffer(commandBuffers[currentFrame], imageIndex);

        VkSubmitInfo submitInfo {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame] };
        VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffers[currentFrame];

        VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame] };
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        auto result1 = vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]);
        if (result1 != VK_SUCCESS)
        {
            printf("failed: %d\n", result1);
            throw std::runtime_error("submitting draw command buffer failed!");
        }

        VkPresentInfoKHR presentInfo {};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;

        VkSwapchainKHR swapChains[] = { swapChain };
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;
        presentInfo.pImageIndices = &imageIndex;

        result = vkQueuePresentKHR(presentQueue, &presentInfo);
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

        auto end = std::chrono::high_resolution_clock::now();
        currentFrame = (currentFrame + 1) % max_frames_in_flight;
        frameTime = (end - time).count() / 1000000000.0f;
//        printf("drew %lu meshes in %lld\n", meshesDrawn, (end - time).count());
    };

    void updateUniformBuffer(mat4 m, u32 currentImage)
    {
        static auto startTime = std::chrono::high_resolution_clock::now();
        auto currentTime = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

        UniformBufferObject ubo {};
        ubo.modelView = m;
        //ubo.view = mat4::I(); //mat4::lookAt(vec3(0.0f, 0.0f, 3.0f), vec3(0.0f, 0.0f, 1.0f), vec3(0.0f, 1.0f, 0.0f));
        ubo.proj = currentScene->currentCamera->transform; //mat4::perspective((float) swapChainExtent.width / (float) swapChainExtent.height, PI / 4, 0.1f, 10.0f);
        //memcpy(uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));

        // adapted from https://vkguide.dev/docs/chapter-3/push_constants/
        vkCmdPushConstants(commandBuffers[currentFrame], pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(ubo), &ubo);
    }

    void cleanup()
    {
        if (this->currentScene != null)
            delete this->currentScene;

        cleanupSwapChain();

        for (size_t i = 0; i < max_frames_in_flight; i++)
        {
            vkDestroyBuffer(device, uniformBuffers[i], null);
            vkFreeMemory(device, uniformBuffersMemory[i], null);
        }

        vkDestroyDescriptorPool(device, descriptorPool, null);
        vkDestroyDescriptorSetLayout(device, descriptorSetLayout, null);

        vkDestroyPipeline(device, graphicsPipeline, null);
        vkDestroyPipelineLayout(device, pipelineLayout, null);

        vkDestroyRenderPass(device, renderPass, null);

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

        vkDestroySurfaceKHR(instance, surface, null);
        vkDestroyInstance(instance, null);
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

        vkDestroySwapchainKHR(device, swapChain, null);
    }

    void recreateSwapChain()
    {
        int w = 0;
        int h = 0;

        glfwGetFramebufferSize(window, &w, &h);
        while (w == 0 && h == 0)
        {
            glfwGetFramebufferSize(window, &w, &h);
            glfwWaitEvents();
        }

        vkDeviceWaitIdle(device);

        cleanupSwapChain();
        createSwapChain();
        createImageViews();
        createDepthResources();
        createFramebuffers();
    }

};

int main()
{
    Renderer app;

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