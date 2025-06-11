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
	//
	VkDevice Device;
	//	
	VkSurfaceKHR surface;
	//	
	VkSurfaceFormatKHR surfaceFormat;
	//
	VkSwapchainKHR Swapchain;
	//
	uint32_t FamilyIndex = 0;
	//
	uint32_t swapchainImagesCount = 0;
	//
	std::vector<VkImage> SwapchainImages;
	//
	VkRenderPass RenderPass;
	//
	std::vector<VkFramebuffer> Framebuffer;
	//
	std::vector<VkImageView> SwapchainImageViews;
	//
	uint32_t Width = 1920;
	uint32_t Height = 1080;
	//
	VkPipeline MeshPipeline;
	VkPipelineLayout MeshPipelineLayout;
	
	// Functions //
	// This is not the best naming
	// Since Engine::InitInstance is way more general than what
	// this function is actually doing.
	// Should consider move it / rename it once what 
	// it does changes over time
	//
	void MainLoop();
	//
	void InitInstance();
	// Iterate over the list of physical devices and pick the one that
	// looks better or matches better the requirements of the engine
	void SelectPhysicalDevice();
	// Create a single device instance out of the physical device 
	void CreateDevice();
	// Create/Get surface for rendering
	void CreateSurface();
	// 
	void GetSwapchainFormat();
	//
	void CreateSwapchain();
	//
	void createRenderpass();
	//
	void GetOrCreateSwapchainImages();
	//
	void createImageView();
	//
	void createFramebuffer();
	//
	void createMeshPipeline(VkShaderModule vs, VkShaderModule fs);
	// 
	VkShaderModule loadShader(const char* shaderPath) const;
	
	VkCommandPool CreateCommandPool();
};


void EngineInstance::MainLoop()
{
	VkSemaphore AquireSemaphone;
	VkSemaphore submitSemaphore;
	VkSemaphoreCreateInfo SemaphoreCreateInfo = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
	
	vkCreateSemaphore(Device, &SemaphoreCreateInfo, nullptr, &AquireSemaphone);
	vkCreateSemaphore(Device, &SemaphoreCreateInfo, nullptr, &submitSemaphore);

	VkQueue Queue;
	vkGetDeviceQueue(Device, FamilyIndex, 0, &Queue);
	
	VkCommandPool CommandPool = CreateCommandPool();
	
	VkCommandBufferAllocateInfo AllocateInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
	AllocateInfo.commandPool = CommandPool;
	AllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	AllocateInfo.commandBufferCount = 1;

	VkCommandBuffer CommandBuffer;
	VK_CHECK(vkAllocateCommandBuffers(Device, &AllocateInfo, &CommandBuffer));

	VkShaderModule MeshVs = loadShader("Shaders/mesh.vert.spv");
	VkShaderModule MeshFs = loadShader("Shaders/mesh.frag.spv");
	createMeshPipeline(MeshVs, MeshFs);
	
	while (!glfwWindowShouldClose(window)) 
	{
		glfwPollEvents();

		// Main rendering loop for now
		uint32_t ImageIndex = 0;
		VK_CHECK(vkAcquireNextImageKHR(Device, Swapchain,  ~0ull, AquireSemaphone, VK_NULL_HANDLE, &ImageIndex));

		VK_CHECK(vkResetCommandPool(Device, CommandPool, 0))


		VkCommandBufferBeginInfo BeginInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
		BeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		
		
		VK_CHECK(vkBeginCommandBuffer(CommandBuffer, &BeginInfo));

		constexpr VkClearColorValue ClearColorValue = {27.0f/ 255.0f,3.0f / 255.0f,3.0f / 255.0f,1.0f};
		VkClearValue ClearValue = {ClearColorValue};
		
		VkRenderPassBeginInfo RenderpassBeginInfo = {VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
		RenderpassBeginInfo.renderPass = RenderPass;
		RenderpassBeginInfo.framebuffer = Framebuffer[ImageIndex];
		RenderpassBeginInfo.renderArea.extent = {Width, Height};
		RenderpassBeginInfo.renderArea.offset = {0,0};
		RenderpassBeginInfo.clearValueCount = 1;
		RenderpassBeginInfo.pClearValues = &ClearValue;
		
		
		vkCmdBeginRenderPass(CommandBuffer, &RenderpassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		VkViewport Viewport = {0,float(Height), float(Width), -float(Height), 0.0f, 1.0f};
		vkCmdSetViewport(CommandBuffer, 0, 1, &Viewport);

		VkRect2D Scissor = {0,0, (Width), (Height)};
		vkCmdSetScissor(CommandBuffer, 0, 1, &Scissor);

		vkCmdBindPipeline(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, MeshPipeline);
		vkCmdDraw(CommandBuffer, 3,1,0,0);
			
		
		vkCmdEndRenderPass(CommandBuffer);
		
		//vkCmdClearColorImage(CommandBuffer, SwapchainImages[ImageIndex], VK_IMAGE_LAYOUT_GENERAL, &ClearColorValue, 1, &ImageSubresourceRamge);
		
		VK_CHECK(vkEndCommandBuffer(CommandBuffer))

		VkPipelineStageFlags PipelineStageFlags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		
		VkSubmitInfo SubmitInfo = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
		const void*                    pNext;
		SubmitInfo.pWaitDstStageMask = &PipelineStageFlags;
		SubmitInfo.waitSemaphoreCount = 1;
		SubmitInfo.pWaitSemaphores = &AquireSemaphone;
		SubmitInfo.commandBufferCount = 1;
		SubmitInfo.pCommandBuffers = &CommandBuffer;
		SubmitInfo.signalSemaphoreCount = 1;
		SubmitInfo.pSignalSemaphores = &submitSemaphore;
		
		vkQueueSubmit(Queue, 1, &SubmitInfo, VK_NULL_HANDLE);
		
		VkPresentInfoKHR PresentInfo = {VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
		PresentInfo.pImageIndices = &ImageIndex;
		PresentInfo.pWaitSemaphores = &submitSemaphore;
		PresentInfo.waitSemaphoreCount = 1;
		PresentInfo.pSwapchains = &Swapchain;
		PresentInfo.swapchainCount = 1;

		VK_CHECK(vkQueuePresentKHR(Queue, &PresentInfo))
		// This is bad. here just for purely testing 
		VK_CHECK(vkDeviceWaitIdle(Device))
	}
}

VkCommandPool EngineInstance::CreateCommandPool()
{
	VkCommandPoolCreateInfo CommandPoolCreateInfo = {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
	CommandPoolCreateInfo.queueFamilyIndex = FamilyIndex;
	CommandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;

	VkCommandPool CommandPool = VK_NULL_HANDLE;
	VK_CHECK(vkCreateCommandPool(Device, &CommandPoolCreateInfo, nullptr, &CommandPool));
	return CommandPool;
}

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
#if defined(VK_USE_PLATFORM_WIN32_KHR)
		VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#endif
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

	const uint32_t WindowWidth = Width;
	const uint32_t WindowHeigh = Height;
	window = glfwCreateWindow(WindowWidth, WindowHeigh, "OutterSpace", 0, 0);
	
	SelectPhysicalDevice();
	CreateDevice();
	CreateSurface();
	GetSwapchainFormat();
	CreateSwapchain();
	GetOrCreateSwapchainImages();
	createRenderpass();
	createImageView();
	createFramebuffer();
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
	DeviceQueueCreateInfo.queueFamilyIndex = FamilyIndex;
	DeviceQueueCreateInfo.queueCount = 1;
	DeviceQueueCreateInfo.pQueuePriorities = QueuePriorities;
	
	VkDeviceCreateInfo DeviceCreateInfo {VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};	
	DeviceCreateInfo.queueCreateInfoCount = 1;
	DeviceCreateInfo.pQueueCreateInfos = &DeviceQueueCreateInfo;

	const char* Extensions[]=
		{
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
#if defined(VK_USE_PLATFORM_WIN32_KHR)
		
#endif
	};

	DeviceCreateInfo.ppEnabledExtensionNames = Extensions;
	DeviceCreateInfo.enabledExtensionCount = ARRAY_SIZE(Extensions);
	
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

void EngineInstance::GetSwapchainFormat()
{
	uint32_t DeviceSurfaceCount = 0;
	std::vector<VkSurfaceFormatKHR> SupportedFormats;
	vkGetPhysicalDeviceSurfaceFormatsKHR(PhysicalDevice, surface, &DeviceSurfaceCount, nullptr);
	SupportedFormats.resize(DeviceSurfaceCount);
	vkGetPhysicalDeviceSurfaceFormatsKHR(PhysicalDevice, surface, &DeviceSurfaceCount, SupportedFormats.data());

	uint32_t Candidate = 0;
	for (uint32_t i = 0; i < DeviceSurfaceCount; i++)
	{
		if (SupportedFormats[i].format == VK_FORMAT_B8G8R8A8_SRGB)
		{
			Candidate = i;
		}
	}

	surfaceFormat = SupportedFormats[Candidate];
}

void EngineInstance::CreateSwapchain()
{

	int32_t Width = 0;
	int32_t Heigh = 0;
	glfwGetWindowSize(window, &Width, &Heigh);
	
	VkSwapchainCreateInfoKHR SwapchainCreateInfo {VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
	SwapchainCreateInfo.surface = surface;
	SwapchainCreateInfo.minImageCount = 2;
	SwapchainCreateInfo.imageFormat = surfaceFormat.format;
	SwapchainCreateInfo.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
	SwapchainCreateInfo.imageExtent.width = Width;
	SwapchainCreateInfo.imageExtent.height = Heigh;
	SwapchainCreateInfo.imageArrayLayers = 1;
	SwapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	SwapchainCreateInfo.queueFamilyIndexCount = 1;
	SwapchainCreateInfo.pQueueFamilyIndices = &FamilyIndex;
	SwapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	SwapchainCreateInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;

	SwapchainCreateInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
	
	SwapchainCreateInfo.oldSwapchain = nullptr;
	
	VK_CHECK(vkCreateSwapchainKHR(Device, &SwapchainCreateInfo, nullptr, &Swapchain))
}

void EngineInstance::createRenderpass()
{
	// This will be removed once we get rid of renderpasses later.
	// just need this for the purpose of rendering something, but it will be removed
	// soon, since the addition of dynamic rendering and removing renderpasses (and framebuffers)
	VkAttachmentDescription attachment[1] = {};
	attachment[0].format = surfaceFormat.format;
	attachment[0].samples = VK_SAMPLE_COUNT_1_BIT;
	attachment[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachment[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachment[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachment[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachment[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachment[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference ColorAttachments = {0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

	VkSubpassDescription Subpass = {};
	Subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	Subpass.colorAttachmentCount = 1;
	Subpass.pColorAttachments = &ColorAttachments;	
	
	VkRenderPassCreateInfo RenderPassCreateInfo {VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
	RenderPassCreateInfo.attachmentCount = ARRAY_SIZE(attachment);
	RenderPassCreateInfo.pAttachments = attachment;
	RenderPassCreateInfo.subpassCount = 1;
	RenderPassCreateInfo.pSubpasses = &Subpass;
	
	VK_CHECK(vkCreateRenderPass(Device, &RenderPassCreateInfo, nullptr, &RenderPass))
}

void EngineInstance::GetOrCreateSwapchainImages()
{
	swapchainImagesCount = 0;
	vkGetSwapchainImagesKHR(Device, Swapchain, &swapchainImagesCount, SwapchainImages.data());
	SwapchainImages.resize(swapchainImagesCount);
	vkGetSwapchainImagesKHR(Device, Swapchain, &swapchainImagesCount, SwapchainImages.data());
}

void EngineInstance::createImageView()
{
	SwapchainImageViews.resize(swapchainImagesCount);
	for (uint32_t i = 0; i < swapchainImagesCount; ++i)
	{
		VkImageViewCreateInfo ImageViewCreateInfo {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
		ImageViewCreateInfo.image = SwapchainImages[i];
		ImageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		ImageViewCreateInfo.format = surfaceFormat.format;

		ImageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		ImageViewCreateInfo.subresourceRange.levelCount = 1;
		ImageViewCreateInfo.subresourceRange.layerCount = 1;
		
		VK_CHECK(vkCreateImageView(Device, &ImageViewCreateInfo, nullptr, &SwapchainImageViews[i]))		
	}
	
}

void EngineInstance::createFramebuffer()
{
	Framebuffer.resize(swapchainImagesCount);
	for (uint32_t i = 0; i < swapchainImagesCount; ++i)
	{
		VkFramebufferCreateInfo FramebufferCreateInfo {VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
		FramebufferCreateInfo.flags;
		FramebufferCreateInfo.renderPass = RenderPass;
		FramebufferCreateInfo.attachmentCount = 1;
		FramebufferCreateInfo.pAttachments = &SwapchainImageViews[i];
		FramebufferCreateInfo.width = Width;
		FramebufferCreateInfo.height = Height;
		FramebufferCreateInfo.layers = 1;
		
		vkCreateFramebuffer(Device, &FramebufferCreateInfo, nullptr, &Framebuffer[i]);
	}
	
}

void EngineInstance::createMeshPipeline(VkShaderModule vs, VkShaderModule fs)
{
	VkPipelineLayoutCreateInfo PipelineLayoutCreateInfo {VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
	PipelineLayoutCreateInfo.flags;
	PipelineLayoutCreateInfo.setLayoutCount;
	PipelineLayoutCreateInfo.pSetLayouts;
	PipelineLayoutCreateInfo.pushConstantRangeCount;
	PipelineLayoutCreateInfo.pPushConstantRanges;
	
	VK_CHECK(vkCreatePipelineLayout(Device, &PipelineLayoutCreateInfo, nullptr, &MeshPipelineLayout));
	
	VkGraphicsPipelineCreateInfo PipelineCreateIndo = {VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
	
	VkPipelineShaderStageCreateInfo ShaderStages[2] = {};
	ShaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	ShaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;

	ShaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	ShaderStages[0].module = vs;
	ShaderStages[0].pName = "main";
	
	ShaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	ShaderStages[1].module = fs;
	ShaderStages[1].pName = "main";
	// 
	PipelineCreateIndo.stageCount = ARRAY_SIZE(ShaderStages);
	PipelineCreateIndo.pStages = ShaderStages;
	//
	// Empty, the vertex input layout will be dealt with in the shader itself
	// there is no point on doing this, since it will make the vertex declaration
	// way more complicated, and I want to have it explicitly in the shaders
	// that way is way more scalable and easier to read (it should have no impact on perf)
	VkPipelineVertexInputStateCreateInfo VertexInputStateCreateInfo = {VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
	//
	PipelineCreateIndo.pVertexInputState = &VertexInputStateCreateInfo;
	//
	VkPipelineInputAssemblyStateCreateInfo InputAssemblyCreateInfo = {VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
	InputAssemblyCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	//
	PipelineCreateIndo.pInputAssemblyState = &InputAssemblyCreateInfo;
	//
	VkPipelineTessellationStateCreateInfo TessellationStateCreateInfo = {VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO};
	PipelineCreateIndo.pTessellationState = &TessellationStateCreateInfo;
	//
	VkPipelineViewportStateCreateInfo ViewportStateCreateInfo = {VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
	ViewportStateCreateInfo.viewportCount = 1;
	ViewportStateCreateInfo.scissorCount = 1;
	PipelineCreateIndo.pViewportState = &ViewportStateCreateInfo;
	//
	VkPipelineRasterizationStateCreateInfo RasterizationStateCreateInfo = {VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
	RasterizationStateCreateInfo.lineWidth = 1.0f;
	PipelineCreateIndo.pRasterizationState = &RasterizationStateCreateInfo;

	VkPipelineMultisampleStateCreateInfo MultisampleStateCreateInfo = {VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
	MultisampleStateCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	PipelineCreateIndo.pMultisampleState = &MultisampleStateCreateInfo;

	VkPipelineDepthStencilStateCreateInfo DepthStencilStateCreateInfo = {VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
	PipelineCreateIndo.pDepthStencilState = &DepthStencilStateCreateInfo;
	//
	VkPipelineColorBlendAttachmentState ColorBlendAttachmentState = {};
	ColorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	
	VkPipelineColorBlendStateCreateInfo ColorBlendStateCreateInfo = {VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
	ColorBlendStateCreateInfo.attachmentCount = 1;
	ColorBlendStateCreateInfo.pAttachments = &ColorBlendAttachmentState;
	PipelineCreateIndo.pColorBlendState = &ColorBlendStateCreateInfo;

	VkDynamicState DynamicStates[2] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
	VkPipelineDynamicStateCreateInfo DynamicStateCreateInfo = {VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
	DynamicStateCreateInfo.dynamicStateCount = ARRAY_SIZE(DynamicStates);
	DynamicStateCreateInfo.pDynamicStates = DynamicStates;
	PipelineCreateIndo.pDynamicState = &DynamicStateCreateInfo;
	
	PipelineCreateIndo.layout = MeshPipelineLayout;
	
	PipelineCreateIndo.renderPass = RenderPass;
	
	//PipelineCreateIndo.subpass = ;
	
	//PipelineCreateIndo.basePipelineHandle;
	
	//PipelineCreateIndo.basePipelineIndex;
	

	// This should be used later,
	// as usual now, taking shortcucts
	VkPipelineCache pipelineCache = VK_NULL_HANDLE;
	VK_CHECK(vkCreateGraphicsPipelines(Device, pipelineCache, 1, &PipelineCreateIndo ,nullptr, &MeshPipeline))
}

VkShaderModule EngineInstance::loadShader(const char* shaderPath) const
{
	FILE* file = fopen(shaderPath, "rb");
	assert(file != nullptr);
	fseek(file, 0, SEEK_END);

	uint32_t size = ftell(file);
	fseek(file, 0, SEEK_SET);
		
	char* code = new char[size];
	fread(code, size, 1, file);
	fclose(file);

	VkShaderModuleCreateInfo ShaderModuleCreateInfo {VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
	ShaderModuleCreateInfo.codeSize = size;
	ShaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t*>(code);	

	VkShaderModule ShaderModule;
	VK_CHECK(vkCreateShaderModule(Device, &ShaderModuleCreateInfo, nullptr, &ShaderModule))
	return ShaderModule;
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

	Engine.MainLoop();

	glfwDestroyWindow(Engine.window);

	
	return 0;
}
