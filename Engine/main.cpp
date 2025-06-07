#include <assert.h>
#include <cstdio>
#include <vector>
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <vulkan/vulkan.h>

#define VK_CHECK(VK_FUNCTION) \
	do { \
		VkResult Result = VK_FUNCTION; \
		assert(Result == VK_SUCCESS); \
	} while (0); 

#define ARRAY_SIZE(x)  (sizeof(x) / sizeof((x)[0]))

// Temporary class to hold all the relevant engine data
// is not API agnostic (just yet)
// need to investigate other API's as well to
// determine the best layout and not building a
// vulkan only API
// Also, this class will be refactored as the project
// progresses
class EngineInstance
{
	public:
	// Variables // 
	VkInstance Instance;
	// Collection of available physical devices
	std::vector<VkPhysicalDevice> PhysicalDevices;
	// The device instance selected to create the device 
	// Restricted right now for 1 PhysicalDevice and 1 device
	// This is a auto-imposed restriction for now since you can have multiple
	// devices out of the same physical device, but keeping it simple for now
	VkPhysicalDevice PhysicalDevice;
	//
	GLFWwindow* window;
	
	VkDevice Device;
	
	VkSurfaceKHR surface;


	// Functions //
	// This is not the best naming
	// Since Engine::InitInstance is way more general than what
	// this function is actually doing.
	// Should consider move it / rename it once what 
	// it does changes over time
	void InitInstance();
	// Iterate over the list of physical devices and pick the one that
	// looks better or matches better the requirements of the engine
	void SelectPhysicalDevice();
	// Create a single device instance out of the physical device 
	void CreateDevice();
	// Create/Get surface for rendering
	void CreateSurface();
};


void EngineInstance::InitInstance()
{
	int InitResult = glfwInit();
	assert(InitResult);

	
	VkApplicationInfo ApplicationInfo {VK_STRUCTURE_TYPE_APPLICATION_INFO};
	ApplicationInfo.apiVersion = VK_API_VERSION_1_4;
	
	VkInstanceCreateInfo InstanceInfo {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
	InstanceInfo.pApplicationInfo = &ApplicationInfo;

	const char* Extensions[]=
		{
		VK_KHR_SURFACE_EXTENSION_NAME,
	};

	InstanceInfo.ppEnabledExtensionNames = Extensions;
	InstanceInfo.enabledExtensionCount = ARRAY_SIZE(Extensions);
	
#ifdef _DEBUG
	const char* DebugLayers[] = {
		"VK_LAYER_KHRONOS_validation",
	};

	InstanceInfo.ppEnabledLayerNames = DebugLayers;
	InstanceInfo.enabledLayerCount = ARRAY_SIZE(DebugLayers);
	
#endif
	
	VK_CHECK(vkCreateInstance(&InstanceInfo, nullptr, &Instance))

	uint32_t DeviceCount = 0;
	assert(PhysicalDevices.empty());
	VK_CHECK(vkEnumeratePhysicalDevices(Instance, &DeviceCount, nullptr));
	PhysicalDevices.resize(DeviceCount);
	VK_CHECK(vkEnumeratePhysicalDevices(Instance, &DeviceCount, PhysicalDevices.data()));
	// with this data we can start selecting which device is the best one for us

	const uint32_t WindowWidth = 1920;
	const uint32_t WindowHeigh = 1080;
	window = glfwCreateWindow(WindowWidth, WindowHeigh, "OutterSpace", 0, 0);
	
	SelectPhysicalDevice();
	CreateDevice();

	
	
}

void EngineInstance::SelectPhysicalDevice()
{
	assert(!PhysicalDevices.empty());
	printf("Checking physical devices, detected %llu\n", PhysicalDevices.size());

	uint32_t DeviceCandidate = 0xDEAD;
	for (uint32_t i = 0; i < PhysicalDevices.size(); i++)
	{
		VkPhysicalDeviceProperties Properties;
		vkGetPhysicalDeviceProperties(PhysicalDevices[i], &Properties);

		printf("Physical Device Name: %s\n", Properties.deviceName);
		printf("Physical Device Type: %d\n", Properties.deviceType);

		// We want to pick first a dedicated gpu
		if (Properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
		{
			DeviceCandidate = i;
		}
	}
	
	if (DeviceCandidate == 0xDEAD && !PhysicalDevices.empty())
	{
		printf("There is no candidate dedicated physical device, falling back to first gpu found\n");
		DeviceCandidate = 0;
		PhysicalDevice = PhysicalDevices[0];
	}
	else
	{
		PhysicalDevice = PhysicalDevices[DeviceCandidate];
	}
	VkPhysicalDeviceProperties Properties;
	vkGetPhysicalDeviceProperties(PhysicalDevices[DeviceCandidate], &Properties);
	printf("Selected Physical Device Name: %s\n", Properties.deviceName);
}

void EngineInstance::CreateDevice()
{
	assert(PhysicalDevice != VK_NULL_HANDLE);

	// This is not correct,
	// Validation layers will complain about it
	// But at the moment this is a shortcut
	constexpr float QueuePriorities[] = {1.0f};
	VkDeviceQueueCreateInfo DeviceQueueCreateInfo = {VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
	DeviceQueueCreateInfo.queueFamilyIndex = 0;
	DeviceQueueCreateInfo.queueCount = 1;
	DeviceQueueCreateInfo.pQueuePriorities = QueuePriorities;
	
	VkDeviceCreateInfo DeviceCreateInfo {VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};	
	DeviceCreateInfo.queueCreateInfoCount = 1;
	DeviceCreateInfo.pQueueCreateInfos = &DeviceQueueCreateInfo;

	//DeviceCreateInfo.enabledExtensionCount;
	//DeviceCreateInfo.ppEnabledExtensionNames;
	//DeviceCreateInfo.pEnabledFeatures;
	
	VK_CHECK(vkCreateDevice(PhysicalDevice, &DeviceCreateInfo, nullptr, &Device));
}

void EngineInstance::CreateSurface()
{
#if defined(VK_USE_PLATFORM_WIN32_KHR)
	VkWin32SurfaceCreateInfoKHR WindowCreateInfo {VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR};
	WindowCreateInfo.hinstance = GetModuleHandle(0);
	WindowCreateInfo.hwnd = glfwGetWin32Window(window);

	vkCreateWin32SurfaceKHR(Instance, &WindowCreateInfo, nullptr,&surface);
#else
	assert(false);
#endif
}


int main()
{
	printf("Hello world again \n");

	// GLFW is a temporary solution, 
	// later on in the project will be replaced
	// with a custom implementation.
	// Since the goal is to have the minimum overload 
	// of external libraries as possible
	EngineInstance Engine;
	Engine.InitInstance();
	
	while (!glfwWindowShouldClose(Engine.window)) 
	{
		glfwPollEvents();
	}

	glfwDestroyWindow(Engine.window);

	
	return 0;
}
