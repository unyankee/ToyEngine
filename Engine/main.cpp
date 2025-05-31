#include <stdio.h>
#include <assert.h>
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>


#define VK_CHECK(VK_FUNCTION) \
	do { \
		VkResult Result = VK_FUNCTION; \
		assert(Result == VK_SUCCESS); \
	} while (0); 

#define ARRAY_SIZE(x)  (sizeof(x) / sizeof((x)[0]))


int main()
{
	printf("Hello world again \n");

	// GLFW is a temporary solution, 
	// later on in the project will be replaced
	// with a custom implementation.
	// Since the goal is to have the minimum overload 
	// of external libraries as possible
	int InitResult = glfwInit();
	assert(InitResult);

	VkApplicationInfo ApplicationInfo {VK_STRUCTURE_TYPE_APPLICATION_INFO};
	ApplicationInfo.apiVersion = VK_API_VERSION_1_4;
	ApplicationInfo.pApplicationName;
	ApplicationInfo.applicationVersion;
	ApplicationInfo.pEngineName;
	ApplicationInfo.engineVersion;


	VkInstance Instance;
	VkInstanceCreateInfo InstanceInfo {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
	InstanceInfo.pApplicationInfo = &ApplicationInfo;
	InstanceInfo.flags;

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

	
	VK_CHECK(vkCreateInstance(&InstanceInfo, nullptr, &Instance));

	const uint32_t WindowWidth = 1920;
	const uint32_t WindowHeigh = 1080;
	GLFWwindow* window = glfwCreateWindow(WindowWidth, WindowHeigh, "OutterSpace", 0, 0);


	while (!glfwWindowShouldClose(window)) 
	{
		glfwPollEvents();
	}

	glfwDestroyWindow(window);

	vkDestroyInstance(Instance, 0);
	
	return 0;
}