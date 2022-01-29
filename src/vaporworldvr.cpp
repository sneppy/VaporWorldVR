#include <stddef.h>

#include <queue>
#include <variant>

#include <android/native_window_jni.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>

#include "VrApi.h"
#include "VrApi_Helpers.h"

#include "logging.h"
#include "runnable_thread.h"
#include "event.h"
#include "mutex.h"
#include "math/math.h"
#include "message.h"


#define FORWARD(x) ::std::forward<decltype((x))>((x))


namespace VaporWorldVR
{
	struct EGLState
	{
		EGLint versionMajor = -1;
		EGLint versionMinor = -1;
		EGLDisplay display = EGL_NO_DISPLAY;
		EGLConfig config = EGL_NO_CONFIG_KHR;
		EGLSurface surface = EGL_NO_SURFACE;
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

		// Create the context
		state->context = eglCreateContext(state->display, state->config, shareCtx, NULL);
		if (!state->context)
		{
			VW_LOG_ERROR("Failed to create EGL context (%#x)", eglGetError());
			// TODO: Log configuration?
			return EGL_FALSE;
		}

		// Create dummy window to check everything works correctly
		EGLint dummySurfaceAttrs[] = {EGL_WIDTH, 16,
		                              EGL_HEIGHT, 16,
									  EGL_NONE};
		EGLSurface dummySurface = eglCreatePbufferSurface(state->display, state->config, dummySurfaceAttrs);
		if (dummySurface == EGL_NO_SURFACE)
		{
			VW_LOG_ERROR("Failed to create dummy surface (%#x); EGL initialization failed", eglGetError());
			eglDestroyContext(state->display, state->context);
			state->context = EGL_NO_CONTEXT;
			return EGL_FALSE;
		}

		// Attempt to make context and dummy surface current
		status = eglMakeCurrent(state->display, dummySurface, dummySurface, state->context);
		if (!status)
		{
			VW_LOG_ERROR("Failed to make context current (%#x); EGL initialization failed", glGetError());
			eglDestroySurface(state->display, dummySurface);
			eglDestroyContext(state->display, state->context);
			state->context = EGL_NO_CONTEXT;
			return EGL_FALSE;
		}

		VW_LOG_DEBUG("EGL successfully initialized");
		eglMakeCurrent(state->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
		eglDestroySurface(state->display, dummySurface);
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
		Application(JNIEnv* env, jobject activity)
			: nativeWindow{nullptr}
			, java{.Env = env}
			, ovr{nullptr}
			, eglState{}
			, frameCounter{0}
			, requestExit{false}
			, resumed{false}
		{
			// Get java env properties
			env->GetJavaVM(&java.Vm);
			java.ActivityObject = env->NewGlobalRef(activity);
		}

		~Application()
		{
			java.Env->DeleteGlobalRef(java.ActivityObject);
			java.Vm = nullptr;
			java.Env = nullptr;
		}

		FORCE_INLINE ANativeWindow const* getNativeWindow() const
		{
			return nativeWindow;
		}

		virtual void setup() override
		{
			// Setup Java env
			VW_ASSERT(java.Vm)
			VW_ASSERT(java.Env)
			VW_ASSERT(java.ActivityObject)
			java.Vm->AttachCurrentThread(&java.Env, NULL);

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
		}

		virtual void teardown() override
		{
			// Destroy the EGL context
			terminateEGL(&eglState);

			// Shutdown VR API and detach thread
			vrapi_Shutdown();
			java.Vm->DetachCurrentThread();
		}

		virtual void run() override
		{

			while (!requestExit)
			{
				flushMessages(true);

				if (!ovr)
				{
					// Skip if not in VR mode yet
					continue;
				}
			}
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
				VW_LOG_DEBUG("Application destroyed");
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
				VW_LOG_WARN("Unkown event type (%d)", msg.type);
				break;
			}

			updateApplicationState();
		}

	protected:
		ANativeWindow* nativeWindow;
		ovrJava java;
		ovrMobile* ovr;
		EGLState eglState;
		uint64_t frameCounter;
		bool requestExit;
		bool resumed;

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

	auto* app = new Application{env, activity};

	auto* appThread = createRunnableThread(app);
	appThread->setName("VaporWorldVR::MainThread");
	appThread->start();
	app->postMessage<ApplicationEvent>({ApplicationEvent::Type_Created});
	return reinterpret_cast<void*>(app);
}

static void resumeApplication(void* handle)
{
	using namespace VaporWorldVR;

	auto* app = reinterpret_cast<Application*>(handle);
	app->postMessage<ApplicationEvent>({ApplicationEvent::Type_Resumed});
}

static void pauseApplication(void* handle)
{
	using namespace VaporWorldVR;

	auto* app = reinterpret_cast<Application*>(handle);
	app->postMessage<ApplicationEvent>({ApplicationEvent::Type_Paused});
}

static void destroyApplication(void* handle)
{
	using namespace VaporWorldVR;

	auto* app = reinterpret_cast<Application*>(handle);
	app->postMessage<ApplicationEvent>({ApplicationEvent::Type_Destroyed});
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
		constexpr bool ackRvcd = true;
		app->postMessage<ApplicationEvent>({ApplicationEvent::Type_SurfaceDestroyed}/* , ackRcvd */);
		oldWindow = const_cast<ANativeWindow*>(app->getNativeWindow());
	}

	if (newNativeWindow)
	{
		// Create the new window
		constexpr bool ackRvcd = true;
		app->postMessage<ApplicationEvent>({ApplicationEvent::Type_SurfaceCreated, newNativeWindow}/* , ackRcvd */);
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
	destroyApplication(app);
}

/* Called after the surface is created. */
JNIEXPORT void JNICALL Java_com_vaporworldvr_VaporWorldVRWrapper_onSurfaceCreated(JNIEnv* env, jobject obj,
                                                                                  jlong handle, jobject surface)
{
	// Get a new native window from surface
	ANativeWindow* nativeWindow = ANativeWindow_fromSurface(env, surface);
	ANativeWindow* oldNativeWindow = setApplicationWindow((void*)handle, nativeWindow);
	VW_CHECK(oldNativeWindow != nativeWindow);
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
	VW_ASSERT(oldNativeWindow != nullptr);
	ANativeWindow_release(oldNativeWindow);
}
#ifdef __cplusplus
}
#endif
