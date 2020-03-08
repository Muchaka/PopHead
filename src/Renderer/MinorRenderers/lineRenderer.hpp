#pragma once

#include "Renderer/API/shader.hpp"
#include "Utilities/rect.hpp"
#include <SFML/System/Vector2.hpp>
#include <SFML/Graphics/Color.hpp>

namespace ph::LineRenderer {

void init();
void shutDown();

void setScreenBoundsPtr(const FloatRect* screenBounds);

void drawLine(const sf::Color& colorA, const sf::Color& colorB,
			  const sf::Vector2f positionA, const sf::Vector2f positionB, float thickness = 1.f);

#ifndef PH_DISTRIBUTION
void setDebugCountingActive(bool active); 
void resetDebugNumbers();
unsigned getNumberOfDrawCalls(); 
#endif

}

