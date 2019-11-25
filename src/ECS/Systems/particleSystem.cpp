#include "particleSystem.hpp"
#include "ECS/Components/particleComponents.hpp"
#include "ECS/Components/physicsComponents.hpp"
#include "Renderer/renderer.hpp"

namespace ph::system {

void PatricleSystem::update(float seconds)
{
	auto view = mRegistry.view<component::ParticleEmitter, component::BodyRect>();
	view.each([seconds](component::ParticleEmitter& emi, const component::BodyRect& body)
	{
		// exit if is not emitting
		if(!emi.isEmitting)
			return;

		// alocate particles
		if(!emi.wasInitialized) {
			emi.particles.reserve(emi.amountOfParticles);
			emi.wasInitialized = true;
		}

		// erase particles
		if(!emi.particles.empty() && emi.particles.front().lifetime >= emi.parWholeLifetime)
			emi.particles.erase(emi.particles.begin());

		// add particles
		if((emi.particles.size() < emi.amountOfParticles) &&
		   (emi.particles.empty() || emi.particles.back().lifetime > emi.parWholeLifetime / emi.amountOfParticles))
		{
			Particle particle;
			particle.position = body.rect.getTopLeft() + emi.offset;
			particle.lifetime = 0.f;
			emi.particles.emplace_back(particle);
		}

		for(auto& particle : emi.particles)
		{
			// update particles
			particle.lifetime += seconds;
			particle.position += emi.parInitialVelocity * seconds; // TODO: Add acceleration

			// draw particles
			if(!emi.parTexture && emi.parSize.x == emi.parSize.y)
				Renderer::submitPoint(particle.position, emi.parColor, 0.f, emi.parSize.x);
		}
	});
}

}
