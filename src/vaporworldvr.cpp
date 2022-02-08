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
#include "utility.h"

#define VW_TEXTURE_SWAPCHAIN_MAX_LEN 16


static char const shaderVersionString[] = "#version 320 es\n";
static char const shaderCommonTypesString[] =
	"struct ViewInfo"
	"{"
	"	mat4 worldToView;"
	"	mat4 viewToClip;"
	"	mat4 worldToClip;"
	"};";
static char const vertexShaderString[] =
	"layout(location = 0) in vec3 vertexPosition;"
	"layout(location = 1) in vec3 vertexNormal;"
	"layout(location = 2) in vec4 vertexTangent;"
	"layout(location = 3) in float vertexOcclusion;"
	"out vec4 vertexColor;"

	"layout(std430, row_major, binding = 0) buffer ViewInfoBuffer"
	"{"
	"	ViewInfo viewInfo;"
	"};"

	"void main()"
	"{"
	"	gl_Position = viewInfo.worldToClip * vec4(vertexPosition, 1.f);"
	"	vertexColor = vec4(vec3(vertexOcclusion), 1.f);"
	"}";
static char const fragmentShaderString[] =
	"in lowp vec4 vertexColor;"
	"out lowp vec4 outColor;"

	"void main()"
	"{"
	"	outColor = vertexColor;"
	"}";


static int edges[][15] = {
	{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 3, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 9, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{1, 3, 8, 9, 1, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{1, 10, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 3, 8, 1, 10, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{9, 10, 2, 0, 9, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{2, 3, 8, 2, 8, 10, 10, 8, 9, -1, -1, -1, -1, -1, -1},
	{3, 2, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 2, 11, 8, 0, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{1, 0, 9, 2, 11, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{1, 2, 11, 1, 11, 9, 9, 11, 8, -1, -1, -1, -1, -1, -1},
	{3, 1, 10, 11, 3, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 1, 10, 0, 10, 8, 8, 10, 11, -1, -1, -1, -1, -1, -1},
	{3, 0, 9, 3, 9, 11, 11, 9, 10, -1, -1, -1, -1, -1, -1},
	{9, 10, 8, 10, 11, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{4, 8, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{4, 0, 3, 7, 4, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 9, 1, 8, 7, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{4, 9, 1, 4, 1, 7, 7, 1, 3, -1, -1, -1, -1, -1, -1},
	{1, 10, 2, 8, 7, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{3, 7, 4, 3, 4, 0, 1, 10, 2, -1, -1, -1, -1, -1, -1},
	{9, 10, 2, 9, 2, 0, 8, 7, 4, -1, -1, -1, -1, -1, -1},
	{2, 9, 10, 2, 7, 9, 2, 3, 7, 7, 4, 9, -1, -1, -1},
	{8, 7, 4, 3, 2, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{11, 7, 4, 11, 4, 2, 2, 4, 0, -1, -1, -1, -1, -1, -1},
	{9, 1, 0, 8, 7, 4, 2, 11, 3, -1, -1, -1, -1, -1, -1},
	{4, 11, 7, 9, 11, 4, 9, 2, 11, 9, 1, 2, -1, -1, -1},
	{3, 1, 10, 3, 10, 11, 7, 4, 8, -1, -1, -1, -1, -1, -1},
	{1, 10, 11, 1, 11, 4, 1, 4, 0, 7, 4, 11, -1, -1, -1},
	{4, 8, 7, 9, 11, 0, 9, 10, 11, 11, 3, 0, -1, -1, -1},
	{4, 11, 7, 4, 9, 11, 9, 10, 11, -1, -1, -1, -1, -1, -1},
	{9, 4, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{9, 4, 5, 0, 3, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 4, 5, 1, 0, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{8, 4, 5, 8, 5, 3, 3, 5, 1, -1, -1, -1, -1, -1, -1},
	{1, 10, 2, 9, 4, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{3, 8, 0, 1, 10, 2, 4, 5, 9, -1, -1, -1, -1, -1, -1},
	{5, 10, 2, 5, 2, 4, 4, 2, 0, -1, -1, -1, -1, -1, -1},
	{2, 5, 10, 3, 5, 2, 3, 4, 5, 3, 8, 4, -1, -1, -1},
	{9, 4, 5, 2, 11, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 2, 11, 0, 11, 8, 4, 5, 9, -1, -1, -1, -1, -1, -1},
	{0, 4, 5, 0, 5, 1, 2, 11, 3, -1, -1, -1, -1, -1, -1},
	{2, 5, 1, 2, 8, 5, 2, 11, 8, 4, 5, 8, -1, -1, -1},
	{10, 11, 3, 10, 3, 1, 9, 4, 5, -1, -1, -1, -1, -1, -1},
	{4, 5, 9, 0, 1, 8, 8, 1, 10, 8, 10, 11, -1, -1, -1},
	{5, 0, 4, 5, 11, 0, 5, 10, 11, 11, 3, 0, -1, -1, -1},
	{5, 8, 4, 5, 10, 8, 10, 11, 8, -1, -1, -1, -1, -1, -1},
	{9, 8, 7, 5, 9, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{9, 0, 3, 9, 3, 5, 5, 3, 7, -1, -1, -1, -1, -1, -1},
	{0, 8, 7, 0, 7, 1, 1, 7, 5, -1, -1, -1, -1, -1, -1},
	{1, 3, 5, 3, 7, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{9, 8, 7, 9, 7, 5, 10, 2, 1, -1, -1, -1, -1, -1, -1},
	{10, 2, 1, 9, 0, 5, 5, 0, 3, 5, 3, 7, -1, -1, -1},
	{8, 2, 0, 8, 5, 2, 8, 7, 5, 10, 2, 5, -1, -1, -1},
	{2, 5, 10, 2, 3, 5, 3, 7, 5, -1, -1, -1, -1, -1, -1},
	{7, 5, 9, 7, 9, 8, 3, 2, 11, -1, -1, -1, -1, -1, -1},
	{9, 7, 5, 9, 2, 7, 9, 0, 2, 2, 11, 7, -1, -1, -1},
	{2, 11, 3, 0, 8, 1, 1, 8, 7, 1, 7, 5, -1, -1, -1},
	{11, 1, 2, 11, 7, 1, 7, 5, 1, -1, -1, -1, -1, -1, -1},
	{9, 8, 5, 8, 7, 5, 10, 3, 1, 10, 11, 3, -1, -1, -1},
	{5, 0, 7, 5, 9, 0, 7, 0, 11, 1, 10, 0, 11, 0, 10},
	{11, 0, 10, 11, 3, 0, 10, 0, 5, 8, 7, 0, 5, 0, 7},
	{11, 5, 10, 7, 5, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{10, 5, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 3, 8, 5, 6, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{9, 1, 0, 5, 6, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{1, 3, 8, 1, 8, 9, 5, 6, 10, -1, -1, -1, -1, -1, -1},
	{1, 5, 6, 2, 1, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{1, 5, 6, 1, 6, 2, 3, 8, 0, -1, -1, -1, -1, -1, -1},
	{9, 5, 6, 9, 6, 0, 0, 6, 2, -1, -1, -1, -1, -1, -1},
	{5, 8, 9, 5, 2, 8, 5, 6, 2, 3, 8, 2, -1, -1, -1},
	{2, 11, 3, 10, 5, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{11, 8, 0, 11, 0, 2, 10, 5, 6, -1, -1, -1, -1, -1, -1},
	{0, 9, 1, 2, 11, 3, 5, 6, 10, -1, -1, -1, -1, -1, -1},
	{5, 6, 10, 1, 2, 9, 9, 2, 11, 9, 11, 8, -1, -1, -1},
	{6, 11, 3, 6, 3, 5, 5, 3, 1, -1, -1, -1, -1, -1, -1},
	{0, 11, 8, 0, 5, 11, 0, 1, 5, 5, 6, 11, -1, -1, -1},
	{3, 6, 11, 0, 6, 3, 0, 5, 6, 0, 9, 5, -1, -1, -1},
	{6, 9, 5, 6, 11, 9, 11, 8, 9, -1, -1, -1, -1, -1, -1},
	{5, 6, 10, 4, 8, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{4, 0, 3, 4, 3, 7, 6, 10, 5, -1, -1, -1, -1, -1, -1},
	{1, 0, 9, 5, 6, 10, 8, 7, 4, -1, -1, -1, -1, -1, -1},
	{10, 5, 6, 1, 7, 9, 1, 3, 7, 7, 4, 9, -1, -1, -1},
	{6, 2, 1, 6, 1, 5, 4, 8, 7, -1, -1, -1, -1, -1, -1},
	{1, 5, 2, 5, 6, 2, 3, 4, 0, 3, 7, 4, -1, -1, -1},
	{8, 7, 4, 9, 5, 0, 0, 5, 6, 0, 6, 2, -1, -1, -1},
	{7, 9, 3, 7, 4, 9, 3, 9, 2, 5, 6, 9, 2, 9, 6},
	{3, 2, 11, 7, 4, 8, 10, 5, 6, -1, -1, -1, -1, -1, -1},
	{5, 6, 10, 4, 2, 7, 4, 0, 2, 2, 11, 7, -1, -1, -1},
	{0, 9, 1, 4, 8, 7, 2, 11, 3, 5, 6, 10, -1, -1, -1},
	{9, 1, 2, 9, 2, 11, 9, 11, 4, 7, 4, 11, 5, 6, 10},
	{8, 7, 4, 3, 5, 11, 3, 1, 5, 5, 6, 11, -1, -1, -1},
	{5, 11, 1, 5, 6, 11, 1, 11, 0, 7, 4, 11, 0, 11, 4},
	{0, 9, 5, 0, 5, 6, 0, 6, 3, 11, 3, 6, 8, 7, 4},
	{6, 9, 5, 6, 11, 9, 4, 9, 7, 7, 9, 11, -1, -1, -1},
	{10, 9, 4, 6, 10, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{4, 6, 10, 4, 10, 9, 0, 3, 8, -1, -1, -1, -1, -1, -1},
	{10, 1, 0, 10, 0, 6, 6, 0, 4, -1, -1, -1, -1, -1, -1},
	{8, 1, 3, 8, 6, 1, 8, 4, 6, 6, 10, 1, -1, -1, -1},
	{1, 9, 4, 1, 4, 2, 2, 4, 6, -1, -1, -1, -1, -1, -1},
	{3, 8, 0, 1, 9, 2, 2, 9, 4, 2, 4, 6, -1, -1, -1},
	{0, 4, 2, 4, 6, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{8, 2, 3, 8, 4, 2, 4, 6, 2, -1, -1, -1, -1, -1, -1},
	{10, 9, 4, 10, 4, 6, 11, 3, 2, -1, -1, -1, -1, -1, -1},
	{0, 2, 8, 2, 11, 8, 4, 10, 9, 4, 6, 10, -1, -1, -1},
	{3, 2, 11, 0, 6, 1, 0, 4, 6, 6, 10, 1, -1, -1, -1},
	{6, 1, 4, 6, 10, 1, 4, 1, 8, 2, 11, 1, 8, 1, 11},
	{9, 4, 6, 9, 6, 3, 9, 3, 1, 11, 3, 6, -1, -1, -1},
	{8, 1, 11, 8, 0, 1, 11, 1, 6, 9, 4, 1, 6, 1, 4},
	{3, 6, 11, 3, 0, 6, 0, 4, 6, -1, -1, -1, -1, -1, -1},
	{6, 8, 4, 11, 8, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{7, 6, 10, 7, 10, 8, 8, 10, 9, -1, -1, -1, -1, -1, -1},
	{0, 3, 7, 0, 7, 10, 0, 10, 9, 6, 10, 7, -1, -1, -1},
	{10, 7, 6, 1, 7, 10, 1, 8, 7, 1, 0, 8, -1, -1, -1},
	{10, 7, 6, 10, 1, 7, 1, 3, 7, -1, -1, -1, -1, -1, -1},
	{1, 6, 2, 1, 8, 6, 1, 9, 8, 8, 7, 6, -1, -1, -1},
	{2, 9, 6, 2, 1, 9, 6, 9, 7, 0, 3, 9, 7, 9, 3},
	{7, 0, 8, 7, 6, 0, 6, 2, 0, -1, -1, -1, -1, -1, -1},
	{7, 2, 3, 6, 2, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{2, 11, 3, 10, 8, 6, 10, 9, 8, 8, 7, 6, -1, -1, -1},
	{2, 7, 0, 2, 11, 7, 0, 7, 9, 6, 10, 7, 9, 7, 10},
	{1, 0, 8, 1, 8, 7, 1, 7, 10, 6, 10, 7, 2, 11, 3},
	{11, 1, 2, 11, 7, 1, 10, 1, 6, 6, 1, 7, -1, -1, -1},
	{8, 6, 9, 8, 7, 6, 9, 6, 1, 11, 3, 6, 1, 6, 3},
	{0, 1, 9, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{7, 0, 8, 7, 6, 0, 3, 0, 11, 11, 0, 6, -1, -1, -1},
	{7, 6, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{7, 11, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{3, 8, 0, 11, 6, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 9, 1, 11, 6, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{8, 9, 1, 8, 1, 3, 11, 6, 7, -1, -1, -1, -1, -1, -1},
	{10, 2, 1, 6, 7, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{1, 10, 2, 3, 8, 0, 6, 7, 11, -1, -1, -1, -1, -1, -1},
	{2, 0, 9, 2, 9, 10, 6, 7, 11, -1, -1, -1, -1, -1, -1},
	{6, 7, 11, 2, 3, 10, 10, 3, 8, 10, 8, 9, -1, -1, -1},
	{7, 3, 2, 6, 7, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{7, 8, 0, 7, 0, 6, 6, 0, 2, -1, -1, -1, -1, -1, -1},
	{2, 6, 7, 2, 7, 3, 0, 9, 1, -1, -1, -1, -1, -1, -1},
	{1, 2, 6, 1, 6, 8, 1, 8, 9, 8, 6, 7, -1, -1, -1},
	{10, 6, 7, 10, 7, 1, 1, 7, 3, -1, -1, -1, -1, -1, -1},
	{10, 6, 7, 1, 10, 7, 1, 7, 8, 1, 8, 0, -1, -1, -1},
	{0, 7, 3, 0, 10, 7, 0, 9, 10, 6, 7, 10, -1, -1, -1},
	{7, 10, 6, 7, 8, 10, 8, 9, 10, -1, -1, -1, -1, -1, -1},
	{6, 4, 8, 11, 6, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{3, 11, 6, 3, 6, 0, 0, 6, 4, -1, -1, -1, -1, -1, -1},
	{8, 11, 6, 8, 6, 4, 9, 1, 0, -1, -1, -1, -1, -1, -1},
	{9, 6, 4, 9, 3, 6, 9, 1, 3, 11, 6, 3, -1, -1, -1},
	{6, 4, 8, 6, 8, 11, 2, 1, 10, -1, -1, -1, -1, -1, -1},
	{1, 10, 2, 3, 11, 0, 0, 11, 6, 0, 6, 4, -1, -1, -1},
	{4, 8, 11, 4, 11, 6, 0, 9, 2, 2, 9, 10, -1, -1, -1},
	{10, 3, 9, 10, 2, 3, 9, 3, 4, 11, 6, 3, 4, 3, 6},
	{8, 3, 2, 8, 2, 4, 4, 2, 6, -1, -1, -1, -1, -1, -1},
	{0, 2, 4, 4, 2, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{1, 0, 9, 2, 4, 3, 2, 6, 4, 4, 8, 3, -1, -1, -1},
	{1, 4, 9, 1, 2, 4, 2, 6, 4, -1, -1, -1, -1, -1, -1},
	{8, 3, 1, 8, 1, 6, 8, 6, 4, 6, 1, 10, -1, -1, -1},
	{10, 0, 1, 10, 6, 0, 6, 4, 0, -1, -1, -1, -1, -1, -1},
	{4, 3, 6, 4, 8, 3, 6, 3, 10, 0, 9, 3, 10, 3, 9},
	{10, 4, 9, 6, 4, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{4, 5, 9, 7, 11, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 3, 8, 4, 5, 9, 11, 6, 7, -1, -1, -1, -1, -1, -1},
	{5, 1, 0, 5, 0, 4, 7, 11, 6, -1, -1, -1, -1, -1, -1},
	{11, 6, 7, 8, 4, 3, 3, 4, 5, 3, 5, 1, -1, -1, -1},
	{9, 4, 5, 10, 2, 1, 7, 11, 6, -1, -1, -1, -1, -1, -1},
	{6, 7, 11, 1, 10, 2, 0, 3, 8, 4, 5, 9, -1, -1, -1},
	{7, 11, 6, 5, 10, 4, 4, 10, 2, 4, 2, 0, -1, -1, -1},
	{3, 8, 4, 3, 4, 5, 3, 5, 2, 10, 2, 5, 11, 6, 7},
	{7, 3, 2, 7, 2, 6, 5, 9, 4, -1, -1, -1, -1, -1, -1},
	{9, 4, 5, 0, 6, 8, 0, 2, 6, 6, 7, 8, -1, -1, -1},
	{3, 2, 6, 3, 6, 7, 1, 0, 5, 5, 0, 4, -1, -1, -1},
	{6, 8, 2, 6, 7, 8, 2, 8, 1, 4, 5, 8, 1, 8, 5},
	{9, 4, 5, 10, 6, 1, 1, 6, 7, 1, 7, 3, -1, -1, -1},
	{1, 10, 6, 1, 6, 7, 1, 7, 0, 8, 0, 7, 9, 4, 5},
	{4, 10, 0, 4, 5, 10, 0, 10, 3, 6, 7, 10, 3, 10, 7},
	{7, 10, 6, 7, 8, 10, 5, 10, 4, 4, 10, 8, -1, -1, -1},
	{6, 5, 9, 6, 9, 11, 11, 9, 8, -1, -1, -1, -1, -1, -1},
	{3, 11, 6, 0, 3, 6, 0, 6, 5, 0, 5, 9, -1, -1, -1},
	{0, 8, 11, 0, 11, 5, 0, 5, 1, 5, 11, 6, -1, -1, -1},
	{6, 3, 11, 6, 5, 3, 5, 1, 3, -1, -1, -1, -1, -1, -1},
	{1, 10, 2, 9, 11, 5, 9, 8, 11, 11, 6, 5, -1, -1, -1},
	{0, 3, 11, 0, 11, 6, 0, 6, 9, 5, 9, 6, 1, 10, 2},
	{11, 5, 8, 11, 6, 5, 8, 5, 0, 10, 2, 5, 0, 5, 2},
	{6, 3, 11, 6, 5, 3, 2, 3, 10, 10, 3, 5, -1, -1, -1},
	{5, 9, 8, 5, 8, 2, 5, 2, 6, 3, 2, 8, -1, -1, -1},
	{9, 6, 5, 9, 0, 6, 0, 2, 6, -1, -1, -1, -1, -1, -1},
	{1, 8, 5, 1, 0, 8, 5, 8, 6, 3, 2, 8, 6, 8, 2},
	{1, 6, 5, 2, 6, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{1, 6, 3, 1, 10, 6, 3, 6, 8, 5, 9, 6, 8, 6, 9},
	{10, 0, 1, 10, 6, 0, 9, 0, 5, 5, 0, 6, -1, -1, -1},
	{0, 8, 3, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{10, 6, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{11, 10, 5, 7, 11, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{11, 10, 5, 11, 5, 7, 8, 0, 3, -1, -1, -1, -1, -1, -1},
	{5, 7, 11, 5, 11, 10, 1, 0, 9, -1, -1, -1, -1, -1, -1},
	{10, 5, 7, 10, 7, 11, 9, 1, 8, 8, 1, 3, -1, -1, -1},
	{11, 2, 1, 11, 1, 7, 7, 1, 5, -1, -1, -1, -1, -1, -1},
	{0, 3, 8, 1, 7, 2, 1, 5, 7, 7, 11, 2, -1, -1, -1},
	{9, 5, 7, 9, 7, 2, 9, 2, 0, 2, 7, 11, -1, -1, -1},
	{7, 2, 5, 7, 11, 2, 5, 2, 9, 3, 8, 2, 9, 2, 8},
	{2, 10, 5, 2, 5, 3, 3, 5, 7, -1, -1, -1, -1, -1, -1},
	{8, 0, 2, 8, 2, 5, 8, 5, 7, 10, 5, 2, -1, -1, -1},
	{9, 1, 0, 5, 3, 10, 5, 7, 3, 3, 2, 10, -1, -1, -1},
	{9, 2, 8, 9, 1, 2, 8, 2, 7, 10, 5, 2, 7, 2, 5},
	{1, 5, 3, 3, 5, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 7, 8, 0, 1, 7, 1, 5, 7, -1, -1, -1, -1, -1, -1},
	{9, 3, 0, 9, 5, 3, 5, 7, 3, -1, -1, -1, -1, -1, -1},
	{9, 7, 8, 5, 7, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{5, 4, 8, 5, 8, 10, 10, 8, 11, -1, -1, -1, -1, -1, -1},
	{5, 4, 0, 5, 0, 11, 5, 11, 10, 11, 0, 3, -1, -1, -1},
	{0, 9, 1, 8, 10, 4, 8, 11, 10, 10, 5, 4, -1, -1, -1},
	{10, 4, 11, 10, 5, 4, 11, 4, 3, 9, 1, 4, 3, 4, 1},
	{2, 1, 5, 2, 5, 8, 2, 8, 11, 4, 8, 5, -1, -1, -1},
	{0, 11, 4, 0, 3, 11, 4, 11, 5, 2, 1, 11, 5, 11, 1},
	{0, 5, 2, 0, 9, 5, 2, 5, 11, 4, 8, 5, 11, 5, 8},
	{9, 5, 4, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{2, 10, 5, 3, 2, 5, 3, 5, 4, 3, 4, 8, -1, -1, -1},
	{5, 2, 10, 5, 4, 2, 4, 0, 2, -1, -1, -1, -1, -1, -1},
	{3, 2, 10, 3, 10, 5, 3, 5, 8, 4, 8, 5, 0, 9, 1},
	{5, 2, 10, 5, 4, 2, 1, 2, 9, 9, 2, 4, -1, -1, -1},
	{8, 5, 4, 8, 3, 5, 3, 1, 5, -1, -1, -1, -1, -1, -1},
	{0, 5, 4, 1, 5, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{8, 5, 4, 8, 3, 5, 9, 5, 0, 0, 5, 3, -1, -1, -1},
	{9, 5, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{4, 7, 11, 4, 11, 9, 9, 11, 10, -1, -1, -1, -1, -1, -1},
	{0, 3, 8, 4, 7, 9, 9, 7, 11, 9, 11, 10, -1, -1, -1},
	{1, 11, 10, 1, 4, 11, 1, 0, 4, 7, 11, 4, -1, -1, -1},
	{3, 4, 1, 3, 8, 4, 1, 4, 10, 7, 11, 4, 10, 4, 11},
	{4, 7, 11, 9, 4, 11, 9, 11, 2, 9, 2, 1, -1, -1, -1},
	{9, 4, 7, 9, 7, 11, 9, 11, 1, 2, 1, 11, 0, 3, 8},
	{11, 4, 7, 11, 2, 4, 2, 0, 4, -1, -1, -1, -1, -1, -1},
	{11, 4, 7, 11, 2, 4, 8, 4, 3, 3, 4, 2, -1, -1, -1},
	{2, 10, 9, 2, 9, 7, 2, 7, 3, 7, 9, 4, -1, -1, -1},
	{9, 7, 10, 9, 4, 7, 10, 7, 2, 8, 0, 7, 2, 7, 0},
	{3, 10, 7, 3, 2, 10, 7, 10, 4, 1, 0, 10, 4, 10, 0},
	{1, 2, 10, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{4, 1, 9, 4, 7, 1, 7, 3, 1, -1, -1, -1, -1, -1, -1},
	{4, 1, 9, 4, 7, 1, 0, 1, 8, 8, 1, 7, -1, -1, -1},
	{4, 3, 0, 7, 3, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{4, 7, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{9, 8, 10, 10, 8, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{3, 9, 0, 3, 11, 9, 11, 10, 9, -1, -1, -1, -1, -1, -1},
	{0, 10, 1, 0, 8, 10, 8, 11, 10, -1, -1, -1, -1, -1, -1},
	{3, 10, 1, 11, 10, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{1, 11, 2, 1, 9, 11, 9, 8, 11, -1, -1, -1, -1, -1, -1},
	{3, 9, 0, 3, 11, 9, 1, 9, 2, 2, 9, 11, -1, -1, -1},
	{0, 11, 2, 8, 11, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{3, 11, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{2, 8, 3, 2, 10, 8, 10, 9, 8, -1, -1, -1, -1, -1, -1},
	{9, 2, 10, 0, 2, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{2, 8, 3, 2, 10, 8, 0, 8, 1, 1, 8, 10, -1, -1, -1},
	{1, 2, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{1, 8, 3, 9, 8, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 1, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 8, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}
};


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


	struct ShaderInitializer
	{
		enum Type
		{
			Type_None,
			Type_Source,
			Type_Binary
		};

		Type type;
		::std::vector<GLchar> sourceOrBinary;

		void operator()(GLuint shader) const
		{
			switch (type)
			{
			case Type_Source:
			{
				GLchar const* const source[] = {sourceOrBinary.data()};
				GLint const sourceLen[] = {(GLint)sourceOrBinary.size()};
				glShaderSource(shader, 1, source, sourceLen);
			}
			break;

			case Type_Binary:
			{
				VW_ASSERTF(false, "Binary shader source not implemented");
			}
			break;

			default:
				VW_LOG_ERROR("Invalid initializer type '%d'", type);
				break;
			}
		}
	};


	class ComputeShader
	{
		friend class ComputeShaderInstance;

	public:
		ComputeShader([[maybe_unused]]::std::string const& inName)
			: name{inName}
		{}

		void init(ShaderInitializer const& initializer)
		{
			GLint status = GL_FALSE;

			// Create and compile the shader
			GLuint shader = glCreateShader(GL_COMPUTE_SHADER);
			initializer(shader);
			glCompileShader(shader);
			glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
			if (status != GL_TRUE)
			{
				VW_LOG_ERROR("Failed to compile compute shader '%s':\n%s", name.c_str(),
							 GL::getShaderLog(shader).c_str());
				glDeleteShader(shader);
				return;
			}

			program = glCreateProgram();
			glAttachShader(program, shader);
			glLinkProgram(program);
			glGetProgramiv(program, GL_LINK_STATUS, &status);
			GL_CHECK_ERRORS;
			if (status != GL_TRUE)
			{
				VW_LOG_ERROR("Failed to link compute program '%s'", name.c_str());
				glDeleteShader(shader);
				glDeleteProgram(program);
				return;
			}

			VW_LOG_DEBUG("Compute shader '%s' correctly initialized", name.c_str());
		}

		void release()
		{
			glDeleteProgram(program);
		}

	protected:
		GLuint program;
		::std::string name;
	};


	class ComputeShaderInstance
	{
	public:
		FORCE_INLINE char const* getName() const
		{
			return getComputeShader()->name.c_str();
		}

		virtual void bind() const
		{
			ComputeShader* computeShader = getComputeShader();
			glUseProgram(computeShader->program);
		}

		virtual void unbind() const
		{
			glUseProgram(0);
		}

		void dispatch(uint3 groups)
		{
			glDispatchCompute(groups.x, groups.y, groups.z);
			GL_CHECK_ERRORS;
		}

	protected:
		virtual ComputeShader* getComputeShader() const = 0;
	};


#define VOXEL_MAX_VERTEX_COUNT 15
#define CHUNK_MAX_VERTEX_BUFFER_SIZE 0x400000ull // 4 MB
#define MAX_CHUNKS 255


	struct ChunkInfo
	{
        uint32_t vertexCount;
		uint32_t instanceCount;
        uint32_t firstVertex;
        uint32_t _0 = 0;

		float3 origin;
		uint32_t maxVertexCount;
		uint32_t resolution;
		float size;
	};


	struct ChunkVertexPositionOnly
	{
		float3 position;
		float _0;
	};


	struct ChunkVertexVaryings
	{
		float3 normal;
		float occlusion;
		float4 tangent;
	};


	struct Chunk
	{
		ChunkInfo info;
		GLuint vertexBuffer;
		size_t indirectDrawArgsOffset;
		bool dirty;
	};


	class GenerateChunkComputeShader : public ComputeShaderInstance
	{
	public:
		GenerateChunkComputeShader(Chunk const& inChunk, GLuint inChunkInfoBuffer, GLuint inNoiseTextures[],
		                           uint32_t inNumNoiseTextures)
			: chunk{inChunk}
			, chunkInfoBuffer{inChunkInfoBuffer}
			, noiseTextures{}
			, numNoiseTextures{inNumNoiseTextures}
		{
			::memcpy(noiseTextures, inNoiseTextures, numNoiseTextures * sizeof(noiseTextures[0]));
			VW_LOG_DEBUG("%u\n", noiseTextures[numNoiseTextures - 1]);
		}

		virtual void bind() const override
		{
			ComputeShaderInstance::bind();

			// Upload chunk info data
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, chunkInfoBuffer);
			glBufferSubData(GL_SHADER_STORAGE_BUFFER, chunk.indirectDrawArgsOffset, sizeof(ChunkInfo), &chunk.info);
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
			GL_CHECK_ERRORS;

			// Bind buffers
			glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 0, chunkInfoBuffer, chunk.indirectDrawArgsOffset,
			                  sizeof(ChunkInfo));
			glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 1, chunk.vertexBuffer, 0,
			                  chunk.info.maxVertexCount * sizeof(ChunkVertexPositionOnly));
			glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 2, chunk.vertexBuffer,
			                  chunk.info.maxVertexCount * sizeof(ChunkVertexPositionOnly),
			                  chunk.info.maxVertexCount * sizeof(ChunkVertexVaryings));
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, edgesBuffer);
			GL_CHECK_ERRORS;

			// Bind textures
			for (uint32_t idx = 0; idx < numNoiseTextures; idx++)
			{
				glActiveTexture(GL_TEXTURE0 + idx);
				glBindTexture(GL_TEXTURE_3D, noiseTextures[idx]);
			}
		}

		virtual void unbind() const override
		{
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, 0);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, 0);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, 0);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, 0);

			ComputeShaderInstance::unbind();
		}

	private:
		static GLuint edgesBuffer;
		Chunk const& chunk;
		GLuint chunkInfoBuffer;
		GLuint noiseTextures[4];
		uint32_t numNoiseTextures;

		virtual ComputeShader* getComputeShader() const override
		{
			static ComputeShader* computeShader = nullptr;
			if (!computeShader)
			{
				static GLchar const shaderSource[] =
					"#version 320 es\n"
					"layout(local_size_x = 8, local_size_y = 8, local_size_z = 8) in;"

					"struct ChunkInfo"
					"{"
					"	uint vertexCount;"
					"	uint instanceCount;"
					"	uint firstVertex;"
					"	uint _;"
					"	vec3 origin;"
					"	uint maxVertexCount;"
					"	uint resolution;"
					"	float size;"
					"};"

					"const vec3 _128[8] = vec3[](vec3(0.0), vec3(0.0, 1.0, 0.0), vec3(1.0, 1.0, 0.0), vec3(1.0, 0.0, 0.0), vec3(0.0, 0.0, 1.0), vec3(0.0, 1.0, 1.0), vec3(1.0), vec3(1.0, 0.0, 1.0));"
					"const vec3 _209[32] = vec3[](vec3(0.28658199310302734375, 0.2577629983425140380859375, -0.922729015350341796875), vec3(-0.171811997890472412109375, -0.888078987598419189453125, 0.4263750016689300537109375), vec3(0.440764009952545166015625, -0.502089023590087890625, -0.7440659999847412109375), vec3(-0.8410069942474365234375, -0.4288179874420166015625, -0.3298819959163665771484375), vec3(-0.3802129924297332763671875, -0.588038027286529541015625, -0.713898003101348876953125), vec3(-0.0553929992020130157470703125, -0.20715999603271484375, -0.97673797607421875), vec3(-0.9015100002288818359375, -0.0778110027313232421875, 0.4257059991359710693359375), vec3(-0.974592983722686767578125, 0.123829998075962066650390625, -0.18664300441741943359375), vec3(0.2080419957637786865234375, -0.524280011653900146484375, 0.825740993022918701171875), vec3(0.258428990840911865234375, -0.898570001125335693359375, -0.35466301441192626953125), vec3(-0.2621180117130279541015625, 0.574474990367889404296875, -0.775417983531951904296875), vec3(0.735212028026580810546875, 0.551819980144500732421875, 0.3936460018157958984375), vec3(0.828700006008148193359375, -0.523922979831695556640625, -0.196877002716064453125), vec3(0.788742005825042724609375, 0.0057270000688731670379638671875, -0.614697992801666259765625), vec3(-0.69688498973846435546875, 0.6493380069732666015625, -0.3044860064983367919921875), vec3(-0.625312983989715576171875, 0.08241300284862518310546875, -0.776009976863861083984375), vec3(0.3586960136890411376953125, 0.92872297763824462890625, 0.093864001333713531494140625), vec3(0.18826399743556976318359375, 0.6289780139923095703125, 0.754283010959625244140625), vec3(-0.495193004608154296875, 0.2945959866046905517578125, 0.817310988903045654296875), vec3(0.81888902187347412109375, 0.50866997241973876953125, -0.2658509910106658935546875), vec3(0.02718899957835674285888671875, 0.0577570013701915740966796875, 0.99795997142791748046875), vec3(-0.18842099606990814208984375, 0.961802005767822265625, -0.198581993579864501953125), vec3(0.995438992977142333984375, 0.01998200081288814544677734375, 0.09328199923038482666015625), vec3(-0.3152540028095245361328125, -0.925345003604888916015625, -0.2105959951877593994140625), vec3(0.411992013454437255859375, -0.877705991268157958984375, 0.2447330057621002197265625), vec3(0.62585699558258056640625, 0.080058999359607696533203125, 0.77581799030303955078125), vec3(-0.2438389956951141357421875, 0.866185009479522705078125, 0.436194002628326416015625), vec3(-0.72546398639678955078125, -0.643644988536834716796875, 0.2437680065631866455078125), vec3(0.76678502559661865234375, -0.4307020008563995361328125, 0.475959002971649169921875), vec3(-0.4463759958744049072265625, -0.3916639983654022216796875, 0.8045799732208251953125), vec3(-0.76155698299407958984375, 0.56250798702239990234375, 0.32189500331878662109375), vec3(0.344460010528564453125, 0.753223001956939697265625, -0.56035900115966796875));"

					"struct ChunkVertexPositionOnly"
					"{"
					"	vec3 position;"
					"	int _;"
					"};"

					"struct ChunkVertexVaryings"
					"{"
					"	vec3 normal;"
					"	float occlusion;"
					"	vec4 tangent;"
					"};"

					"layout(binding = 0, std430) buffer ChunkInfoBuffer"
					"{"
					"	ChunkInfo chunkInfo;"
					"} _77;"

					"layout(binding = 3, std430) readonly buffer ConnectedEdgesBuffer"
					"{"
					"	int connectedEdges[256][15];"
					"} _353;"

					"layout(binding = 1, std430) writeonly buffer ChunkVertexPositionOnlyBuffer"
					"{"
					"	ChunkVertexPositionOnly positions[];"
					"} _407;"

					"layout(binding = 2, std430) writeonly buffer ChunkVertexVaryingsBuffer"
					"{"
					"	ChunkVertexVaryings varyings[];"
					"} _551;"

					"layout(binding = 0) uniform lowp sampler3D noiseTextureSampler0;"
					"layout(binding = 1) uniform lowp sampler3D noiseTextureSampler1;"
					"layout(binding = 2) uniform lowp sampler3D noiseTextureSampler2;"

					"float sampleDensity(vec3 pos)"
					"{"
					"	float density = 0.0;"
					"	density += (texture(noiseTextureSampler0, pos * 0.007).x * 0.20000000298023223876953125);"
					"	density += (texture(noiseTextureSampler1, pos * 0.05).x * 0.300000011920928955078125);"
					"	density += (texture(noiseTextureSampler2, pos * 0.25).x * 0.5);"
					"	return (density * 2.0) - pos.y;"
					"}"

					"float computeOcclusion(vec3 pos)"
					"{"
					"	float occlusion = 0.0;"
					"	for (uint i = 0u; i < 32u; i++)"
					"	{"
					"		vec3 ray = _209[i] * 0.00999999977648258209228515625;"
					"		float v = 1.0;"
					"		for (uint j = 0u; j < 8u; j++, ray *= 2.0)"
					"		{"
					"			vec3 param = pos + ray;"
					"			float d = sampleDensity(param);"
					"			v *= 1.0 - clamp(d * 9999.0, 0.0, 1.0);"
					"		}"
					"		occlusion += (v / 32.0);"
					"	}"
					"	return occlusion * 0.5f + 0.5f;"
					"}"

					"void main()"
					"{"
					"	ivec3 voxelIndex = ivec3(gl_GlobalInvocationID);"
					"	float voxelSize = _77.chunkInfo.size / float(_77.chunkInfo.resolution);"
					"	vec3 voxelOffset = vec3(voxelIndex) * voxelSize;"
					"	vec3 voxelWSPos = _77.chunkInfo.origin + voxelOffset;"
					"	float densities[8];"
					"	for (int i = 0; i < 8; i++)"
					"	{"
					"		vec3 param = voxelWSPos + (_128[i] * voxelSize);"
					"		densities[i] = sampleDensity(param);"
					"	}"
					"	uint perm = (((((((uint(densities[0] > 0.0) << uint(0)) | (uint(densities[1] > 0.0) << uint(1))) | (uint(densities[2] > 0.0) << uint(2))) | (uint(densities[3] > 0.0) << uint(3))) | (uint(densities[4] > 0.0) << uint(4))) | (uint(densities[5] > 0.0) << uint(5))) | (uint(densities[6] > 0.0) << uint(6))) | (uint(densities[7] > 0.0) << uint(7));"
					"	if ((perm == 0u) || (perm == 255u))"
					"	{"
					"		return;"
					"	}"
					"	vec3 voxelVertexOffsets[12] = vec3[](mix(vec3(0.0), vec3(0.0, 1.0, 0.0), vec3((-densities[0]) / (densities[1] - densities[0]))), mix(vec3(0.0, 1.0, 0.0), vec3(1.0, 1.0, 0.0), vec3((-densities[1]) / (densities[2] - densities[1]))), mix(vec3(1.0, 1.0, 0.0), vec3(1.0, 0.0, 0.0), vec3((-densities[2]) / (densities[3] - densities[2]))), mix(vec3(1.0, 0.0, 0.0), vec3(0.0), vec3((-densities[3]) / (densities[0] - densities[3]))), mix(vec3(0.0, 0.0, 1.0), vec3(0.0, 1.0, 1.0), vec3((-densities[4]) / (densities[5] - densities[4]))), mix(vec3(0.0, 1.0, 1.0), vec3(1.0), vec3((-densities[5]) / (densities[6] - densities[5]))), mix(vec3(1.0), vec3(1.0, 0.0, 1.0), vec3((-densities[6]) / (densities[7] - densities[6]))), mix(vec3(1.0, 0.0, 1.0), vec3(0.0, 0.0, 1.0), vec3((-densities[7]) / (densities[4] - densities[7]))), mix(vec3(0.0), vec3(0.0, 0.0, 1.0), vec3((-densities[0]) / (densities[4] - densities[0]))), mix(vec3(0.0, 1.0, 0.0), vec3(0.0, 1.0, 1.0), vec3((-densities[1]) / (densities[5] - densities[1]))), mix(vec3(1.0, 1.0, 0.0), vec3(1.0), vec3((-densities[2]) / (densities[6] - densities[2]))), mix(vec3(1.0, 0.0, 0.0), vec3(1.0, 0.0, 1.0), vec3((-densities[3]) / (densities[7] - densities[3]))));"
					"	uint numVertices = 0u;"
					"	for (; (_353.connectedEdges[perm][numVertices] != (-1)) && (numVertices <= 15u); numVertices++)"
					"	{"
					"	}"
					"	if ((_77.chunkInfo.vertexCount + numVertices) > _77.chunkInfo.maxVertexCount)"
					"	{"
					"		return;"
					"	}"
					"	uint _379 = atomicAdd(_77.chunkInfo.vertexCount, numVertices);"
					"	uint baseVertexIdx = _379;"
					"	for (uint vertexIdx = 0u; vertexIdx < numVertices; vertexIdx++)"
					"	{"
					"		uint edgeIdx = uint(_353.connectedEdges[perm][vertexIdx]);"
					"		vec3 vertexWSPos = voxelWSPos + (voxelVertexOffsets[edgeIdx] * voxelSize);"
					"		_407.positions[baseVertexIdx + vertexIdx].position = vertexWSPos;"
					"		vec3 param_1 = vertexWSPos;"
					"		_551.varyings[baseVertexIdx + vertexIdx].occlusion = computeOcclusion(param_1);"
					"	}"
					"}";

				ShaderInitializer initializer;
				initializer.type = ShaderInitializer::Type_Source;
				initializer.sourceOrBinary = {shaderSource, shaderSource + sizeof(shaderSource)};

				computeShader = new ComputeShader("GenerateChunkComputeShader");
				computeShader->init(initializer);

				// Also create edges buffer
				glGenBuffers(1, &edgesBuffer);
				glBindBuffer(GL_SHADER_STORAGE_BUFFER, edgesBuffer);
				glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(edges), edges, GL_STATIC_DRAW);
				glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
				GL_CHECK_ERRORS;
			}

			return computeShader;
		}
	};

	GLuint GenerateChunkComputeShader::edgesBuffer = 0;


	struct Scene
	{
		GLuint vao;
		GLuint indirectDrawArgsBuffer;
		GLuint noiseTextures[4];
		Chunk chunk;
	};


	struct PerlinNoise
	{
		float3 grads[512];
		uint32_t perms[512];
	};


	static void initChunk(Chunk& chunk, uint32_t idx)
	{
		constexpr size_t vertexDataSize = sizeof(ChunkVertexPositionOnly) + sizeof(ChunkVertexVaryings);
		chunk.info.vertexCount = 0;
		chunk.info.firstVertex = 0;
		chunk.info.instanceCount = 1;
		chunk.info.origin = {-1.f, -1.f, -1.f};
		chunk.info.resolution = 64;
		chunk.info.size = 2.f;
		chunk.info.maxVertexCount = CHUNK_MAX_VERTEX_BUFFER_SIZE / vertexDataSize;

		glGenBuffers(1, &chunk.vertexBuffer);
		glBindBuffer(GL_ARRAY_BUFFER, chunk.vertexBuffer);
		glBufferData(GL_ARRAY_BUFFER, CHUNK_MAX_VERTEX_BUFFER_SIZE, NULL, GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		chunk.indirectDrawArgsOffset = idx * sizeof(ChunkInfo);
		chunk.dirty = true;
	}


	static void initPerlinNoiseGenerator(PerlinNoise& noiseGen)
	{
		for (uint32_t i = 0; i < 256; ++i)
		{
			noiseGen.perms[i] = i;
		}

		for (uint32_t i = 0; i < 256; ++i)
		{
			uint32_t j = rand() & 0xff;
			::std::swap(noiseGen.perms[i], noiseGen.perms[j]);
		}

		for (uint32_t i = 0; i < 256; ++i)
		{
			constexpr float deltaAngle = (M_PI * 2.f) / 256;
			noiseGen.grads[i] = {Math::cos(noiseGen.perms[i] * deltaAngle),
			                     Math::cos(noiseGen.perms[noiseGen.perms[i]] * deltaAngle),
								 Math::sin(noiseGen.perms[i] * deltaAngle)};
		}
	}


	static float perlinGradientValue(PerlinNoise const& noise, float3 p, int3 i, int3 period)
	{
		float3 grad = noise.grads[noise.perms[(noise.perms[(noise.perms[i.x % period.x] + i.y) % period.y] + i.z) % period.z]];
		return p.dot(grad);
	}


	static float perlinNoiseSample3D(PerlinNoise const& noise, float3 pos, int3 period)
	{
		static constexpr int3 voxelVertices[] = {{0, 0, 0},
		                                         {1, 0, 0},
		                                         {0, 1, 0},
		                                         {1, 1, 0},
		                                         {0, 0, 1},
		                                         {1, 0, 1},
		                                         {0, 1, 1},
		                                         {1, 1, 1}};

		int3 i = (int3)pos;
		float3 t = pos - (float3)i;
		float3 w = t * t * (3.f - t * 2.f);

		return Math::lerp(
			Math::lerp(
				Math::lerp(perlinGradientValue(noise, t - (float3)voxelVertices[0], i + voxelVertices[0], period),
				           perlinGradientValue(noise, t - (float3)voxelVertices[1], i + voxelVertices[1], period), w.x),
				Math::lerp(perlinGradientValue(noise, t - (float3)voxelVertices[2], i + voxelVertices[2], period),
				           perlinGradientValue(noise, t - (float3)voxelVertices[3], i + voxelVertices[3], period), w.x),
				w.y
			),
			Math::lerp(
				Math::lerp(perlinGradientValue(noise, t - (float3)voxelVertices[4], i + voxelVertices[4], period),
				           perlinGradientValue(noise, t - (float3)voxelVertices[5], i + voxelVertices[5], period), w.x),
				Math::lerp(perlinGradientValue(noise, t - (float3)voxelVertices[6], i + voxelVertices[6], period),
				           perlinGradientValue(noise, t - (float3)voxelVertices[7], i + voxelVertices[7], period), w.x),
				w.y
			),
			w.z
		);
	}


	static float perlinNoiseSampleOctaves3D(PerlinNoise const& noise, float3 const& pos, int3 period, uint32_t numOctaves)
	{
		float value = 0.f;
		float freq = 1.f;
		float ampl = 0.5f;

		for (uint32_t octave = 0; octave < numOctaves; octave++)
		{
			value += perlinNoiseSample3D(noise, pos * freq, period) * ampl;
			freq *= 2.f;
			period *= 2;
			ampl *= 0.5f;
		}

		return value;
	}


	static void initNoiseTextures(GLuint textures[], uint32_t numTextures, uint3 const& textureRes)
	{
		// The size of the texture buffer in Bytes
		size_t const textureBufferSize = textureRes.x * textureRes.y * textureRes.z * sizeof(float);
		float3 const textureDensity{textureRes / 4};

		// Generate GL textures
		glGenTextures(numTextures, textures);

		// Buffer used to store texture temporary data
		float* textureBuffer = (float*)::malloc(textureBufferSize);

		for (uint32_t idx = 0; idx < numTextures; ++idx)
		{
			VW_LOG_DEBUG("Generating noise texture #%u", idx);

			// Noise generator
			PerlinNoise noiseGen;
			initPerlinNoiseGenerator(noiseGen);

			for (uint32_t i = 0; i < textureRes.x; ++i)
			{
				for (uint32_t j = 0; j < textureRes.y; ++j)
				{
					for (uint32_t k = 0; k < textureRes.z; ++k)
					{
						size_t const pixelIdx = ((i * textureRes.y) + j) * textureRes.z + k;
						float3 pos{i / textureDensity.x, j / textureDensity.y, k / textureDensity.z};
						textureBuffer[pixelIdx] = perlinNoiseSampleOctaves3D(noiseGen, pos, 4, 5);
					}
				}
			}

			glBindTexture(GL_TEXTURE_3D, textures[idx]);
			glTexStorage3D(GL_TEXTURE_3D, 1, GL_R32F, textureRes.x, textureRes.y, textureRes.z);
			glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, textureRes.x, textureRes.y, textureRes.z, GL_RED, GL_FLOAT,
			                textureBuffer);
			glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glBindTexture(GL_TEXTURE_3D, 0);
		}

		::free(textureBuffer);
	}


	struct RenderCommand : public Message {};

	struct RenderCommandShutdown : public RenderCommand
	{
		//
	};

	struct RenderCommandBeginFrame : public RenderCommand
	{
		uint64_t frameIdx;
	};

	struct RenderCommandEndFrame : public RenderCommand
	{
		ovrMobile* ovr;
		ovrTracking2 tracking;
		uint64_t frameIdx;
		uint32_t frameFlags;
		uint32_t swapInterval;
		double displayTime;
		Scene* scene;
	};

	struct RenderCommandFlush : public RenderCommand {};

	struct RenderCommandDispatchCompute : public RenderCommand
	{
		ComputeShaderInstance* shader;
		uint3 groups;
		GLbitfield forceMemoryBarrier = 0;
		GLsync* fence = nullptr;
	};


	class Renderer : public Runnable, public MessageTarget<Renderer,
	                                                       RenderCommandShutdown, RenderCommandBeginFrame, RenderCommandEndFrame,
														   RenderCommandFlush, RenderCommandDispatchCompute>
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

		void processMessage(RenderCommandShutdown const& cmd)
		{
			// Set exit flag
			requestExit = true;
		}

		FORCE_INLINE void processMessage(RenderCommandBeginFrame const& cmd)
		{
			//
		}

		void processMessage(RenderCommandEndFrame const& cmd)
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
				glUseProgram(program);

				Framebuffer& fb = framebuffers[eyeIdx];
				glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fb.fbos[fb.textureSwapChainIdx]);
				GL_CHECK_ERRORS;

				glEnable(GL_DEPTH_TEST);
				glDisable(GL_CULL_FACE);
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
					viewInfoData[2] = viewInfoData[1].dot(viewInfoData[0]);
					glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
				}
				glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, viewInfoBuffer);
				GL_CHECK_ERRORS;

				if (cmd.scene)
				{
					if (cmd.scene->vao == 0)
					{
						// Create vertex arrays
						glGenVertexArrays(1, &cmd.scene->vao);
						glBindVertexArray(cmd.scene->vao);
						GL_CHECK_ERRORS;

						// Define attrib formats and bindings
						glEnableVertexAttribArray(0);
						glVertexAttribFormat(0, 3, GL_FLOAT, GL_FALSE, 0);
						glVertexAttribBinding(0, 0);
						GL_CHECK_ERRORS;

						glDisableVertexAttribArray(1);
						glDisableVertexAttribArray(2);
						glEnableVertexAttribArray(3);
						glVertexAttribFormat(1, 3, GL_FLOAT, GL_FALSE, offsetof(ChunkVertexVaryings, normal));
						glVertexAttribFormat(2, 4, GL_FLOAT, GL_FALSE, offsetof(ChunkVertexVaryings, tangent));
						glVertexAttribFormat(3, 1, GL_FLOAT, GL_FALSE, offsetof(ChunkVertexVaryings, occlusion));
						glVertexAttribBinding(1, 1);
						glVertexAttribBinding(2, 1);
						glVertexAttribBinding(3, 1);
						GL_CHECK_ERRORS;

						glBindVertexArray(0);
					}

					// Draw chunk
					glBindVertexArray(cmd.scene->vao);
					glBindVertexBuffer(0, cmd.scene->chunk.vertexBuffer, 0, sizeof(ChunkVertexPositionOnly));
					glBindVertexBuffer(1, cmd.scene->chunk.vertexBuffer,
					                   cmd.scene->chunk.info.maxVertexCount * sizeof(ChunkVertexPositionOnly),
									   sizeof(ChunkVertexVaryings));
					glBindBuffer(GL_DRAW_INDIRECT_BUFFER, cmd.scene->indirectDrawArgsBuffer);
					glDrawArraysIndirect(GL_TRIANGLES, (void*)cmd.scene->chunk.indirectDrawArgsOffset);
					glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
					glBindVertexArray(0);
					GL_CHECK_ERRORS;
				}

				const GLenum depthAttachment[] = {GL_DEPTH_ATTACHMENT};
				glInvalidateFramebuffer(GL_DRAW_FRAMEBUFFER, 1, depthAttachment);
				glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
				fb.textureSwapChainIdx = (fb.textureSwapChainIdx + 1) % fb.textureSwapChainLen;

				glUseProgram(0);
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

		void processMessage(RenderCommandFlush const& cmd)
		{
			// TODO: Flush
		}

		void processMessage(RenderCommandDispatchCompute const& cmd)
		{
			VW_LOG_DEBUG("Dispatch compute shader '%s'", cmd.shader->getName());
			cmd.shader->bind();
			cmd.shader->dispatch(cmd.groups);
			cmd.shader->unbind();

			if (cmd.forceMemoryBarrier != GL_NONE)
			{
				// Sync memory after compute
				glMemoryBarrier(cmd.forceMemoryBarrier);
				GL_CHECK_ERRORS;
			}

			if (cmd.fence)
			{
				// Create fence to wait for compute shader to end
				*(cmd.fence) = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
			}
			VW_LOG_DEBUG("Dispatch done");
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
				vrapi_SetTrackingSpace(ovr, VRAPI_TRACKING_SPACE_LOCAL_FLOOR);
				displayTime = vrapi_GetPredictedDisplayTime(ovr, frameCounter);
				tracking = vrapi_GetPredictedTracking2(ovr, displayTime);

				// Begin next frame.
				RenderCommandBeginFrame beginFrameCmd{};
				beginFrameCmd.frameIdx = frameCounter;
				renderer->postMessage(beginFrameCmd);

				// TODO: Render scene
				updateScene();

				// End current frame.
				static constexpr uint32_t swapInterval = 1;
				RenderCommandEndFrame endFrameCmd{};
				endFrameCmd.ovr = ovr;
				endFrameCmd.frameIdx = frameCounter;
				endFrameCmd.displayTime = displayTime;
				endFrameCmd.swapInterval = swapInterval;
				endFrameCmd.tracking = tracking;
				endFrameCmd.scene = scene;
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

			// Delete >>>>>>>>>>>>>>>>>>>>>>>
			setupScene();
		}

		void teardown()
		{
			teardownScene();
			// Delete <<<<<<<<<<<<<<<<<<<<<<

			// Stop the render thread
			renderer->postMessage<RenderCommandShutdown>({}, MessageWait_Processed);
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

		// REMOVE ---------------------------------
		Scene* scene;

		void setupScene()
		{
			scene = new Scene;
			scene->vao = 0;

			// Shared buffer for indrect draw arguments
			glGenBuffers(1, &scene->indirectDrawArgsBuffer);
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, scene->indirectDrawArgsBuffer);
			glBufferData(GL_SHADER_STORAGE_BUFFER, MAX_CHUNKS * sizeof(ChunkInfo), NULL, GL_DYNAMIC_COPY);
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

			// Create and upload noise textures
			initNoiseTextures(scene->noiseTextures, 3, 64);

			// Initialize first chunk
			initChunk(scene->chunk, 0);
		}

		void updateScene()
		{
			if (scene->chunk.dirty)
			{
				GLsync fence;

				// We need to regenerate chunk data
				RenderCommandDispatchCompute computeCmd;
				computeCmd.shader = new GenerateChunkComputeShader(scene->chunk, scene->indirectDrawArgsBuffer,
				                                                   scene->noiseTextures, 3);
				computeCmd.groups = {8, 8, 8};
				computeCmd.fence = &fence;
				renderer->postMessage(computeCmd, MessageWait_Processed);
				scene->chunk.dirty = false;

				// Wait for compute shader to terminate execution
				glWaitSync(fence, 0, 0);
				glBindBuffer(GL_SHADER_STORAGE_BUFFER, scene->indirectDrawArgsBuffer);
				ChunkInfo* info = (ChunkInfo*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, sizeof(ChunkInfo), GL_MAP_READ_BIT);
				if (info)
				{
					VW_LOG_DEBUG("Generated %u vertices", info->vertexCount);
					glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
				}
				glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
			}
		}

		void teardownScene()
		{
			//
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
