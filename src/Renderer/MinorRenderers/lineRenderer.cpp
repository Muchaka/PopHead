#include "lineRenderer.hpp"
#include "Renderer/API/shader.hpp"
#include "Renderer/API/openglErrors.hpp"
#include "Renderer/Shaders/embeddedShaders.hpp"
#include "Utilities/profiling.hpp"
#include "Utilities/vector4.hpp"
#include "Utilities/cast.hpp"
#include <GL/glew.h>

namespace ph::LineRenderer {

static Shader lineShader;
static const FloatRect* screenBounds;
static unsigned lineVAO;
static unsigned lineVBO;
static unsigned numberOfDrawCalls;
static bool isDebugCountingActive = false;

void setScreenBoundsPtr(const FloatRect* bounds) 
{ 
	screenBounds = bounds;
}

void init()
{
	lineShader.init(shader::lineSrc());
	lineShader.initUniformBlock("SharedData", 0);

	GLCheck( glEnable(GL_LINE_SMOOTH) );
	GLCheck( glHint(GL_LINE_SMOOTH_HINT, GL_NICEST) );

	GLCheck( glGenVertexArrays(1, &lineVAO) );
	GLCheck( glBindVertexArray(lineVAO) );

	GLCheck( glGenBuffers(1, &lineVBO) );
	GLCheck( glBindBuffer(GL_ARRAY_BUFFER, lineVBO) );
	GLCheck( glBufferData(GL_ARRAY_BUFFER, 2 * 6 * sizeof(float), nullptr, GL_DYNAMIC_DRAW) );

	GLCheck( glEnableVertexAttribArray(0) );
	GLCheck( glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0) );
	GLCheck( glEnableVertexAttribArray(1) );
	GLCheck( glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(2 * sizeof(float))) );
}

void shutDown()
{
	GLCheck( glDeleteVertexArrays(1, &lineVAO) );
	GLCheck( glDeleteBuffers(1, &lineVBO) );
}

void drawLine(const sf::Color& colorA, const sf::Color& colorB,
              const sf::Vector2f posA, const sf::Vector2f posB, float thickness)
{
	auto colA = Cast::toNormalizedColorVector4f(colorA);
	auto colB = Cast::toNormalizedColorVector4f(colorB);
	float vertexData[] = {
		posA.x, posA.y, colA.x, colA.y, colA.z, colA.w,
		posB.x, posB.y, colB.x, colB.y, colB.z, colB.w
	};
	GLCheck( glBindBuffer(GL_ARRAY_BUFFER, lineVBO) );
	GLCheck( glBufferSubData(GL_ARRAY_BUFFER, 0, 2 * 6 * sizeof(float), vertexData) );

	GLCheck( glBindVertexArray(lineVAO) );
	lineShader.bind();
	
	GLCheck( glLineWidth(thickness * (360.f / screenBounds->height)) );

	GLCheck( glDrawArrays(GL_LINES, 0, 2) );

#ifndef PH_DISTRIBUTION
	if(isDebugCountingActive)
		++numberOfDrawCalls;
#endif
}

#ifndef PH_DISTRIBUTION
void resetDebugNumbers()
{
	numberOfDrawCalls = 0;
}

unsigned getNumberOfDrawCalls() const 
{
	return numberOfDrawCalls; 
}

void setDebugCountingActive(bool active) 
{ 
	isDebugCountingActive = active; 
}
#endif


}
