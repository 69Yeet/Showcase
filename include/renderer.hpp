#ifndef RENDERER_HPP
#define RENDERER_HPP

#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include <vector>
#include <cstring>
#include <stdexcept>
#include <cstdlib>
#include <iostream>
#include <cassert>
#include <array>
#include <vulkan/vk_enum_string_helper.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "vertex.hpp"
#include "shader.hpp"
#include "texture.hpp"

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

std::vector<Vertex> testModel 
{
	{ {.5f, 0, .5f}, {0, -1, 0}, {0, 0} },
	{ {.5f, 0, -.5f}, {0, -1, 0}, {1, 0} },
	{ {-.5f, .5f, .5f}, {0, -1, 0}, {0, 1} },
	{ {-.5f, .5f, -.5f}, {0, -1, 0}, {1, 1} }
};

constexpr uint32_t maxFramesInFlight{2};

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
		uint32_t width { 800 };
		uint32_t height { 600 };
		VkInstance instance;
		VkDebugUtilsMessengerEXT debugMessenger;
		uint32_t argc;
		char **argv;
		VkPhysicalDevice physicalDevice { VK_NULL_HANDLE };
		VkDevice device { VK_NULL_HANDLE };
		VkQueue graphicsQueue;
		VkSurfaceCapabilitiesKHR surfaceCaps;
		VkSurfaceKHR surface { VK_NULL_HANDLE };

		std::array<VkCommandBuffer, maxFramesInFlight> commandBuffers;
		std::array<VkFence, maxFramesInFlight> fences;
		std::array<VkSemaphore, maxFramesInFlight> imageAcquiredSemaphores;
		std::vector<VkSemaphore> renderCompleteSemaphores;

		VkSwapchainKHR swapchain;
		uint32_t imageCount { 0 };
		std::vector<VkImage> swapchainImages;
		std::vector<VkImageView> swapchainImagesView;
		VkImage depthImage;
		VkImageView depthImageView;

		VkCommandPool commandPool;

		VkImage textureImage;
		VkImageView textureImageView;
		std::vector<VkDescriptorImageInfo> textureDescriptors{};

		//Memory variables to be changed later
		VkDeviceMemory depthImageMemory;
		VkBuffer vBuffer;
		VkDeviceMemory vertexMemory;
		std::array<ShaderDataBuffer, maxFramesInFlight> shaderDataBuffers;
		VkDeviceMemory textureImageMemory;
		std::array<Texture, 3> textures{};

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
			CreateSurface();
			CreateSwapchain();
			CreateDepthImage();
			LoadMesh();
			CreateShaderBuffer();
			SetupSynchronisation();
			CreateCommandBuffers();
			LoadTextures();
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
			CleanupSwapchain();

			CleanupTextures();

			CleanupSynchronisation();
			vkDestroyCommandPool(device, commandPool, nullptr);

			vkDestroyImageView(device, depthImageView, nullptr);
			vkDestroyImage(device, depthImage, nullptr);
			vkFreeMemory(device, depthImageMemory, nullptr);

			CleanupShaderDataBuffer();
			
			vkDestroyBuffer(device, vBuffer, nullptr);
			vkFreeMemory(device, vertexMemory, nullptr);

			vkDestroyDevice(device, nullptr);

			if (enableValidationLayers) 
			{
    		    DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
    		}

			vkDestroySurfaceKHR(instance, surface, nullptr);
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

		void CreateSurface()
		{
			if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS)
			{
				throw std::runtime_error("Failed to create window surface!");
			}

			if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCaps) != VK_SUCCESS)
			{
				throw std::runtime_error("Failed to get surface capabilities!");
			}
		}

		void CreateSwapchain()
		{
			VkExtent2D swapchainExtent { surfaceCaps.currentExtent };
			if (surfaceCaps.currentExtent.width = 0xFFFFFFFF)
			{
				swapchainExtent = {
					.width = static_cast<uint32_t>(width), 
					.height = static_cast<uint32_t>(height)
				};
			}

			const VkFormat imageFormat { VK_FORMAT_R8G8B8A8_SRGB};
			VkSwapchainCreateInfoKHR swapchainCI
			{
				.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
				.surface = surface,
				.minImageCount = surfaceCaps.minImageCount,
				.imageFormat = imageFormat,
				.imageColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR,
				.imageExtent = { 
					.width = static_cast<uint32_t>(width), 
					.height = static_cast<uint32_t>(height)
				},
				.imageArrayLayers = 1,
				.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
				.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
				.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
				.presentMode = VK_PRESENT_MODE_FIFO_KHR
			};

			if (vkCreateSwapchainKHR(device, &swapchainCI, nullptr, &swapchain) != VK_SUCCESS)
			{
				throw std::runtime_error("Failed to create swapchain!");
			}

			if (vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr) != VK_SUCCESS)
			{
				throw std::runtime_error("Failed to get swapchain image count!");
			}
			swapchainImages.resize(imageCount);

			if (vkGetSwapchainImagesKHR(device, swapchain, &imageCount, swapchainImages.data()) != VK_SUCCESS)
			{
				throw std::runtime_error("Failed to get swapchain images!");
			}
			swapchainImagesView.resize(imageCount);

			for (auto i = 0; i < imageCount; i++)
			{
				VkImageViewCreateInfo viewCI
				{
					.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
					.image = swapchainImages[i],
					.viewType = VK_IMAGE_VIEW_TYPE_2D, .format = imageFormat,
					.subresourceRange{
						.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
						.levelCount = 1,
						.layerCount = 1
					} 
				};
				if(vkCreateImageView(device, &viewCI, nullptr, &swapchainImagesView[i]) != VK_SUCCESS)
				{
					throw std::runtime_error("Failed to create image views!");
				}
			}
		}

		void CreateDepthImage()
		{
			std::vector<VkFormat> depthFormatList { VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT};
			VkFormat depthFormat { VK_FORMAT_UNDEFINED };
			for (VkFormat& format : depthFormatList)
			{
				VkFormatProperties2 formatProperties { .sType = VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_2 };
				vkGetPhysicalDeviceFormatProperties2(physicalDevice, format, &formatProperties);
				if (formatProperties.formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
				{
					depthFormat = format;
					break;
				}
			}

			VkImageCreateInfo depthImageCI
			{
				.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
				.imageType = VK_IMAGE_TYPE_2D,
				.format = depthFormat,
				.extent = { .width = width, .height = height, .depth = 1},
				.mipLevels = 1,
				.arrayLayers = 1,
				.samples = VK_SAMPLE_COUNT_1_BIT,
				.tiling = VK_IMAGE_TILING_OPTIMAL,
				.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
				.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
			};

			if (CreateImage(depthImage, depthImageMemory, depthImageCI, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) != VK_SUCCESS)
			{
				throw std::runtime_error("Failed to create depth image!");
			}
			
			VkImageViewCreateInfo depthViewCI
			{
				.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
				.image = depthImage,
				.viewType = VK_IMAGE_VIEW_TYPE_2D,
				.format = depthFormat,
				.subresourceRange { .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT, .levelCount = 1, .layerCount = 1 }
			};

			if (vkCreateImageView(device, &depthViewCI, nullptr, &depthImageView) != VK_SUCCESS)
			{
				throw std::runtime_error("Failed to create depth view!");
			}


		}

		void LoadMesh()
		{
			//const VkDeviceSize indexCount = {2};

			VkDeviceSize vBufSize{ sizeof(Vertex) * 4 };
			VkDeviceSize iBufSize{ sizeof(uint16_t) * 6 };

			std::vector<uint16_t> index{0, 1, 2, 1, 3, 2};

			VkBufferCreateInfo bufferCI
			{
			    .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			    .size = vBufSize + iBufSize,
			    .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
				.sharingMode = VK_SHARING_MODE_EXCLUSIVE
			};

			if (vkCreateBuffer(device, &bufferCI, nullptr, &vBuffer) != VK_SUCCESS)
			{
				throw std::runtime_error("Failed to create vertex buffer!");
			}

			VkMemoryRequirements memReqs;
			vkGetBufferMemoryRequirements(device, vBuffer, &memReqs);

			VkMemoryAllocateInfo vertexAI
			{
				.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
				.allocationSize = memReqs.size,
				.memoryTypeIndex = findMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
			};

			if (vkAllocateMemory(device, &vertexAI, nullptr, &vertexMemory) != VK_SUCCESS)
			{
				throw std::runtime_error("Failed to allocate memory to vertex buffer!");
			}

			vkBindBufferMemory(device, vBuffer, vertexMemory, 0);

			void* data;
			vkMapMemory(device, vertexMemory, 0, bufferCI.size, 0, &data);
			memcpy(data, testModel.data(), vBufSize);
			memcpy(((char *)data) + vBufSize, index.data(), iBufSize);
			vkUnmapMemory(device, vertexMemory);
		}

		void CreateShaderBuffer()
		{
			for (auto i = 0; i < maxFramesInFlight; i++) 
			{
    			VkBufferCreateInfo uBufferCI{
    			    .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
    			    .size = sizeof(ShaderData),
    			    .usage = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
    			};

				if (vkCreateBuffer(device, &uBufferCI, nullptr, &shaderDataBuffers[i].buffer) != VK_SUCCESS)
				{
					throw std::runtime_error("Failed to create shader data buffer!");
				}

				VkMemoryRequirements memReqs;
				vkGetBufferMemoryRequirements(device, shaderDataBuffers[i].buffer, &memReqs);

				VkMemoryAllocateFlagsInfo memAFI
				{
					.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO,
					.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT
				};

				shaderDataBuffers[i].allocationInfo = 
				{
					.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
					.pNext = &memAFI,
					.allocationSize = memReqs.size,
					.memoryTypeIndex = findMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
				};

				if (vkAllocateMemory(device, &shaderDataBuffers[i].allocationInfo, nullptr, &shaderDataBuffers[i].memory) != VK_SUCCESS)
				{
					throw std::runtime_error("Failed to allocate shader data memory!");
				}

				vkBindBufferMemory(device, shaderDataBuffers[i].buffer, shaderDataBuffers[i].memory, 0);

				VkBufferDeviceAddressInfo uBufferBdaInfo
				{
    			    .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
    			    .buffer = shaderDataBuffers[i].buffer
    			};
    			shaderDataBuffers[i].deviceAddress = vkGetBufferDeviceAddress(device, &uBufferBdaInfo);
			}
		}

		void SetupSynchronisation()
		{
			VkSemaphoreCreateInfo semaphoreCI
			{
			    .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
			};
			VkFenceCreateInfo fenceCI
			{
			    .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
			    .flags = VK_FENCE_CREATE_SIGNALED_BIT
			};
			for (auto i = 0; i < maxFramesInFlight; i++) {
			    if (vkCreateFence(device, &fenceCI, nullptr, &fences[i]) != VK_SUCCESS)
				{
					throw std::runtime_error("Failed to setup fence!");
				}
			    if (vkCreateSemaphore(device, &semaphoreCI, nullptr, &imageAcquiredSemaphores[i]) != VK_SUCCESS)
				{
					throw std::runtime_error("Failed to setup semaphore!");
				}
			}
			renderCompleteSemaphores.resize(swapchainImages.size());
			for (auto i = 0; i < swapchainImages.size(); i++)
			{
			    if (vkCreateSemaphore(device, &semaphoreCI, nullptr, &renderCompleteSemaphores[i]) != VK_SUCCESS)
				{
					throw std::runtime_error("/n Failed to setup render semaphores!");
				}
			}
		}

		void CreateCommandBuffers()
		{
			VkCommandPoolCreateInfo commandPoolCI
			{
				.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
				.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
				.queueFamilyIndex = PickQueueFamily(physicalDevice)
			};

			if (vkCreateCommandPool(device, &commandPoolCI, nullptr, &commandPool) != VK_SUCCESS)
			{
				throw std::runtime_error("Failed to create command pool!");
			}

			VkCommandBufferAllocateInfo commandBufferAI
			{
				.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
				.commandPool = commandPool,
				.commandBufferCount = maxFramesInFlight
			};

			if (vkAllocateCommandBuffers(device, &commandBufferAI, commandBuffers.data()) != VK_SUCCESS)
			{
				throw std::runtime_error("Failed to create command buffers!");
			} 
		}

		void LoadTextures()
		{
			for (auto i = 0; i < textures.size(); i++)
			{

				int texWidth, texHeight, texChannels;
				std::string filename = "F:/Programming/Cpp/Masterpiece/build/textures/texture" + std::to_string(i) + ".jpg";
    			stbi_uc* pixels = stbi_load(filename.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

				if (pixels == nullptr)
				{
					std::cout << "Texture doesn't exist! \n";
					break;
				}

    			VkDeviceSize imageSize = texWidth * texHeight * 4;

    			if (!pixels) {
    			    throw std::runtime_error("failed to load texture image!");
    			}

				VkBuffer stagingBuffer;
				VkDeviceMemory stagingBufferMemory;

				CreateBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
							VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);
				
				void* data;
				vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
		    	memcpy(data, pixels, static_cast<size_t>(imageSize));
				vkUnmapMemory(device, stagingBufferMemory);

				stbi_image_free(pixels);

				VkImageCreateInfo texImgCI
				{
				    .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
				    .imageType = VK_IMAGE_TYPE_2D,
				    .format = VK_FORMAT_R8G8B8A8_SRGB,
				    .extent = {.width = (uint32_t)texWidth, .height = (uint32_t)texHeight, .depth = 1 },
				    .mipLevels = 1,
				    .arrayLayers = 1,
				    .samples = VK_SAMPLE_COUNT_1_BIT,
				    .tiling = VK_IMAGE_TILING_OPTIMAL,
				    .usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
				    .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
				};

				if (vkCreateImage(device, &texImgCI, nullptr, &textures[i].image) != VK_SUCCESS)
				{
					throw std::runtime_error("Failed to create image!");
				}


				VkMemoryRequirements memReqs;
				vkGetImageMemoryRequirements(device, textures[i].image, &memReqs);

				VkMemoryAllocateInfo imageAI
				{
					.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
					.allocationSize = memReqs.size,
					.memoryTypeIndex = findMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
				};

				if (vkAllocateMemory(device, &imageAI, nullptr, &textures[i].memory) != VK_SUCCESS)
				{
					throw std::runtime_error("Failed to allocate memory to texture!");
				}

				VkImageViewCreateInfo texViewCI
				{
				    .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
				    .image = textures[i].image,
				    .viewType = VK_IMAGE_VIEW_TYPE_2D,
				    .format = texImgCI.format,
				    .subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1, .layerCount = 1 }
				};

				vkBindImageMemory(device, textures[i].image, textures[i].memory, 0);
				
				if (vkCreateImageView(device, &texViewCI, nullptr, &textures[i].view) != VK_SUCCESS)
				{
					throw std::runtime_error("Failed to create depth view!");
				}

				VkFenceCreateInfo fenceOneTimeCI
				{
				    .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO
				};
				VkFence fenceOneTime{};
				if (vkCreateFence(device, &fenceOneTimeCI, nullptr, &fenceOneTime) != VK_SUCCESS)
				{
					throw std::runtime_error("Failed to create fence!");
				}
				VkCommandBuffer cbOneTime{};
				VkCommandBufferAllocateInfo cbOneTimeAI{
				    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
				    .commandPool = commandPool,
				    .commandBufferCount = 1
				};

				if (vkAllocateCommandBuffers(device, &cbOneTimeAI, &cbOneTime) != VK_SUCCESS)
				{
					throw std::runtime_error("Failed to create command buffer!");
				}

				VkCommandBufferBeginInfo cbOneTimeBI
				{
				    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
				    .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
				};

				if(vkBeginCommandBuffer(cbOneTime, &cbOneTimeBI) != VK_SUCCESS)
				{
					throw std::runtime_error("Failed to start command buffer!");
				}

				VkImageMemoryBarrier2 barrierTexImage{
				    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
				    .srcStageMask = VK_PIPELINE_STAGE_2_NONE,
				    .srcAccessMask = VK_ACCESS_2_NONE,
				    .dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
				    .dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
				    .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
				    .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				    .image = textures[i].image,
				    .subresourceRange = { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1, .layerCount = 1 }
				};
				VkDependencyInfo barrierTexInfo{
				    .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
				    .imageMemoryBarrierCount = 1,
				    .pImageMemoryBarriers = &barrierTexImage
				};
				vkCmdPipelineBarrier2(cbOneTime, &barrierTexInfo);

				VkBufferImageCopy region
				{
    	    		.bufferOffset = 0,
    	    		.bufferRowLength = 0,
    	    		.bufferImageHeight = 0,
					.imageSubresource =
					{
    	    			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
    	    			.mipLevel = 0,
    	    			.baseArrayLayer = 0,
    	    			.layerCount = 1,
					},
    	    		.imageOffset = {0, 0, 0},
    	    		.imageExtent = 
					{
    	    		    (uint32_t)texWidth,
    	    		    (uint32_t)texHeight,
    	    		    1
    	    		}
				};
				vkCmdCopyBufferToImage(cbOneTime, stagingBuffer, textures[i].image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
				VkImageMemoryBarrier2 barrierTexRead
				{
				    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
				    .srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT,
				    .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
				    .dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
				    .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
				    .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				    .newLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL,
				    .image = textures[i].image,
				    .subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1, .layerCount = 1 }
				};
				barrierTexInfo.pImageMemoryBarriers = &barrierTexRead;
				vkCmdPipelineBarrier2(cbOneTime, &barrierTexInfo);
				if (vkEndCommandBuffer(cbOneTime) != VK_SUCCESS)
				{
					throw std::runtime_error("Failed to end command buffer!");
				}

				VkCommandBufferSubmitInfo cbOneTimeSubmitInfo
				{
				    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
				    .commandBuffer = cbOneTime
				};

				VkSubmitInfo2 oneTimeSI
				{
				    .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
				    .commandBufferInfoCount = 1,
				    .pCommandBufferInfos = &cbOneTimeSubmitInfo
				};

				if (vkQueueSubmit2(graphicsQueue, 1, &oneTimeSI, fenceOneTime) != VK_SUCCESS)
				{
					throw std::runtime_error("Failed to submit queue");
				}
				if (vkWaitForFences(device, 1, &fenceOneTime, VK_TRUE, UINT64_MAX) != VK_SUCCESS)
				{
					throw std::runtime_error("Failed to wait for fence!");
				}

				vkDestroyBuffer(device, stagingBuffer, nullptr);
				vkFreeMemory(device, stagingBufferMemory, nullptr);
				vkDestroyFence(device, fenceOneTime, nullptr);

				VkSamplerCreateInfo samplerCI
				{
				    .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
				    .magFilter = VK_FILTER_LINEAR,
				    .minFilter = VK_FILTER_LINEAR,
				    .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
				    .anisotropyEnable = VK_TRUE,
				    .maxAnisotropy = 8.0f, // 8 is a widely supported value for max anisotropy
				    .maxLod = 0
				};
				if (vkCreateSampler(device, &samplerCI, nullptr, &textures[i].sampler) != VK_SUCCESS)
				{
					throw std::runtime_error("Failed to create sampler!");
				}

				textureDescriptors.push_back({
				    .sampler = textures[i].sampler,
				    .imageView = textures[i].view,
				    .imageLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL
				});
			}
		}

		void CleanupTextures()
		{
			for (auto i = 0; i < textures.size(); i++)
			{
				vkDestroySampler(device, textures[i].sampler, nullptr);
				vkDestroyImageView(device, textures[i].view, nullptr);
				vkDestroyImage(device, textures[i].image, nullptr);
				vkFreeMemory(device, textures[i].memory, nullptr);
			}
		}

		void CleanupSynchronisation()
		{
			for (int i = 0; i < maxFramesInFlight; i++)
			{
				vkDestroyFence(device, fences[i], nullptr);
				vkDestroySemaphore(device, imageAcquiredSemaphores[i], nullptr);
			}

			for (auto& semaphore : renderCompleteSemaphores)
			{
				vkDestroySemaphore(device, semaphore, nullptr);
			}
		}

		void CleanupShaderDataBuffer()
		{
			for (auto i = 0; i < maxFramesInFlight; i++) 
			{
				shaderDataBuffers[i].deviceAddress = 0;
				vkDestroyBuffer(device, shaderDataBuffers[i].buffer, nullptr);
				vkFreeMemory(device, shaderDataBuffers[i].memory, nullptr);
			}
		}

		void CleanupSwapchain()
		{
			for (VkImageView imageView : swapchainImagesView)
			{
				vkDestroyImageView(device, imageView, nullptr);
			}

			vkDestroySwapchainKHR(device, swapchain, nullptr);
		}

		void CreateBuffer(VkDeviceSize size, VkBufferUsageFlagBits usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
		{
			VkBufferCreateInfo bufferCI
			{
				.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
				.size = size,
				.usage = usage,
				.sharingMode = VK_SHARING_MODE_EXCLUSIVE
			};

			if (vkCreateBuffer(device, &bufferCI, nullptr, &buffer) != VK_SUCCESS)
			{
				throw std::runtime_error("Failed to create buffer!");
			}

			VkMemoryRequirements memReqs;
			vkGetBufferMemoryRequirements(device, buffer, &memReqs);

			VkMemoryAllocateInfo bufferAI
			{
				.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
				.allocationSize = memReqs.size,
				.memoryTypeIndex = findMemoryType(memReqs.memoryTypeBits, properties)
			};

			if (vkAllocateMemory(device, &bufferAI, nullptr, &bufferMemory) != VK_SUCCESS) 
			{
            	throw std::runtime_error("failed to allocate buffer memory!");
        	}

        	vkBindBufferMemory(device, buffer, bufferMemory, 0);
		}

		VkResult CreateImage(VkImage& image, VkDeviceMemory& memory, VkImageCreateInfo& imageCI, VkMemoryPropertyFlags properties)
		{
			if (vkCreateImage(device, &imageCI, nullptr, &image) != VK_SUCCESS) 
			{
        		return VK_ERROR_INITIALIZATION_FAILED;
    		}
			VkMemoryRequirements memReqs;
			vkGetImageMemoryRequirements(device, image, &memReqs);

			VkMemoryAllocateInfo imageAI
			{
				.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
				.allocationSize = memReqs.size,
				.memoryTypeIndex = findMemoryType(memReqs.memoryTypeBits, properties)
			};

			if (vkAllocateMemory(device, &imageAI, nullptr, &memory) != VK_SUCCESS)
			{
				return VK_ERROR_OUT_OF_HOST_MEMORY;
			}

			vkBindImageMemory(device, image, memory, 0);

			return VK_SUCCESS;
		}

		uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) 
		{
    	    VkPhysicalDeviceMemoryProperties memProperties;
    	    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    	    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
    	        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
    	            return i;
    	        }
    	    }

    	    throw std::runtime_error("failed to find suitable memory type!");
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
