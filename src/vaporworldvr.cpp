#include <stddef.h>
#include <android/native_window_jni.h>

#include "logging.h"
#include "runnable_thread.h"
#include "math/math.h"


namespace VaporWorldVR
{
	class Application : public Runnable
	{
	public:
		virtual void run() override
		{
			auto* thread = getThread();
			VW_LOG_DEBUG("Threads are working!");
			VW_LOG_DEBUG("This is thread '%s' with tid=%d", thread->getName().c_str(), thread->getId());

			float3 position = float3::zero;
			float3 linearVelocity = float3::zero;
			float3 axis = float3::zero;
			float angle = 0.f;
			quat rotation = {float3::up, 1.4f};
			float3 angularVelocity = float3::up;
			rotation.getAxisAndAngle(axis, angle);
			VW_LOG_DEBUG("Player position: <%g, %g, %g>", position.x, position.y, position.z);
			VW_LOG_DEBUG("Player rotation: %g @ <%g, %g, %g>", angle, axis.x, axis.y, axis.z);
			VW_LOG_DEBUG("Player distance from origin: %g", position.getSize());

			VW_LOG_DEBUG("Simulating 1 minute, 30 steps per min");
			VW_LOG_DEBUG("=====================================");
			for (int i = 0; i < 30 * 60; ++i)
			{
				float dt = 1.f / 30.f;
				linearVelocity += float3::down * 9.81f * dt;
				position += linearVelocity * dt;
				rotation = quat{angularVelocity * dt} * rotation;

				rotation.getAxisAndAngle(axis, angle);
				VW_LOG_DEBUG("// Step %d", i);
				VW_LOG_DEBUG("Player position: <%g, %g, %g>", position.x, position.y, position.z);
				VW_LOG_DEBUG("Player rotation: %g @ <%g, %g, %g>", angle, axis.x, axis.y, axis.z);
				VW_LOG_DEBUG("Player distance from origin: %g", position.getSize());
			}
		}
	};
} // namespace VaporWorldVR


void test()
{
	using namespace VaporWorldVR;
	auto* app = new Application;
	auto* appThread = createRunnableThread(app);
	appThread->start();
	appThread->join();
	delete app;
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
	test();
	return static_cast<size_t>(0);
}

/* Called after the application is started. */
JNIEXPORT void JNICALL Java_com_vaporworldvr_VaporWorldVRWrapper_onStart(JNIEnv* env, jobject obj, jlong handle)
{
	// TODO
}

/* Called after the application is resumed. */
JNIEXPORT void JNICALL Java_com_vaporworldvr_VaporWorldVRWrapper_onResume(JNIEnv* env, jobject obj, jlong handle)
{
	// TODO
}

/* Called before the application is paused. */
JNIEXPORT void JNICALL Java_com_vaporworldvr_VaporWorldVRWrapper_onPause(JNIEnv* env, jobject obj, jlong handle)
{
	// TODO
}

/* Called before the application is stopped. */
JNIEXPORT void JNICALL Java_com_vaporworldvr_VaporWorldVRWrapper_onStop(JNIEnv* env, jobject obj, jlong handle)
{
	// TODO
}

/* Called before the application is destroyed. */
JNIEXPORT void JNICALL Java_com_vaporworldvr_VaporWorldVRWrapper_onDestroy(JNIEnv* env, jobject obj, jlong handle)
{
	// TODO
}

/* Called after the surface is created. */
JNIEXPORT void JNICALL Java_com_vaporworldvr_VaporWorldVRWrapper_onSurfaceCreated(JNIEnv* env, jobject obj,
                                                                                  jlong handle, jobject surface)
{
	// TODO
}

/* Called after the surface changes. */
JNIEXPORT void JNICALL Java_com_vaporworldvr_VaporWorldVRWrapper_onSurfaceChanged(JNIEnv* env, jobject obj,
                                                                                  jlong handle, jobject surface)
{
	// TODO
}

/* Called after the surface is destroyed. */
JNIEXPORT void JNICALL Java_com_vaporworldvr_VaporWorldVRWrapper_onSurfaceDestroyed(JNIEnv* env, jobject obj,
                                                                                  jlong handle)
{
	// TODO
}
#ifdef __cplusplus
}
#endif
