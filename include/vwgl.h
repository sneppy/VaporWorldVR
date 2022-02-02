#pragma once

#include <string>

#include <GLES3/gl32.h>
#include <GLES3/gl3ext.h>

#include "core_types.h"
#include "logging.h"


#if VW_BUILD_DEBUG
# define GL_CHECK_ERRORS GL::flushErrors(__FILE__, __LINE__)
#else
# define GL_CHECK_ERRORS
#endif

#define SWITCH_CASE(err) case err: return #err;


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
	}

	/**
	 * @brief Returns the log of the shader compilation.
	 *
	 * @param shader The name of the shader resource
	 * @return String with shader log
	 */
	::std::string getShaderLog(GLuint shader)
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
} // namespace VaporWorldVR::GL


#undef SWITCH_CASE
