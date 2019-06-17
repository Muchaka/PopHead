#pragma once

#include <SFML/Graphics.hpp>

namespace ph{

namespace TerminalStyleConstants
{
	constexpr float terminalWidth = 960;
	constexpr float terminalHeight = 400;
	const sf::Vector2f terminalSize(terminalWidth, terminalHeight);

	constexpr float terminalXPosition = -480;
	constexpr float terminalYPosition = 100;
	const sf::Vector2f terminalPosition(terminalXPosition, terminalYPosition);

	const sf::Color terminalColor(0, 0, 0, 230);

	constexpr float textXposition = terminalXPosition + 5;
	const sf::Vector2f inputTextPosition(textXposition, terminalYPosition);

	const sf::Vector2f separatorBetweenInputAndOutputAreaSize(terminalWidth, 10);
	const sf::Vector2f separatorBetweenInputAndOutputAreaPosition(terminalXPosition, terminalYPosition + 25);
	const sf::Color separatorBetweenInputAndOutputColor(sf::Color::Black);

	const std::string fontPath("fonts/consolab.ttf");
	constexpr unsigned int inputCharacterSize = 18;
	constexpr unsigned int outputCharacterSize = 15;
	const sf::Color inputTextColor(sf::Color::White);

	constexpr unsigned int numberOfOutputLines = 19;
	constexpr float spaceBetweenTheLines = 18;
}

}