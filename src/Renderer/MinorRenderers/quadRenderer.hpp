#pragma once

#include "quadData.hpp"
#include "Renderer/API/shader.hpp"
#include "Utilities/rect.hpp"
#include "Utilities/vector4.hpp"
#include <SFML/System/Vector2.hpp>
#include <SFML/Graphics/Color.hpp>
#include <optional>

namespace ph {
	class Texture;
}

namespace ph::QuadRenderer {

#ifndef PH_DISTRIBUTION
struct DebugArray
{
	unsigned data[100] = {};
	unsigned marker = 0;
};

struct DebugNumbers
{
	DebugArray renderGroupsSizes = {}; 
	DebugArray renderGroupsZ = {}; 
	DebugArray renderGroupsIndices = {}; 
	DebugArray notAffectedByLightRenderGroupsSizes = {}; 
	DebugArray notAffectedByLightRenderGroupsZ = {}; 
	DebugArray notAffectedByLightRenderGroupsIndices = {}; 
	unsigned arenaUsedMemory = 0;
	unsigned renderGroups = 0;
	unsigned renderGroupsNotAffectedByLight = 0;
	unsigned drawCalls = 0;
	unsigned drawnSprites = 0;
	unsigned drawnTextures = 0;
};

DebugNumbers getDebugNumbers();
void resetDebugNumbers();
void setDebugNumbersEnabled(bool enabled);
#endif

void init();
void shutDown();

void setScreenBoundsPtr(const FloatRect* bounds); 

void submitBunchOfQuadsWithTheSameTexture(std::vector<QuadData>&, Texture*, const Shader*, float z, ProjectionType projectionType);

void submitQuad(Texture*, const IntRect* textureRect, const sf::Color*, const Shader*,
				sf::Vector2f position, sf::Vector2f size, float z, float rotation, sf::Vector2f rotationOrigin, ProjectionType, bool isAffectedByLight);

void flush(bool affectedByLight);

}

