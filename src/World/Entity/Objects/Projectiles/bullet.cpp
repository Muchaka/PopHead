#include "bullet.hpp"

ph::Bullet::Bullet(GameData* gameData, const std::string& name, float damage, float range)
	:Projectile(gameData, name, damage, range)
{
}

void ph::Bullet::dealDamage(Object&)
{

}
