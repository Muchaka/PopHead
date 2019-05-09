#ifndef PH_RELEASE

#ifndef POPHEAD_PHYSICS_COLLISIONSDEBUG_H_
#define POPHEAD_PHYSICS_COLLISIONSDEBUG_H_

#include <vector>
#include "collisionDebugRect.hpp"

namespace PopHead{
namespace Physics{


class CollisionBody;
enum class BodyType;


class CollisionDebugSettings
{
private:
	CollisionDebugSettings();

public:
	CollisionDebugSettings(const CollisionDebugSettings&) = delete;
	void operator=(CollisionDebugSettings&) = delete;

	static CollisionDebugSettings& getInstance()
	{
		static CollisionDebugSettings instance;
		return instance;
	}

	void turnOn();
	void turnOff();
};


}}

#endif // !POPHEAD_PHYSICS_COLLISIONSDEBUG_H_

#endif // !PH_RELEASE