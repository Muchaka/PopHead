#include "physicsEngine.hpp"

#include "Utilities/math.hpp"
#include "Utilities/debug.hpp"
#include <memory>

namespace ph {

PhysicsEngine::PhysicsEngine()
{
	mStaticBodies.reserve(300);
	mKinematicBodies.reserve(150);
}

CollisionBody& PhysicsEngine::createStaticBodyAndGetTheReference(const sf::FloatRect rect)
{
	mStaticBodies.emplace_back(std::make_unique<CollisionBody>(rect, 0, BodyType::staticBody));
	return *mStaticBodies.back().get();
}

CollisionBody& PhysicsEngine::createKinematicBodyAndGetTheReference(const sf::FloatRect rect, float mass)
{
	mKinematicBodies.emplace_back(std::make_unique<CollisionBody>(rect, mass, BodyType::kinematicBody));
	return *mKinematicBodies.back().get();
}

void PhysicsEngine::clear() noexcept
{
	mStaticBodies.clear();
	mKinematicBodies.clear();
}

void PhysicsEngine::update(sf::Time delta)
{
    for(auto &kinematicBody : mKinematicBodies)
    {
		handleKinematicCollisionsFor(kinematicBody.get());
		kinematicBody->updatePush(delta);
		handleStaticCollisionsFor(*kinematicBody);
		kinematicBody->actionsAtTheEndOfPhysicsLoopIteration();
    }
}

void PhysicsEngine::handleStaticCollisionsFor(CollisionBody& kinematicBody)
{
	for (auto& staticBody : mStaticBodies)
		if (isThereCollision(kinematicBody.getRect(), staticBody->getRect()))
			mStaticCollisionHandler(kinematicBody, *staticBody);
}

void PhysicsEngine::handleKinematicCollisionsFor(CollisionBody* kinematicBody)
{
    for (auto& kinematicBody2 : mKinematicBodies)
    {
		if (kinematicBody == kinematicBody2.get())
			continue;

		if (isThereCollision(kinematicBody->getRect(), kinematicBody2->getRect()))
            mKinematicCollisionHandler(*kinematicBody, *kinematicBody2);
    }
}

bool PhysicsEngine::isThereCollision(sf::FloatRect A, sf::FloatRect B)
{
	return Math::areTheyOverlapping(A, B);
}

}