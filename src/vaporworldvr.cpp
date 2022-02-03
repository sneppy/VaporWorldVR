#include <stddef.h>

#include <queue>
#include <variant>

#include <android/native_window_jni.h>

#include "VrApi.h"
#include "VrApi_Helpers.h"

#include "logging.h"
#include "vwgl.h"
#include "runnable_thread.h"
#include "event.h"
#include "mutex.h"
#include "math/math.h"
#include "message.h"


#define FORWARD(x) ::std::forward<decltype((x))>((x))

#define VW_TEXTURE_SWAPCHAIN_MAX_LEN 16


static char const shaderVersionString[] = "#version 310 es\n";
static char const shaderCommonTypesString[] =
	"struct ViewInfo"
	"{"
	"	mat4 worldToView;"
	"	mat4 viewToClip;"
	"	mat4 worldToClip;"
	"};";
static char const vertexShaderString[] =
	"in vec3 vertexPosition;"
	"out vec4 vertexColor;"

	"layout(std430, row_major, binding = 0) buffer ViewInfoBuffer"
	"{"
	"	ViewInfo viewInfo;"
	"};"

	"void main()"
	"{"
	"	gl_Position = viewInfo.viewToClip * viewInfo.worldToView * vec4(vertexPosition + vec3(1.f, 1.f, -3.f), 1.f);"
	"	vertexColor = vec4(vertexPosition + vec3(0.5f, 0.5f, 0.5f), 1.f);"
	//"	gl_Position = vec4(vertexPosition * 0.25f, 1.f);"
	"}";
static char const fragmentShaderString[] =
	"in lowp vec4 vertexColor;"
	"out lowp vec4 outColor;"

	"void main()"
	"{"
	"	outColor = vertexColor;"
	"}";


namespace VaporWorldVR
{
	static FORCE_INLINE void logMatrix(float4x4 const& m)
	{
		VW_LOG_DEBUG("[%g, %g, %g, %g,\n"
		             " %g, %g, %g, %g,\n"
		             " %g, %g, %g, %g,\n"
		             " %g, %g, %g, %g,\n",
					 m[0][0], m[0][1], m[0][2], m[0][3],
					 m[1][0], m[1][1], m[1][2], m[1][3],
					 m[2][0], m[2][1], m[2][2], m[2][3],
					 m[3][0], m[3][1], m[3][2], m[3][3]);
	}

	class Application;
	class Renderer;
	class Scene;

	class GPUBuffer;
	class VertexBuffer;
	class IndexBuffer;


	struct RenderCommand : public Message
	{

	};

	struct RenderShutdownCmd : public Message
	{
		//
	};

	struct RenderBeginFrameCmd : public RenderCommand
	{
		uint64_t frameIdx;
	};

	struct RenderEndFrameCmd : public RenderCommand
	{
		ovrMobile* ovr;
		uint64_t frameIdx;
		uint32_t frameFlags;
		uint32_t swapInterval;
		double displayTime;
		ovrTracking2 tracking;
	};

	struct RenderFlushCmd : public RenderCommand {};

	struct RenderDrawCmd : public RenderCommand
	{
		GLenum primitiveType;
		GLuint vertexBuffer;
		GLuint indexBuffer;
		size_t numElements;
		size_t drawOffset;
		// Ignored
		uint32_t numInstances;
	};

	struct RenderDrawIndirectCmd : public RenderCommand
	{
		VertexBuffer* vertexBuffer;
		IndexBuffer* indexBuffer;
		GPUBuffer* drawArgsBuffer;
		size_t argsOffset;
	};


	class Renderer : public Runnable, public MessageTarget<Renderer,
	                                                       RenderShutdownCmd, RenderBeginFrameCmd, RenderEndFrameCmd,
														   RenderFlushCmd, RenderDrawCmd>
	{
	public:
		Renderer(EGLState& inShareEglState)
			: java{}
			, eglState{}
			, shareEglState{inShareEglState}
			, state{State_Created}
			, numBuffers{0}
			, multiViewSupported{false}
			, numMultiSamples{1}
			, eyeTextureType{VRAPI_TEXTURE_TYPE_2D}
			, eyeTextureSize{}
			, requestExit{false}
		{}

		FORCE_INLINE void setJavaInfo(JavaVM* jvm, jobject activity)
		{
			java.Vm = jvm;
			java.ActivityObject = activity;
		}

		void processMessage(RenderShutdownCmd const& cmd)
		{
			// Set exit flag
			requestExit = true;
		}

		FORCE_INLINE void processMessage(RenderBeginFrameCmd const& cmd)
		{
			//
		}

		FORCE_INLINE void processMessage(RenderEndFrameCmd const& cmd)
		{
			// TODO: Remove, just an experiment
			ovrLayerProjection2 layer = vrapi_DefaultLayerProjection2();
			layer.HeadPose = cmd.tracking.HeadPose;
			for (int eyeIdx = 0; eyeIdx < VRAPI_FRAME_LAYER_EYE_MAX; ++eyeIdx)
			{
				layer.Textures[eyeIdx].ColorSwapChain = framebuffers[eyeIdx].colorTextureSwapChain;
				layer.Textures[eyeIdx].SwapChainIndex = framebuffers[eyeIdx].textureSwapChainIdx;
				layer.Textures[eyeIdx].TexCoordsFromTanAngles = ovrMatrix4f_TanAngleMatrixFromProjection(
					&cmd.tracking.Eye[eyeIdx].ProjectionMatrix
				);
			}
			layer.Header.Flags |= VRAPI_FRAME_LAYER_FLAG_CHROMATIC_ABERRATION_CORRECTION;

			for (int eyeIdx = 0; eyeIdx < numBuffers; ++eyeIdx)
			{
				Framebuffer& fb = framebuffers[eyeIdx];
				glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fb.fbos[fb.textureSwapChainIdx]);
				GL_CHECK_ERRORS;

				glEnable(GL_DEPTH_TEST);
				glEnable(GL_CULL_FACE);
				GL_CHECK_ERRORS;

				glViewport(0, 0, eyeTextureSize.x, eyeTextureSize.y);
				glScissor(0, 0, eyeTextureSize.x, eyeTextureSize.y);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
				GL_CHECK_ERRORS;

				glBindBuffer(GL_SHADER_STORAGE_BUFFER, viewInfoBuffer);
				float4x4* viewInfoData = (float4x4*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, 8 * sizeof(float4x4),
																	 GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
				VW_CHECKF(viewInfoData != nullptr, "Failed to map buffer");
				if (viewInfoData)
				{
					::memcpy(viewInfoData, &cmd.tracking.Eye[eyeIdx].ViewMatrix, sizeof(float4x4));
					::memcpy(viewInfoData + 1, &cmd.tracking.Eye[eyeIdx].ProjectionMatrix, sizeof(float4x4));
					viewInfoData[2] = viewInfoData[0].dot(viewInfoData[1]);
					glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
				}
				glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, viewInfoBuffer);
				GL_CHECK_ERRORS;

				glBindVertexArray(vao);
				glDrawElements(GL_TRIANGLES, 6 * 6, GL_UNSIGNED_INT, NULL);
				glBindVertexArray(0);
				GL_CHECK_ERRORS;

				const GLenum depthAttachment[] = {GL_DEPTH_ATTACHMENT};
				glInvalidateFramebuffer(GL_DRAW_FRAMEBUFFER, 1, depthAttachment);
				glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
				fb.textureSwapChainIdx = (fb.textureSwapChainIdx + 1) % fb.textureSwapChainLen;
			}
			GL_CHECK_ERRORS;

			ovrLayerProjection2 clearLayer = vrapi_DefaultLayerBlackProjection2();
			clearLayer.Header.Flags |= VRAPI_FRAME_LAYER_FLAG_INHIBIT_SRGB_FRAMEBUFFER;

			ovrLayerHeader2 const* layers[] = {&layer.Header};

			ovrSubmitFrameDescription2 frameDesc{};
			frameDesc.Flags = cmd.frameFlags;
			frameDesc.FrameIndex = cmd.frameIdx;
			frameDesc.SwapInterval = cmd.swapInterval;
			frameDesc.DisplayTime = cmd.displayTime;
			frameDesc.LayerCount = 1;
			frameDesc.Layers = layers;

			VW_CHECKF(cmd.ovr != nullptr, "Missing Ovr state");
			vrapi_SubmitFrame2(cmd.ovr, &frameDesc);
		}

		void processMessage(RenderFlushCmd const& cmd)
		{
			// TODO: Flush
		}

		void processMessage(RenderDrawCmd const& cmd)
		{
			//
		}

	protected:
		enum State : uint8_t
		{
			State_Created,
			State_Started,
			State_Idle,
			State_Busy,
			State_Stopped
		};

		struct Framebuffer
		{
			uint32_t width;
			uint32_t height;
			uint8_t numMultiSamples;
			uint8_t textureSwapChainLen;
			uint8_t textureSwapChainIdx;
			ovrTextureSwapChain* colorTextureSwapChain;
			GLuint depthBuffers[VW_TEXTURE_SWAPCHAIN_MAX_LEN];
			GLuint fbos[VW_TEXTURE_SWAPCHAIN_MAX_LEN];
		};

		ovrJava java;
		EGLState eglState;
		EGLState& shareEglState;
		Framebuffer framebuffers[VRAPI_FRAME_LAYER_EYE_MAX];
		State state;
		uint8_t numBuffers;
		bool multiViewSupported;
		uint8_t numMultiSamples;
		ovrTextureType eyeTextureType;
		uint2 eyeTextureSize;
		bool requestExit;

		virtual void run() override
		{
			// Set up renderer
			setup();

			for (;;)
			{
				flushMessages(true);

				if (requestExit)
					// Shut down render thread
					break;
			}

			// Tear down renderer
			teardown();
		}

		void setup()
		{
			state = State_Started;
			VW_LOG_DEBUG("Renderer started");

			// Attach current thread
			java.Vm->AttachCurrentThread(&java.Env, nullptr);

			// Initialize EGL
			initEGL(&eglState, shareEglState.context);

			// Create framebuffers
			setupFramebuffers();

			// REMOVE --------------------------
			createProgram();
			setupCube();
		}

		void teardown()
		{
			teardownCube();
			destroyProgram();
			// REMOVE --------------------------

			// Destroy framebuffers
			teardownFramebuffers();

			// Terminate EGL
			terminateEGL(&eglState);

			// Deatch thread
			java.Vm->DetachCurrentThread();

			state = State_Stopped;
			VW_LOG_DEBUG("Renderer stopped");
		}

		int setupFramebuffers()
		{
			int status = GL_NO_ERROR;

			// Load extension functions
#define GL_GET_EXT_FUNCTION_ADDR(type, name) name = (type)eglGetProcAddress(#name);
			GL_EXT_FUNCTIONS(GL_GET_EXT_FUNCTION_ADDR)
#undef GL_GET_EXT_FUNCTION_ADDR

#define GL_CHECK_EXT_FUNCTION(_, name) VW_CHECKF(name != nullptr, "Failed to load GL extension function '%s'", #name);
			GL_EXT_FUNCTIONS(GL_CHECK_EXT_FUNCTION)
#undef GL_CHECK_EXT_FUNCTION

			// Query device capabilities
			// TODO
			multiViewSupported = false;
			numMultiSamples = 4;

			// Get suggested fbo size
			eyeTextureSize.x = vrapi_GetSystemPropertyInt(&java, VRAPI_SYS_PROP_SUGGESTED_EYE_TEXTURE_WIDTH);
			eyeTextureSize.y = vrapi_GetSystemPropertyInt(&java, VRAPI_SYS_PROP_SUGGESTED_EYE_TEXTURE_HEIGHT);
			VW_LOG_DEBUG("Suggested eye texture's size is <%u, %u>", eyeTextureSize.x, eyeTextureSize.y);

			// Multi view means we render to a single 2D texture array, so we only need one framebuffer
			numBuffers = VRAPI_FRAME_LAYER_EYE_MAX;
			eyeTextureType = VRAPI_TEXTURE_TYPE_2D;
			if (multiViewSupported)
			{
				VW_LOG_DEBUG("Using multi-view rendering feature");
				numBuffers = 1;
				eyeTextureType = VRAPI_TEXTURE_TYPE_2D_ARRAY;
			}

			for (int eyeIdx = 0; eyeIdx < numBuffers; ++eyeIdx)
			{
				// Setup individual framebuffers
				constexpr int64_t colorFormat = GL_RGBA8;
				constexpr int levels = 1;
				constexpr int bufferCount = 3;
				Framebuffer& fb = framebuffers[eyeIdx];
				fb.width = eyeTextureSize.x;
				fb.height = eyeTextureSize.y;
				fb.numMultiSamples = numMultiSamples;
				fb.colorTextureSwapChain = vrapi_CreateTextureSwapChain3(eyeTextureType, colorFormat, fb.width,
				                                                         fb.height, levels, bufferCount);
				fb.textureSwapChainLen = vrapi_GetTextureSwapChainLength(fb.colorTextureSwapChain);
				fb.textureSwapChainIdx = 0;

				// Gen buffers and framebuffer objects
				glGenFramebuffers(fb.textureSwapChainLen, fb.fbos);
				GL_CHECK_ERRORS;

				if (multiViewSupported)
				{
					// Multi view rendering extension requires texture arrays
					glGenTextures(fb.textureSwapChainLen, fb.depthBuffers);
				}
				else
				{
					// For normal rendering we can use render buffers
					glGenRenderbuffers(fb.textureSwapChainLen, fb.depthBuffers);
				}

				for (int i = 0; i < fb.textureSwapChainLen; ++i)
				{
					// This returns the i-th color texture in the swapchain
					GLuint colorTexture = vrapi_GetTextureSwapChainHandle(fb.colorTextureSwapChain, i);

					// Create the color texture
					GLuint const textureTarget = eyeTextureType == VRAPI_TEXTURE_TYPE_2D_ARRAY
					                           ? GL_TEXTURE_2D_ARRAY
											   : GL_TEXTURE_2D;
					glBindTexture(textureTarget, colorTexture);
					glTexParameteri(textureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
					glTexParameteri(textureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
					glTexParameteri(textureTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
					glTexParameteri(textureTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
					glBindTexture(textureTarget, 0);
					GL_CHECK_ERRORS;

					glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fb.fbos[i]);
					GL_CHECK_ERRORS;

					if (multiViewSupported)
					{
						VW_LOG_ERROR("Multi-view rendering not implemented");
						return -1;
						// Create the depth texture
						glBindTexture(GL_TEXTURE_2D_ARRAY, fb.depthBuffers[i]);
						glTexStorage3D(GL_TEXTURE_2D_ARRAY, levels, GL_DEPTH_COMPONENT, fb.width, fb.height,
						               VRAPI_FRAME_LAYER_EYE_MAX);
						glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
						GL_CHECK_ERRORS;

						if (fb.numMultiSamples > 1)
						{

						}
						else
						{

						}
					}
					else
					{
						if (fb.numMultiSamples > 1 && glRenderbufferStorageMultisampleEXT &&
						    glFramebufferTexture2DMultisampleEXT)
						{
							VW_LOG_DEBUG("Using MSAAx%u", numMultiSamples);
							glBindRenderbuffer(GL_RENDERBUFFER, fb.depthBuffers[i]);
							glRenderbufferStorageMultisampleEXT(GL_RENDERBUFFER, numMultiSamples, GL_DEPTH_COMPONENT32F,
							                                    fb.width, fb.height);
							glBindRenderbuffer(GL_RENDERBUFFER, 0);

							glFramebufferTexture2DMultisampleEXT(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
							                                     GL_TEXTURE_2D, colorTexture, 0, numMultiSamples);
							glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER,
							                          fb.depthBuffers[i]);
						}
						else
						{
							glBindRenderbuffer(GL_RENDERBUFFER, fb.depthBuffers[i]);
							glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32F, fb.width, fb.height);
							glBindRenderbuffer(GL_RENDERBUFFER, 0);

							glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
							                       colorTexture, 0);
							glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER,
							                          fb.depthBuffers[i]);
						}
						GL_CHECK_ERRORS;
					}

					// In debug, check the status of the framebuffer
					status = glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER);
					VW_CHECKF(status == GL_FRAMEBUFFER_COMPLETE, "Incomplete fbo (%s)",
					          GL::getFramebufferStatusString(status));
					if (status == 0)
					{
						VW_LOG_ERROR("Failed to create framebuffer object");
						return status;
					}

					glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
				}
			}

			VW_LOG_DEBUG("Framebuffers setup completed");
			return status;
		}

		void teardownFramebuffers()
		{
			for (int eyeIdx = 0; eyeIdx < numBuffers; ++eyeIdx)
			{
				// Delete GL resources
				Framebuffer& fb = framebuffers[eyeIdx];
				glDeleteFramebuffers(fb.textureSwapChainLen, fb.fbos);
				glDeleteTextures(fb.textureSwapChainLen, fb.depthBuffers);
			}

			VW_LOG_DEBUG("Framebuffers teardown completed");
		}

		// REMOVE ------------
		GLuint program;
		GLuint vao;
		GLuint vertexBuffer;
		GLuint indexBuffer;
		GLuint viewInfoBuffer;
		GLuint viewInfoBufferBinding;

		void createProgram()
		{
			GLuint vertexShader, fragmentShader;
			GLint status;

			// Create vertex shader
			static char const* vertexShaderSource[] = {shaderVersionString, shaderCommonTypesString,
			                                           vertexShaderString};
			vertexShader = glCreateShader(GL_VERTEX_SHADER);
			glShaderSource(vertexShader, 3, vertexShaderSource, NULL);
			glCompileShader(vertexShader);
			glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &status);
			if (status != GL_TRUE)
			{
				VW_LOG_ERROR("Failed to compile vertex shader:\n%s", GL::getShaderLog(vertexShader).c_str());
				glDeleteShader(vertexShader);
				return;
			}
			GL_CHECK_ERRORS;

			// Create fragment shader
			static char const* fragmentShaderSource[] = {shaderVersionString, fragmentShaderString};
			fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
			glShaderSource(fragmentShader, 2, fragmentShaderSource, NULL);
			glCompileShader(fragmentShader);
			glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &status);
			if (status != GL_TRUE)
			{
				GL_CHECK_ERRORS;
				VW_LOG_ERROR("Failed to compile fragment shader:\n%s", GL::getShaderLog(fragmentShader).c_str());
				glDeleteShader(fragmentShader);
				glDeleteShader(vertexShader);
				return;
			}
			GL_CHECK_ERRORS;

			// Link program
			program = glCreateProgram();
			glAttachShader(program, vertexShader);
			glAttachShader(program, fragmentShader);
			glLinkProgram(program);
			glGetProgramiv(program, GL_LINK_STATUS, &status);
			if (status != GL_TRUE)
			{
				VW_LOG_ERROR("Failed to link program (%s)", GL::getErrorString());
				glDeleteProgram(program);
				glDeleteShader(fragmentShader);
				glDeleteShader(vertexShader);
				return;
			}
			GL_CHECK_ERRORS;

			// Get view buffer location
			glUseProgram(program);
			GL_CHECK_ERRORS;

			// Create view buffer
			glGenBuffers(1, &viewInfoBuffer);
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, viewInfoBuffer);
			glBufferData(GL_SHADER_STORAGE_BUFFER, 8 * sizeof(float4x4), NULL, GL_DYNAMIC_DRAW);

			glUseProgram(program);
		}

		void destroyProgram()
		{
			glUseProgram(0);
			glDeleteProgram(program);
			GL_CHECK_ERRORS;
		}

		void setupCube()
		{
			static float3 vertexData[] = {{-0.5f, -0.5f, -0.5f}, {0.5f, -0.5f, -0.5f}, {0.5f, 0.5f, -0.5f},
			                              {-0.5f, 0.5f, -0.5f}, {-0.5f, -0.5f, 0.5f}, {0.5f, -0.5f, 0.5f},
										  {0.5f, 0.5f, 0.5f}, {-0.5f, 0.5f, 0.5f}};
			static uint32_t triangleData[] = {1, 5, 6, 6, 2, 1,
			                                  3, 2, 6, 6, 7, 3,
											  5, 4, 7, 7, 6, 5,
											  4, 0, 3, 3, 7, 4,
											  4, 5, 1, 1, 0, 4,
											  0, 1, 2, 2, 3, 0};

			// Create vertex array object
			glGenVertexArrays(1, &vao);
			glBindVertexArray(vao);

			// Generate graphics buffers
			glGenBuffers(2, &vertexBuffer);

			// Fill vertex buffer
			glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
			glBufferData(GL_ARRAY_BUFFER, sizeof(vertexData), vertexData, GL_STATIC_DRAW);
			GL_CHECK_ERRORS;

			// Fill index buffer
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(triangleData), triangleData, GL_STATIC_DRAW);
			GL_CHECK_ERRORS;

			// Setup vertex attributes
			glEnableVertexAttribArray(0);
			glVertexAttribFormat(0, 3, GL_FLOAT, GL_FALSE, 0);
			GL_CHECK_ERRORS;

			// Bind vertex buffer
			glBindVertexBuffer(0, vertexBuffer, 0, sizeof(vertexData[0]));
			glVertexAttribBinding(0, 0);
			GL_CHECK_ERRORS;

			glBindVertexArray(0);
		}

		void teardownCube()
		{
			glDeleteBuffers(2, &vertexBuffer);
			glDeleteVertexArrays(1, &vao);
		}
	};


	struct ApplicationEvent : public Message
	{
		enum Type
		{
			Type_Created,
			Type_Resumed,
			Type_Paused,
			Type_Destroyed,
			Type_SurfaceCreated,
			Type_SurfaceDestroyed
		};

		Type type;
		ANativeWindow* nativeWindow;

		FORCE_INLINE ApplicationEvent(Type inType, ANativeWindow* inNativeWindow = nullptr)
			: type{inType}
			, nativeWindow{inNativeWindow}
		{
			//
		}
	};


	class Application : public MessageTarget<Application, ApplicationEvent>, public Runnable
	{
	public:
		Application()
			: nativeWindow{nullptr}
			, java{}
			, ovr{nullptr}
			, eglState{}
			, frameCounter{0}
			, requestExit{false}
			, resumed{false}
		{}

		FORCE_INLINE ANativeWindow const* getNativeWindow() const
		{
			return nativeWindow;
		}

		FORCE_INLINE ovrJava const& getJavaInfo() const
		{
			return java;
		}

		FORCE_INLINE void setJavaInfo(JavaVM* jvm, jobject activity)
		{
			java.Vm = jvm;
			java.ActivityObject = activity;
		}

		virtual void run() override
		{
			// Set up application
			setup();

			while (!requestExit)
			{
				// Called once per frame to process application events.
				// If we are not in VR mode, block on waiting for new messages
				bool const blocking = ovr == nullptr;
				flushMessages(blocking);

				// Update the state of the application
				updateApplicationState();

				if (!ovr)
					// Skip if not in VR mode yet
					continue;

				// Increment frame counter, before predicting the display time
				frameCounter++;

				// Predict display time and HMD pose
				displayTime = vrapi_GetPredictedDisplayTime(ovr, frameCounter);
				tracking = vrapi_GetPredictedTracking2(ovr, displayTime);

				// Begin next frame.
				RenderBeginFrameCmd beginFrameCmd{};
				beginFrameCmd.frameIdx = frameCounter;
				renderer->postMessage(beginFrameCmd);

				// TODO: Render scene

				// End current frame.
				static constexpr uint32_t swapInterval = 1;
				RenderEndFrameCmd endFrameCmd{};
				endFrameCmd.ovr = ovr;
				endFrameCmd.frameIdx = frameCounter;
				endFrameCmd.displayTime = displayTime;
				endFrameCmd.swapInterval = swapInterval;
				endFrameCmd.tracking = tracking;
				renderer->postMessage(endFrameCmd, MessageWait_Received);
			}

			// Tear down application
			teardown();
		}

		void processMessage(ApplicationEvent const& msg)
		{
			switch (msg.type)
			{
			case ApplicationEvent::Type_Created:
				VW_LOG_DEBUG("Application created");
				break;

			case ApplicationEvent::Type_Resumed:
			{
				VW_LOG_DEBUG("Application resumed");
				resumed = true;
			}
			break;

			case ApplicationEvent::Type_Paused:
			{
				VW_LOG_DEBUG("Application paused");
				resumed = false;
			}
			break;

			case ApplicationEvent::Type_Destroyed:
			{
				VW_LOG_DEBUG("Application shutdown requested");
				requestExit = true;
			}
			break;

			case ApplicationEvent::Type_SurfaceCreated:
			{
				VW_LOG_DEBUG("Application surface created, new ANativeWindow '%p'", msg.nativeWindow);
				nativeWindow = msg.nativeWindow;
			}
			break;

			case ApplicationEvent::Type_SurfaceDestroyed:
			{
				VW_LOG_DEBUG("Application surface destroyed");
				nativeWindow = nullptr;
			}
			break;

			default:
				VW_LOG_WARN("Unkown event type '%d'", msg.type);
				break;
			}
		}

	protected:
		ANativeWindow* nativeWindow;
		ovrJava java;
		ovrMobile* ovr;
		EGLState eglState;
		Renderer* renderer;
		uint64_t frameCounter;
		double displayTime;
		ovrTracking2 tracking;
		bool requestExit;
		bool resumed;

		void setup()
		{
			// Setup Java env
			VW_ASSERT(java.Vm)
			VW_ASSERT(java.ActivityObject)
			java.Vm->AttachCurrentThread(&java.Env, nullptr);

			// Init Oculus API
			ovrInitParms initParams = vrapi_DefaultInitParms(&java);
			auto status = vrapi_Initialize(&initParams);
			if (status != VRAPI_INITIALIZE_SUCCESS)
			{
				VW_LOG_ERROR("Failed to initialize vrApi");
				exit(0);
			}

			// Create the EGL context
			initEGL(&eglState, nullptr);

			// Create the render thread
			renderer = new Renderer{eglState};
			renderer->setJavaInfo(java.Vm, java.ActivityObject);

			auto* renderThread = createRunnableThread(renderer);
			renderThread->setName("VW_RenderThread");
			renderThread->start();

			VW_LOG_DEBUG("Application setup completed");
		}

		void teardown()
		{
			// Stop the render thread
			renderer->postMessage<RenderShutdownCmd>({}, MessageWait_Processed);
			auto* renderThread = renderer->getThread();
			renderThread->join();
			destroyRunnableThread(renderThread);

			delete renderer;

			// Destroy the EGL context
			terminateEGL(&eglState);

			// Shutdown VR API and detach thread
			vrapi_Shutdown();
			java.Vm->DetachCurrentThread();

			VW_LOG_DEBUG("Application teardown completed");
		}

		void updateApplicationState()
		{
			if (resumed && nativeWindow)
			{
				if (!ovr)
				{
					// Prepare to enter VR mode
					ovrModeParms params = vrapi_DefaultModeParms(&java);
					params.Flags |= VRAPI_MODE_FLAG_RESET_WINDOW_FULLSCREEN;
					params.Flags |= VRAPI_MODE_FLAG_NATIVE_WINDOW;
					params.Display = (size_t)eglState.display;
					params.WindowSurface = (size_t)nativeWindow;
					params.ShareContext = (size_t)eglState.context;

					// Enter VR mode
					VW_LOG_DEBUG("Entering VR mode");
					ovr = vrapi_EnterVrMode(&params);
					if (!ovr)
					{
						// Invalid native window
						VW_LOG_ERROR("Failed to enter VR mode");
						nativeWindow = nullptr;
					}
				}
			}
			else if (ovr)
			{
				// Leave VR mode
				VW_LOG_DEBUG("Leaving VR mode");
				vrapi_LeaveVrMode(ovr);
				ovr = nullptr;
			}
		}
	};
} // namespace VaporWorldVR


static void* createApplication(JNIEnv* env, jobject activity)
{
	using namespace VaporWorldVR;

	// Get the JVM instance
	JavaVM* jvm = nullptr;
	env->GetJavaVM(&jvm);

	auto* app = new Application;
	app->setJavaInfo(jvm, env->NewGlobalRef(activity));

	auto* appThread = createRunnableThread(app);
	appThread->setName("VW_AppThread");
	appThread->start();
	app->postMessage<ApplicationEvent>({ApplicationEvent::Type_Created}, MessageWait_Processed);
	return reinterpret_cast<void*>(app);
}

static void resumeApplication(void* handle)
{
	using namespace VaporWorldVR;

	auto* app = reinterpret_cast<Application*>(handle);
	app->postMessage<ApplicationEvent>({ApplicationEvent::Type_Resumed}, MessageWait_Processed);
}

static void pauseApplication(void* handle)
{
	using namespace VaporWorldVR;

	auto* app = reinterpret_cast<Application*>(handle);
	app->postMessage<ApplicationEvent>({ApplicationEvent::Type_Paused}, MessageWait_Processed);
}

static void destroyApplication(JNIEnv* env, void* handle)
{
	using namespace VaporWorldVR;

	auto* app = reinterpret_cast<Application*>(handle);
	app->postMessage<ApplicationEvent>({ApplicationEvent::Type_Destroyed}, MessageWait_Processed);
	auto* appThread = app->getThread();
	appThread->join();
	destroyRunnableThread(appThread);

	delete app;
}

static ANativeWindow* setApplicationWindow(void* handle, ANativeWindow* newNativeWindow)
{
	using namespace VaporWorldVR;

	auto* app = reinterpret_cast<Application*>(handle);
	if (app->getNativeWindow() == newNativeWindow)
		// Same window or both null, nothing to do
		return newNativeWindow;

	ANativeWindow* oldWindow = nullptr;
	if (app->getNativeWindow())
	{
		// Destroy the current window
		oldWindow = const_cast<ANativeWindow*>(app->getNativeWindow());
		app->postMessage<ApplicationEvent>({ApplicationEvent::Type_SurfaceDestroyed}, MessageWait_Processed);
	}

	if (newNativeWindow)
	{
		// Create the new window
		app->postMessage<ApplicationEvent>({ApplicationEvent::Type_SurfaceCreated, newNativeWindow},
		                                   MessageWait_Processed);
	}

	return oldWindow;
}


// =============================
// Activity lifecycle callbacks.
// =============================
#ifdef __cplusplus
extern "C" {
#endif
/* Called after the application is created. */
JNIEXPORT jlong JNICALL Java_com_vaporworldvr_VaporWorldVRWrapper_onCreate(JNIEnv* env, jobject obj, jobject activity)
{
	void* app = createApplication(env, activity);
	return (jlong)app;
}

/* Called after the application is started. */
JNIEXPORT void JNICALL Java_com_vaporworldvr_VaporWorldVRWrapper_onStart(JNIEnv* env, jobject obj, jlong handle) {}

/* Called after the application is resumed. */
JNIEXPORT void JNICALL Java_com_vaporworldvr_VaporWorldVRWrapper_onResume(JNIEnv* env, jobject obj, jlong handle)
{
	resumeApplication((void*)handle);
}

/* Called before the application is paused. */
JNIEXPORT void JNICALL Java_com_vaporworldvr_VaporWorldVRWrapper_onPause(JNIEnv* env, jobject obj, jlong handle)
{
	pauseApplication((void*)handle);
}

/* Called before the application is stopped. */
JNIEXPORT void JNICALL Java_com_vaporworldvr_VaporWorldVRWrapper_onStop(JNIEnv* env, jobject obj, jlong handle) {}

/* Called before the application is destroyed. */
JNIEXPORT void JNICALL Java_com_vaporworldvr_VaporWorldVRWrapper_onDestroy(JNIEnv* env, jobject obj, jlong handle)
{
	void* app = (void*)handle;
	destroyApplication(env, app);
}

/* Called after the surface is created. */
JNIEXPORT void JNICALL Java_com_vaporworldvr_VaporWorldVRWrapper_onSurfaceCreated(JNIEnv* env, jobject obj,
                                                                                  jlong handle, jobject surface)
{
	// Get a new native window from surface
	ANativeWindow* nativeWindow = ANativeWindow_fromSurface(env, surface);
	ANativeWindow* oldNativeWindow = setApplicationWindow((void*)handle, nativeWindow);
	VW_CHECK(oldNativeWindow == NULL);
}

/* Called after the surface changes. */
JNIEXPORT void JNICALL Java_com_vaporworldvr_VaporWorldVRWrapper_onSurfaceChanged(JNIEnv* env, jobject obj,
                                                                                  jlong handle, jobject surface)
{
	// Get a new native window from surface
	ANativeWindow* nativeWindow = ANativeWindow_fromSurface(env, surface);
	ANativeWindow* oldNativeWindow = setApplicationWindow((void*)handle, nativeWindow);

	if (oldNativeWindow)
	{
		// Release window
		ANativeWindow_release(oldNativeWindow);
	}
}

/* Called after the surface is destroyed. */
JNIEXPORT void JNICALL Java_com_vaporworldvr_VaporWorldVRWrapper_onSurfaceDestroyed(JNIEnv* env, jobject obj,
                                                                                  jlong handle)
{
	ANativeWindow* oldNativeWindow = setApplicationWindow((void*)handle, NULL);
	VW_ASSERT(oldNativeWindow != NULL);
	ANativeWindow_release(oldNativeWindow);
}
#ifdef __cplusplus
}
#endif
