#include "collisionBody.hpp"
#include "bodyType.hpp"

using PopHead::Physics::CollisionBody;

CollisionBody::CollisionBody(sf::FloatRect rect, float mass, PopHead::Physics::BodyType bodyType,
							 PopHead::World::Entity::Object* const thisPointer, PopHead::Base::GameData* gameData)
:mRect(rect)
,mMass(mass)
,mVelocity()
,pointerToObjectWhichIsOwnerOfThisCollisionBody(thisPointer)
{
	switch (bodyType)
	{
	case PopHead::Physics::BodyType::staticBody:
		gameData->getPhysicsEngine().addStaticBody(this);
		break;

	case PopHead::Physics::BodyType::kinematicBody:
		gameData->getPhysicsEngine().addKinematicBody(this);
		break;
	}
}

void CollisionBody::move(sf::Vector2f velocity)
{
	mVelocity += velocity;
}

void CollisionBody::movePhysics()
{
	mRect.left += mVelocity.x;
	mRect.top += mVelocity.y;
}

void CollisionBody::setPositionOfGraphicRepresentation()
{
	pointerToObjectWhichIsOwnerOfThisCollisionBody->setPosition(sf::Vector2f(mRect.left, mRect.top));
}
