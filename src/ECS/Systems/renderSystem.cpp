#include "renderSystem.hpp"
#include "ECS/Components/physicsComponents.hpp"
#include "ECS/Components/graphicsComponents.hpp"
#include "Renderer/renderer.hpp"
#include "entt/entity/utility.hpp"

namespace ph::system {

RenderSystem::RenderSystem(entt::registry& registry, sf::Window& window)
	:System(registry)
	,mCamera()
	,mWindow(window)
{
}

void RenderSystem::update(float seconds)
{
		
	// TODO_ren: Move camera somewhere. To separate system for example.
	if(sf::Keyboard::isKeyPressed(sf::Keyboard::Num8))
		mCamera.zoom(1.04f);
	if(sf::Keyboard::isKeyPressed(sf::Keyboard::Num9))
		mCamera.zoom(0.96f);

	if(sf::Keyboard::isKeyPressed(sf::Keyboard::A))
		mCamera.move({-1.f, 0.f});
	if(sf::Keyboard::isKeyPressed(sf::Keyboard::D))
		mCamera.move({1.f, 0.f});
	if(sf::Keyboard::isKeyPressed(sf::Keyboard::W))
		mCamera.move({0.f, -1.f});
	if(sf::Keyboard::isKeyPressed(sf::Keyboard::S))
		mCamera.move({0.f, 1.f});
	
	// NOTE: beginScene() should be probably where endScene() is
	Renderer::beginScene(mCamera);

	auto bodyColorView = mRegistry.view<component::BodyRect, component::Color>(entt::exclude<component::TexturePtr>);
	bodyColorView.each([this](const component::BodyRect& body, const component::Color& color) {
		Renderer::submitQuad(color.color, body.rect.getTopLeft(), static_cast<sf::Vector2i>(body.rect.getSize()));
	});

	auto bodyTextureView = mRegistry.view<component::BodyRect, component::TexturePtr>(entt::exclude<component::BodyRect>);
	bodyTextureView.each([this](const component::BodyRect& body, const component::TexturePtr textureRef) {
		Renderer::submitQuad(*textureRef.texture, body.rect.getTopLeft(), static_cast<sf::Vector2i>(body.rect.getSize()));
	});

	auto bodyTextureShaderView = mRegistry.view<const component::BodyRect, const component::TexturePtr, const component::ShaderPtr>(entt::exclude<component::TextureRect>);
	bodyTextureShaderView.each([this](const component::BodyRect& body, const component::TexturePtr textureRef, const component::ShaderPtr shaderRef) {
		Renderer::submitQuad(*textureRef.texture, shaderRef.shader, body.rect.getTopLeft(), static_cast<sf::Vector2i>(body.rect.getSize()));
	});
	
	auto bodyTextureTextureRectView = mRegistry.view<const component::BodyRect, const component::TexturePtr, const component::TextureRect>(entt::exclude<component::ShaderPtr>);
	bodyTextureTextureRectView.each([this](const component::BodyRect& body, const component::TexturePtr textureRef, const component::TextureRect& textureRect) {
		Renderer::submitQuad(*textureRef.texture, textureRect.rect, body.rect.getTopLeft(), static_cast<sf::Vector2i>(body.rect.getSize()));
	});

	auto bodyTextureColorView = mRegistry.view<component::BodyRect, component::TexturePtr, component::Color>(entt::exclude<component::TextureRect>);
	bodyTextureColorView.each([this](const component::BodyRect& body, const component::TexturePtr textureRef, const component::Color& color) {
		Renderer::submitQuad(*textureRef.texture, color.color, body.rect.getTopLeft(), static_cast<sf::Vector2i>(body.rect.getSize()));
	});
}

}
