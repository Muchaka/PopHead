#pragma once

#include "Renderer/texture.hpp"
#include "Renderer/Shaders/shader.hpp"
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <memory>
#include <string>
#include <unordered_map>

namespace ph {

template< typename ResourceType >
class ResourceHolder
{
public:
    bool load(const std::string& filePath);
    auto get(const std::string& filePath) -> ResourceType&;
    bool free(const std::string& filePath);

private:
	std::unordered_map< std::string, std::unique_ptr<ResourceType> > mResources;
};

using SoundBufferHolder = ResourceHolder<sf::SoundBuffer>;
using TextureHolder = ResourceHolder<ph::Texture>;
using ShaderHolder = ResourceHolder<ph::Shader>;
using FontHolder = ResourceHolder<sf::Font>;

}

#include "Resources/resourceHolder.inl"
