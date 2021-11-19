//#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
/*
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>*/

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <optional>
#include <set>


//Если разкоментировать то будет выведена информация о расширениях
//#define Show_extensions

//размеры окна
const uint32_t WIDTH = 1920;
const uint32_t HEIGHT = 1080;

// Набор уровней валидации которые мы хотим поддерживать
const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

// Набор необходимых расширений гпу
const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

//Проверка на то проводим ли мы дебаг или нет
#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif
//Семейство индексов очередей
struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;
    //проверка на то, появилось ли значение
    bool isComplete() {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

//метод для получения указателя на очередь
QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface ) {
    // сам индекс
    QueueFamilyIndices indices;
    //колличество очередей
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
    //сами очереди
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());
    //индекс
    int i = 0;
    VkBool32 presentSupport = false;
    for (const auto& queueFamily : queueFamilies) {
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphicsFamily = i;
        }
        if (indices.isComplete()) {
            break;
        }
        if (presentSupport) {
            indices.presentFamily = i;
        }
        i++;
    }

    return indices;
}

class TheGreatestEngine {
public:
    // Главный запуск
    void run() {
        // Инициализация окна приложения
        initWindow();
        // Инициализация вулкана
        initVulkan();
        // Главный цикл
        mainLoop();
        // Очищение при выключении
        cleanup();
    }

private:
    // Главный объект движка
    VkInstance instance;
    // Окно приложения
    GLFWwindow* window;
    // Поверхность окна
    VkSurfaceKHR surface;
    //Физическое гпу
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    //Логическое устройство
    VkDevice device;
    // Дескриптор графической очереди
    VkQueue graphicsQueue;
    // Дескриптор очереди выводящей на экран
    VkQueue presentQueue;
    //Сравнение поддерживаемых уровней валидации с требуемыми
    bool checkValidationLayerSupport() {
        //колличество уровней
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
        //сами уровни
        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        for (const char* layerName : validationLayers) {
            bool layerFound = false;

            for (const auto& layerProperties : availableLayers) {
                if (strcmp(layerName, layerProperties.layerName) == 0) {
                    layerFound = true;
                    break;
                }
            }

            if (!layerFound) {
                return false;
            }
        }

        return true;
    }

    //Просто информация о приложении
    VkApplicationInfo* createInstanceInfo() {
        //Проверка на то, поддерживаются ли все уровни валидации
        if (enableValidationLayers && !checkValidationLayerSupport()) {
            throw std::runtime_error("validation layers requested, but not available!");
        }
        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Hello Triangle";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "No Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_0;
        return &appInfo;
    }

    void initInstance()
    {
        // Объект с информацией конкретно для создания движка
        VkInstanceCreateInfo createInfo{};
        if (enableValidationLayers) {
            //createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size()); ЛИШНЕЕ
            createInfo.ppEnabledLayerNames = validationLayers.data();
        }
        else {
            createInfo.enabledLayerCount = 0;
        }
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        //createInfo.pApplicationInfo = createInstanceInfo(); ЛИШНЕЕ

        
        // Выводит в консоль все доступные расширения
        #ifdef Show_extensions

        uint32_t extensionCount = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
        std::vector<VkExtensionProperties> extensions(extensionCount);
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());
        std::cout << "available extensions:\n";

        for (const auto& extension : extensions) {
            std::cout << '\t' << extension.extensionName << '\n';
        }

        #endif

        // Расширения оконного приложения
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions;

        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        createInfo.enabledExtensionCount = glfwExtensionCount;
        createInfo.ppEnabledExtensionNames = glfwExtensions;

        createInfo.enabledLayerCount = 0;
        // Торжественный момент инициализации объекта вулкана
        if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
            throw std::runtime_error("failed to create instance!");
        }
    }

    //Создание поверхности экрана
    void createSurface() {
        if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
            throw std::runtime_error("failed to create window surface!");
        }
    }

    bool checkDeviceExtensionSupport(VkPhysicalDevice device) {
        // колличество поддерживаемых разрешений
        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
        // доступные разрешения
        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());
        // набор необходимых расширений
        std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());
        // сравнение доступных и требуемых расширений
        for (const auto& extension : availableExtensions) {
            requiredExtensions.erase(extension.extensionName);
        }
        return requiredExtensions.empty();
    }

    //проверка на то работает ли гпу с Vulcan
    bool isDeviceSuitable(VkPhysicalDevice device) {

        QueueFamilyIndices indices = findQueueFamilies(device, surface);

        bool extensionsSupported = checkDeviceExtensionSupport(device);

        return indices.isComplete() && extensionsSupported;
    }

    //выбираем гпу
    void pickPhysicalDevice() {
        //колличество гпу
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
        //если их нет то и ничто не поддерживает вулка
        if (deviceCount == 0) {
            std::cout << deviceCount;
            throw std::runtime_error("failed to find GPUs with Vulkan support!");
        }
        //берём указатели на каждый объект
        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
        //выбираем любой подходящий девайс
        for (const auto& device : devices) {
            if (isDeviceSuitable(device)) {
                physicalDevice = device;
                break;
            }
        }
        //если ничего не выбирается, то возвращается ошибка
        if (physicalDevice == VK_NULL_HANDLE) {
            throw std::runtime_error("failed to find a suitable GPU!");
        }
    }
    // создание логического гпу
    void createLogicalDevice() {
        //находим семейство очередей
        QueueFamilyIndices indices = findQueueFamilies(physicalDevice, surface);
        //Создаём информацию о всех очередях
        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };

        float queuePriority = 1.0f;
        for (uint32_t queueFamily : uniqueQueueFamilies) {
            VkDeviceQueueCreateInfo queueCreateInfo{};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamily;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;
            queueCreateInfos.push_back(queueCreateInfo);
        }
        // Запрашиваем возможности гпу
        VkPhysicalDeviceFeatures deviceFeatures{};
        // создаём информацию о логическом устройстве
        VkDeviceCreateInfo createInfo{};
        //оформляем
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        createInfo.pQueueCreateInfos = queueCreateInfos.data();
        // прередаём возможности
        createInfo.pEnabledFeatures = &deviceFeatures;
        // проверка на поддержку расширений
        createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
        createInfo.ppEnabledExtensionNames = deviceExtensions.data();
        // Поддержка уровней валидации
        if (enableValidationLayers) {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();
        }
        else {
            createInfo.enabledLayerCount = 0;
        }
        
        // Создаём само логическое устройство
        if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
            throw std::runtime_error("failed to create logical device!");
        }
        // создаём дискриптор очереди
        vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
        vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
    }

    void initVulkan() {
        // Создание Instance
        initInstance();
        // Создание поверхности экрана
        createSurface();
        // Выбор видеоустройства
        pickPhysicalDevice();
        // Создаём логическое устройство
        createLogicalDevice();

    }

    void initWindow() {
        // Инициализация объекта для создания окна
        glfwInit();
        // Настройки чтобы оно работало с вулканом
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        // Создаётся само окно
        window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
    }

    void mainLoop() {
        //Цикл который смотрит пока окно не попробуют закрыть
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
        }
    }

    void cleanup() {
        //удаляем поверхность
        vkDestroySurfaceKHR(instance, surface, nullptr);
        //Удаление объекта вулкана
        vkDestroyInstance(instance, nullptr);
        //Закрытие окна
        glfwDestroyWindow(window);
        //Удаление объекта создающего окна
        glfwTerminate();
        // Удаление логического устройства
        vkDestroyDevice(device, nullptr);
    }
};

int main() {
    TheGreatestEngine app;
    //Запуск с возможностью выловить ошибки
    try {
        app.run();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
