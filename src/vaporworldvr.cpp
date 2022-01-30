#include <stddef.h>

#include <queue>
#include <variant>

#include <android/native_window_jni.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>

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


namespace VaporWorldVR
{
	class Application;
	class Renderer;
	class Scene;

	class GPUBuffer;
	class VertexBuffer;
	class IndexBuffer;

	struct EGLState
	{
		EGLint versionMajor = -1;
		EGLint versionMinor = -1;
		EGLDisplay display = EGL_NO_DISPLAY;
		EGLConfig config = EGL_NO_CONFIG_KHR;
		EGLSurface surface = EGL_NO_SURFACE;
		EGLSurface dummySurface = EGL_NO_SURFACE;
		EGLContext context = EGL_NO_CONTEXT;
	};


	static EGLBoolean initEGL(EGLState* state, EGLContext shareCtx)
	{
		EGLBoolean status = EGL_TRUE;

		if (state->display != EGL_NO_DISPLAY)
			// Already initialized
			return status;

		// Get the default display
		state->display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
		VW_CHECKF(state->display, "Could not find a suitable EGL display");

		status = eglInitialize(state->display, &state->versionMajor, &state->versionMinor);
		if (!status)
		{
			VW_LOG_ERROR("Failed to initialize EGL display (%#x)", eglGetError());
			return EGL_FALSE;
		}
		VW_LOG_DEBUG("Initialized EGL display '%p' (version %d.%d)", state->display, state->versionMajor,
		             state->versionMinor);

		// We need to initialize the EGL configuration.
		// I don't know exactly why, but we cannot use eglChooseConfig, so we
		// list the required attributes, and enumerate all configs, and manually
		// pick the first config that fits the requirements
		EGLint numConfigs = 0;
		status = eglGetConfigs(state->display, NULL, 0, &numConfigs);
		VW_CHECKF(status, "Failed to query number of available EGL configurations (%#x)", eglGetError());

		EGLConfig* configs = (EGLConfig*)malloc(numConfigs * sizeof(configs[0]));
		status = eglGetConfigs(state->display, configs, numConfigs, &numConfigs);
		VW_CHECKF(status, "Failed to enumerate available EGL configurations(%#x)", eglGetError());

		constexpr GLint configAttrs[] = {EGL_RED_SIZE, 8,
		                                 EGL_BLUE_SIZE, 8,
										 EGL_GREEN_SIZE, 8,
										 EGL_ALPHA_SIZE, 8,
										 EGL_NONE};
		for (EGLint configIdx = 0; configIdx < numConfigs; ++configIdx)
		{
			EGLConfig config = configs[configIdx];
			EGLint flags = 0;

			// Renderable type must be
			status = eglGetConfigAttrib(state->display, config, EGL_RENDERABLE_TYPE, &flags);
			if (!status || (flags & EGL_OPENGL_ES3_BIT_KHR) != EGL_OPENGL_ES3_BIT_KHR)
				// Skip this configuration
				continue;

			status = eglGetConfigAttrib(state->display, config, EGL_SURFACE_TYPE, &flags);
			if (!status || (flags & (EGL_WINDOW_BIT | EGL_PBUFFER_BIT)) != (EGL_WINDOW_BIT | EGL_PBUFFER_BIT))
				// Skip this configuration
				continue;

			GLint attrIdx = 0;
			for (; configAttrs[attrIdx] != EGL_NONE; attrIdx += 2)
			{
				GLint value = 0;
				status = eglGetConfigAttrib(state->display, config, configAttrs[attrIdx], &value);
				if (!status || value != configAttrs[attrIdx + 1])
					break;
			}

			if (configAttrs[attrIdx] == EGL_NONE)
			{
				// This config satifsies all requirements
				state->config = config;
				break;
			}
		}

		if (!state->config)
		{
			VW_LOG_ERROR("No suitable EGL configuration found (%#x)", eglGetError());
			return EGL_FALSE;
		}
		VW_LOG_DEBUG("Picked EGL configuration");

		// Create the context.
		// We need a GLES3.x context
		static constexpr EGLint contextAttrs[] = {EGL_CONTEXT_CLIENT_VERSION, 3,
		                                          EGL_NONE};
		state->context = eglCreateContext(state->display, state->config, shareCtx, contextAttrs);
		if (!state->context)
		{
			VW_LOG_ERROR("Failed to create EGL context (%#x)", eglGetError());
			// TODO: Log configuration?
			return EGL_FALSE;
		}

		// Create dummy window to check everything works correctly
		static constexpr EGLint dummySurfaceAttrs[] = {EGL_WIDTH, 16,
		                                               EGL_HEIGHT, 16,
									                   EGL_NONE};
		state->dummySurface = eglCreatePbufferSurface(state->display, state->config, dummySurfaceAttrs);
		if (state->dummySurface == EGL_NO_SURFACE)
		{
			VW_LOG_ERROR("Failed to create dummy surface (%#x); EGL initialization failed", eglGetError());
			eglDestroyContext(state->display, state->context);
			state->context = EGL_NO_CONTEXT;
			return EGL_FALSE;
		}

		// Attempt to make context and dummy surface current
		status = eglMakeCurrent(state->display, state->dummySurface, state->dummySurface, state->context);
		if (!status)
		{
			VW_LOG_ERROR("Failed to make context current (%#x); EGL initialization failed", glGetError());
			eglDestroySurface(state->display, state->dummySurface);
			eglDestroyContext(state->display, state->context);
			state->context = EGL_NO_CONTEXT;
			return EGL_FALSE;
		}

		VW_LOG_DEBUG("EGL successfully initialized");
		return status;
	}


	static EGLBoolean terminateEGL(EGLState* state)
	{
		EGLBoolean status = true;
		if (state->display == EGL_NO_DISPLAY)
			// Nothing to do
			return status;

		// Unbind context
		status = eglMakeCurrent(state->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
		VW_CHECKF(status, "Failed to unbind context (%#x)", eglGetError());

		if (state->dummySurface)
		{
			status = eglDestroySurface(state->display, state->dummySurface);
			VW_CHECKF(status, "Failed to destroy dummy surface (%#x)", eglGetError());
		}

		if (state->context)
		{
			status = eglDestroyContext(state->display, state->context);
			VW_CHECKF(status, "Failed to destroy EGL context (%#x)", eglGetError());
		}

		// Terminate EGL
		status = eglTerminate(state->display);
		if (!status)
		{
			VW_LOG_ERROR("Failed to terminate EGL (%#x)", eglGetError());
			return status;
		}

		VW_LOG_DEBUG("EGL terminated");
		return status;
	}


	struct RenderCommand : public Message
	{

	};

	struct RenderShutdownCmd : public Message
	{
		bool flushCommands;
	};

	struct RenderBeginFrameCmd : public RenderCommand
	{
		uint64_t frameIdx;
	};

	struct RenderEndFrameCmd : public RenderCommand
	{
		uint64_t frameIdx;
		uint32_t frameFlags;
		uint32_t swapInterval;
		double displayTime;
	};

	struct RenderFlushCmd : public RenderCommand {};

	struct RenderDrawCmd : public RenderCommand
	{
		VertexBuffer* vertexBuffer;
		IndexBuffer* indexBuffer;
		size_t drawOffset;
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
														   RenderFlushCmd>
	{
	public:
		Renderer(ovrMobile* inOvr, EGLState& inShareEglState)
			: java{}
			, ovr{inOvr}
			, eglState{}
			, shareEglState{inShareEglState}
			, state{State_Created}
			, numBuffers{0}
			, useMultiView{false}
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
			if (cmd.flushCommands)
			{
				// Force renderer to flush remaining messages
				// TODO: Flush
			}

			// Set exit flag
			requestExit = true;
		}

		FORCE_INLINE void processMessage(RenderBeginFrameCmd const& cmd)
		{
			VW_LOG_DEBUG_IF(cmd.frameIdx % 1000 == 0, "=== BEGIN FRAME %llu ===", (unsigned long long)cmd.frameIdx);
		}

		FORCE_INLINE void processMessage(RenderEndFrameCmd const& cmd)
		{
			VW_LOG_DEBUG_IF(cmd.frameIdx % 1000 == 0, "=== END FRAME %llu ===", (unsigned long long)cmd.frameIdx);

			// TODO: Remove, just an experiment
			ovrLayerProjection2 clearLayer = vrapi_DefaultLayerBlackProjection2();
			clearLayer.Header.Flags |= VRAPI_FRAME_LAYER_FLAG_INHIBIT_SRGB_FRAMEBUFFER;

			ovrLayerHeader2 const* layers[] = {&clearLayer.Header};

			ovrSubmitFrameDescription2 frameDesc{};
			frameDesc.Flags = cmd.frameFlags;
			frameDesc.FrameIndex = cmd.frameIdx;
			frameDesc.SwapInterval = cmd.swapInterval;
			frameDesc.DisplayTime = cmd.displayTime;
			frameDesc.LayerCount = 1;
			frameDesc.Layers = layers;

			vrapi_SubmitFrame2(ovr, &frameDesc);
		}

		void processMessage(RenderFlushCmd const& cmd)
		{
			// TODO: Flush
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
			bool useMultiView;
			uint8_t numMultiSamples;
			uint8_t textureSwapChainLen;
			uint8_t textureSwapChainIdx;
			ovrTextureSwapChain* colorTextureSwapChain;
			GLuint depthBuffers[VW_TEXTURE_SWAPCHAIN_MAX_LEN];
			GLuint fbos[VW_TEXTURE_SWAPCHAIN_MAX_LEN];
		};

		ovrJava java;
		ovrMobile* ovr;
		EGLState eglState;
		EGLState& shareEglState;
		Framebuffer framebuffers[VRAPI_FRAME_LAYER_EYE_MAX];
		State state;
		uint8_t numBuffers;
		bool useMultiView;
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
		}

		void teardown()
		{
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

			// Get suggested fbo size
			eyeTextureSize.x = vrapi_GetSystemPropertyInt(&java, VRAPI_SYS_PROP_SUGGESTED_EYE_TEXTURE_WIDTH);
			eyeTextureSize.y = vrapi_GetSystemPropertyInt(&java, VRAPI_SYS_PROP_SUGGESTED_EYE_TEXTURE_HEIGHT);
			VW_LOG_DEBUG("Suggested eye texture's size is <%u, %u>", eyeTextureSize.x, eyeTextureSize.y);

			// Multi view means we render to a single 2D texture array, so we only need one framebuffer
			numBuffers = VRAPI_FRAME_LAYER_EYE_MAX;
			eyeTextureType = VRAPI_TEXTURE_TYPE_2D;
			if (useMultiView)
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
				fb.useMultiView = useMultiView;
				fb.colorTextureSwapChain = vrapi_CreateTextureSwapChain3(eyeTextureType, colorFormat, fb.width,
				                                                         fb.height, levels, bufferCount);
				fb.textureSwapChainLen = vrapi_GetTextureSwapChainLength(fb.colorTextureSwapChain);

				// Gen buffers and framebuffer objects
				glGenTextures(fb.textureSwapChainLen, fb.depthBuffers);
				glGenFramebuffers(fb.textureSwapChainLen, fb.fbos);

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

					glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fb.fbos[i]);

					if (fb.useMultiView)
					{
						VW_LOG_ERROR("Multi-view rendering not implemented");
						return -1;
						// Create the depth texture
						glBindTexture(GL_TEXTURE_2D_ARRAY, fb.depthBuffers[i]);
						glTexStorage3D(GL_TEXTURE_2D_ARRAY, levels, GL_DEPTH_COMPONENT, fb.width, fb.height,
						               VRAPI_FRAME_LAYER_EYE_MAX);
						glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

						if (fb.numMultiSamples > 1)
						{

						}
						else
						{

						}
					}
					else
					{
						// Create the depth texture
						glBindTexture(GL_TEXTURE_2D, fb.depthBuffers[i]);
						glTexStorage2D(GL_TEXTURE_2D, levels, GL_DEPTH_COMPONENT, fb.width, fb.height);
						glBindTexture(GL_TEXTURE_2D, 0);

						if (fb.numMultiSamples >  1)
						{
							VW_LOG_ERROR("MSAA rendering not implemented");
							return -1;
						}
						else
						{
							glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
							                       colorTexture, 0);
							glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D,
							                       fb.depthBuffers[i], 0);
						}
					}

					// In debug, check the status of the framebuffer
					status = glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER);
					VW_CHECKF(status == GL_FRAMEBUFFER_COMPLETE, "Incomplete fbo (%s)",
					           GL::getFramebufferStatusString(status));
					if (status == 0)
					{
						VW_LOG_ERROR("Failed to create framebuffer object");
						GL::flushErrors();
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

				// Begin next frame.
				RenderBeginFrameCmd beginFrameCmd{};
				beginFrameCmd.frameIdx = frameCounter;
				renderer->postMessage(beginFrameCmd);

				// TODO: Render scene
				::usleep(1000);

				// End current frame.
				static constexpr uint32_t swapInterval = 1;
				RenderEndFrameCmd endFrameCmd{};
				endFrameCmd.frameIdx = frameCounter;
				endFrameCmd.displayTime = displayTime;
				endFrameCmd.swapInterval = swapInterval;
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
			renderer = new Renderer{ovr, eglState};
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
