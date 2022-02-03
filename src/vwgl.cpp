#include "vwgl.h"


#define DEFINE_GL_EXT_FUNCTIONS(type, name) type name = 0;
GL_EXT_FUNCTIONS(DEFINE_GL_EXT_FUNCTIONS)
#undef DEFINE_GL_EXT_FUNCTONS


namespace VaporWorldVR
{
	::std::string GL::getShaderLog(GLuint shader)
	{
		// Query log size (includes the NULL terminator)
		GLsizei logSize = 0;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logSize);
		if (logSize == 0)
			return "";

		// Get log
		::std::string log;
		log.resize(logSize - 1);
		glGetShaderInfoLog(shader, logSize, &logSize, &log[0]);
		VW_CHECK(logSize == log.size());

		// Strip trailing newlines
		log.erase(logSize - 1);

		return log;
	}


	EGLBoolean initEGL(EGLState* state, EGLContext shareCtx)
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


	EGLBoolean terminateEGL(EGLState* state)
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
} // namespace VaporWorldVR
