#pragma once

#include <string>

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl32.h>
#include <GLES3/gl3ext.h>

#include "core_types.h"
#include "logging.h"


#ifndef GL_EXT_multisampled_render_to_texture
typedef void(GL_APIENTRY* PFNGLRENDERBUFFERSTORAGEMULTISAMPLEEXTPROC)(GLenum target,
                                                                      GLsizei samples,
                                                                      GLenum internalformat,
                                                                      GLsizei width,
                                                                      GLsizei height);
typedef void(GL_APIENTRY* PFNGLFRAMEBUFFERTEXTURE2DMULTISAMPLEEXTPROC)(GLenum target,
                                                                       GLenum attachment,
                                                                       GLenum textarget,
                                                                       GLuint texture,
                                                                       GLint level,
                                                                       GLsizei samples);
#endif


#ifndef GL_OVR_multiview
/// static const int GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_NUM_VIEWS_OVR       = 0x9630;
/// static const int GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_BASE_VIEW_INDEX_OVR = 0x9632;
/// static const int GL_MAX_VIEWS_OVR                                      = 0x9631;
typedef void(GL_APIENTRY* PFNGLFRAMEBUFFERTEXTUREMULTIVIEWOVRPROC)(GLenum target,
                                                                   GLenum attachment,
                                                                   GLuint texture,
                                                                   GLint level,
                                                                   GLint baseViewIndex,
                                                                   GLsizei numViews);
#endif


#ifndef GL_OVR_multiview_multisampled_render_to_texture
typedef void(GL_APIENTRY* PFNGLFRAMEBUFFERTEXTUREMULTISAMPLEMULTIVIEWOVRPROC)(GLenum target,
                                                                              GLenum attachment,
                                                                              GLuint texture,
                                                                              GLint level,
                                                                              GLsizei samples,
                                                                              GLint baseViewIndex,
                                                                              GLsizei numViews);
#endif


#define GL_EXT_FUNCTIONS(func) func(PFNGLRENDERBUFFERSTORAGEMULTISAMPLEEXTPROC, glRenderbufferStorageMultisampleEXT)\
                               func(PFNGLFRAMEBUFFERTEXTURE2DMULTISAMPLEEXTPROC, glFramebufferTexture2DMultisampleEXT)\
						       func(PFNGLFRAMEBUFFERTEXTUREMULTIVIEWOVRPROC, glFramebufferTextureMultiviewOVR)\
						       func(PFNGLFRAMEBUFFERTEXTUREMULTISAMPLEMULTIVIEWOVRPROC,\
						            glFramebufferTextureMultisampleMultiviewOVR)


#define DECLARE_GL_EXT_FUNCTION(type, name) extern type name;
GL_EXT_FUNCTIONS(DECLARE_GL_EXT_FUNCTION)
#undef DECLARE_GL_EXT_FUNCTION


#if VW_BUILD_DEBUG
# define GL_CHECK_ERRORS GL::flushErrors(__FILE__, __LINE__)
#else
# define GL_CHECK_ERRORS
#endif


namespace VaporWorldVR::GL
{
	/**
	 * @brief Returns the string for the given GLES error.
	 *
	 * @param err The code the error
	 * @return String of the error
	 */
	FORCE_INLINE char const* const getErrorString(GLenum err)
	{
		#define SWITCH_CASE(err) case err: return #err;
		switch (err)
		{
		SWITCH_CASE(GL_NO_ERROR)
		SWITCH_CASE(GL_INVALID_ENUM)
		SWITCH_CASE(GL_INVALID_VALUE)
		SWITCH_CASE(GL_INVALID_OPERATION)
		SWITCH_CASE(GL_INVALID_FRAMEBUFFER_OPERATION)
		SWITCH_CASE(GL_OUT_OF_MEMORY)
		default: return "N/A";
		}
		#undef SWITCH_CASE
	}

	/**
	 * @brief Returns the string of the most recent GLES error.
	 */
	FORCE_INLINE char const* const getErrorString()
	{
		return getErrorString(glGetError());
	}

	/**
	 * @brief Prints all pending GLES errors.
	 */
	FORCE_INLINE void flushErrors(char const* const filename, int lineno)
	{
		GLenum err = GL_NO_ERROR;
		while ((err = glGetError()) != GL_NO_ERROR)
		{
			VW_LOG_ERROR("%s:%d: Encountered GLES error #%u (%s)", filename, lineno, err, getErrorString(err));
		}
	}

	/**
	 * @brief Returns the string of the status of the framebuffer object.
	 *
	 * @param status The status of the fbo
	 * @return The string of the status
	 */
	FORCE_INLINE char const* const getFramebufferStatusString(GLenum status)
	{
		#define SWITCH_CASE(err) case err: return #err;
		switch (status)
		{
		SWITCH_CASE(GL_FRAMEBUFFER_COMPLETE)
		SWITCH_CASE(GL_FRAMEBUFFER_UNDEFINED)
		SWITCH_CASE(GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT)
		SWITCH_CASE(GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT)
		SWITCH_CASE(GL_FRAMEBUFFER_UNSUPPORTED)
		SWITCH_CASE(GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE)
		default: return "N/A";
		}
		#undef SWITCH_CASE
	}

	/**
	 * @brief Returns the log of the shader compilation.
	 *
	 * @param shader The name of the shader resource
	 * @return String with shader log
	 */
	::std::string getShaderLog(GLuint shader);
} // namespace VaporWorldVR::GL


namespace VaporWorldVR
{
	/**
	 * @brief Describes the current EGL state.
	 */
	struct EGLState
	{
		/* EGL major version. */
		EGLint versionMajor = -1;

		/* EGL minor version. */
		EGLint versionMinor = -1;

		/* Primary display. */
		EGLDisplay display = EGL_NO_DISPLAY;

		/* Chosen EGL configuration. */
		EGLConfig config = EGL_NO_CONFIG_KHR;

		/* Dummy surface. */
		EGLSurface dummySurface = EGL_NO_SURFACE;

		/* Current EGL context. */
		EGLContext context = EGL_NO_CONTEXT;
	};


	/**
	 * @brief Called to initialize EGL state.
	 *
	 * @param state The state to initialize
	 * @param shareCtx An optional EGL context to share
	 * @return True if EGL was correctly initialized
	 */
	EGLBoolean initEGL(EGLState* state, EGLContext shareCtx);


	/**
	 * @brief Terminate EGL state.
	 *
	 * @param state State to terminate
	 * @return True if EGL was correctly terminated
	 */
	EGLBoolean terminateEGL(EGLState* state);
} // namespace VaporWorldVR
