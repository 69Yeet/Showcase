#ifndef RENDERER_HPP
#define RENDERER_HPP

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vector>
#include <cstring>
#include <stdexcept>
#include <cstdlib>
#include <iostream>
#include <cassert>

#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif

const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) 
{
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
};

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) 
{
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}

class Renderer
{
	public:
		void run()
		{
			initWindow();
			initVulkan();
			mainLoop();
			cleanup();
		}

		void setWitdthHeight(uint32_t width, uint32_t height)
		{
			this->width = width;
			this->height = height;
		}

		void setMainArguments(uint32_t argc, char **argv)
		{
			this->argc = argc;
			this->argv = argv;
		}
	
	private:
		GLFWwindow* window;
		uint32_t width = 800;
		uint32_t height = 600;
		VkInstance instance;
		VkDebugUtilsMessengerEXT debugMessenger;
		uint32_t argc;
		char **argv;
		VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
		VkDevice device = VK_NULL_HANDLE;
		VkQueue graphicsQueue;
		
		
		void initWindow()
		{
			glfwInit();

			glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
			glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

			window = glfwCreateWindow(width, height, "Showcase", nullptr, nullptr);
		}

		void initVulkan()
		{
			CreateInstance();
			SetupDebugMessenger();
			PickPhysicalDevice();
			CreateLogicalDevice();
		}

		void mainLoop()
		{
			while (!glfwWindowShouldClose(window))
			{
				glfwPollEvents();
			}
		}

		void cleanup()
		{
			vkDestroyDevice(device, nullptr);

			if (enableValidationLayers) 
			{
    		    DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
    		}

			vkDestroyInstance(instance, nullptr);

			glfwDestroyWindow(window);
			glfwTerminate();
		}

		void CreateInstance()
		{
			if (enableValidationLayers && !CheckValidationLayerSupport()) 
			{
            	throw std::runtime_error("validation layers requested, but not available!");
        	}
			
			VkApplicationInfo appInfo
			{
				.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
				.pApplicationName = "Showcase",
				.apiVersion = VK_API_VERSION_1_3,
			};

			uint32_t glfwExtensionCount = 0;
			const char** glfwExtensions;

			glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

			auto extensions = GetRequiredExtensions();

			VkInstanceCreateInfo instanceCI
			{
				.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
				.pApplicationInfo = &appInfo,
				.enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
				.ppEnabledExtensionNames = extensions.data()
			};

			VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
			if (enableValidationLayers) 
			{
			    instanceCI.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			    instanceCI.ppEnabledLayerNames = validationLayers.data();

				PopulateDebugMessengerCreateInfo(debugCreateInfo);
        		instanceCI.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;
			} else {
			    instanceCI.enabledLayerCount = 0;
			}

			if (vkCreateInstance(&instanceCI, nullptr, &instance) != VK_SUCCESS)
			{
				throw std::runtime_error("failed to create instance!");
			}
		}

		void PickPhysicalDevice()
		{
			uint32_t deviceCount{ 0 };
			if (vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr) != VK_SUCCESS)
			{
				throw std::runtime_error("Failed to get device count!");
			}
			std::vector<VkPhysicalDevice> devices(deviceCount);
			if (vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data()) != VK_SUCCESS)
			{
				throw std::runtime_error("Failed to get device list!");
			}

			uint32_t deviceIndex{ 0 };
			if (argc > 1) {
			    deviceIndex = std::stoi(argv[1]);
			    assert(deviceIndex < deviceCount);
			}

			VkPhysicalDeviceProperties2 deviceProperties{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 };
			vkGetPhysicalDeviceProperties2(devices[deviceIndex], &deviceProperties);
			std::cout << "Selected device: " << deviceProperties.properties.deviceName <<  "\n";

			physicalDevice = devices[deviceIndex];
		}

		uint32_t PickQueueFamily(VkPhysicalDevice device)
		{
			uint32_t queueFamilyCount{ 0 };
			
			vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
			std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);

			vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

			uint32_t queueFamily;
			for (size_t i = 0; i < queueFamilies.size(); i++)
			{
			    if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
				{
			        queueFamily = i;
			        break;
			    }
			}

			if (glfwGetPhysicalDevicePresentationSupport(instance, device, queueFamily) != GLFW_TRUE)
			{
				throw std::runtime_error("Selected queue does not support presentation!");
			}

			return queueFamily;
		}

		void CreateLogicalDevice()
		{
			const float qfpriorities { 1.0f };
			const uint32_t queueFamily { PickQueueFamily(physicalDevice) };

			VkDeviceQueueCreateInfo queueCI
			{
			    .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
			    .queueFamilyIndex = queueFamily,
			    .queueCount = 1,
			    .pQueuePriorities = &qfpriorities
			};

			VkPhysicalDeviceVulkan12Features enabledVk12Features
			{
			    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
			    .descriptorIndexing = true,
			    .shaderSampledImageArrayNonUniformIndexing = true,
			    .descriptorBindingVariableDescriptorCount = true,
			    .runtimeDescriptorArray = true,
			    .bufferDeviceAddress = true
			};
			VkPhysicalDeviceVulkan13Features enabledVk13Features
			{
			    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
			    .pNext = &enabledVk12Features,
			    .synchronization2 = true,
			    .dynamicRendering = true,
			};
			VkPhysicalDeviceFeatures enabledVk10Features
			{
			    .samplerAnisotropy = VK_TRUE
			};

			const std::vector<const char *> deviceExtensions {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

			VkDeviceCreateInfo deviceCI
			{
				.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
				.pNext = & enabledVk13Features,
				.queueCreateInfoCount = 1,
				.pQueueCreateInfos = &queueCI,
				.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size()),
				.ppEnabledExtensionNames = deviceExtensions.data(),
				.pEnabledFeatures = &enabledVk10Features,
			};

			if (vkCreateDevice(physicalDevice, &deviceCI, nullptr, &device) != VK_SUCCESS)
			{
				throw std::runtime_error("Failed to create logical device!");
			}

			vkGetDeviceQueue(device, queueFamily, 0, &graphicsQueue);
		}

		void SetupDebugMessenger()
		{
			if (!enableValidationLayers) return;

			VkDebugUtilsMessengerCreateInfoEXT createInfo{};
			
			PopulateDebugMessengerCreateInfo(createInfo);

			if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) 
			{
			    throw std::runtime_error("failed to set up debug messenger!");
			}
		}

		bool CheckValidationLayerSupport() 
		{
	    	uint32_t layerCount;
	    	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	    	std::vector<VkLayerProperties> availableLayers(layerCount);
	    	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	    	for (const char* layerName : validationLayers) 
			{
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

		std::vector<const char*> GetRequiredExtensions() 
		{
		    uint32_t glfwExtensionCount = 0;
		    const char** glfwExtensions;
		    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

		    if (enableValidationLayers) {
		        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		    }
		
		    return extensions;
		}

		static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback
		(
		    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		    VkDebugUtilsMessageTypeFlagsEXT messageType,
		    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		    void* pUserData) {
			
		    std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
			
		    return VK_FALSE;
		}

		void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) 
		{
		    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		    createInfo.pfnUserCallback = DebugCallback;
		}
};

#endif
