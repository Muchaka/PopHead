#include "GameObjects/DrawableGameObjects/melee.hpp"
#include "GameObjects/DrawableGameObjects/Characters/player.hpp"
#include "gameData.hpp"

namespace ph {

Swing::Swing(const GameObject& opponentsNode, const sf::Vector2f direction, const sf::Vector2f position,
	const float damage, const float range, const float rotationRange)
	:mEnemiesNode(opponentsNode)
	,mDirection(direction)
	,mStartPositionBeginning(position)
	,mDamage(damage)
	,mRange(range)
	,mRotationRange(rotationRange)
	,mRotation(0.f)
{
	setMeeleWeaponStartingPosition(direction);
	handleHitCharacters();
}

void Swing::setMeeleWeaponStartingPosition(const sf::Vector2f attackDirection)
{
	sf::Transform rotation;
	rotation.rotate(-mRotationRange / 2, mStartPositionBeginning);
	mHitArea[0].position = mStartPositionBeginning;
	mHitArea[1].position = mStartPositionBeginning + (attackDirection * mRange);
	mHitArea[1] = rotation.transformPoint(mHitArea[1].position);
	mStartPositionEnd = mHitArea[1].position;
}

void Swing::handleHitCharacters()
{
	auto* characterWhoWasHit = getCharacterWhoWasHit();
	if (characterWhoWasHit == nullptr)
		return;
	characterWhoWasHit->takeDamage(static_cast<unsigned int>(mDamage));
}

auto Swing::getCharacterWhoWasHit() -> Character*
{
	while(mRotation < mRotationRange)
	{ 
		for (auto& enemy : mEnemiesNode.getChildren()) {
			auto& e = dynamic_cast<Character&>(*enemy);
			if (!e.isDead() && wasEnemyHit(e))
				return &e;
		}
		incrementRotation();
	}
	return nullptr;
}

bool Swing::wasEnemyHit(Character& character)
{
	const sf::FloatRect hitbox = character.getGlobalBounds();
	return Math::isPointInsideRect(mHitArea[1].position, hitbox);
}

void Swing::incrementRotation()
{
	mRotation += 5.f;
	sf::Transform rotation;
	rotation.rotate(5.f, mStartPositionBeginning);
	mHitArea[1] = rotation.transformPoint(mHitArea[1].position);
}


MeleeWeapon::MeleeWeapon(GameData* const gameData, const float damage, const float range, const float rotatationRange)
	:GameObject("sword")
	,mGameData(gameData)
	,mDamage(damage)
	,mRange(range)
	,mRotationRange(rotatationRange)
	,mGraphicsRotation(0.f)
	,mShouldDrawSwing(false)
{
}

void MeleeWeapon::attack(const sf::Vector2f attackDirection)
{
	mGameData->getSoundPlayer().playAmbientSound("sounds/swordAttack.wav");
	sf::Vector2f rightHandPosition = getRightHandPosition(attackDirection);
	Swing swing(getEnemies(), attackDirection, rightHandPosition, mDamage, mRange, mRotationRange);
	initializeAttackGraphics(swing);
}

auto MeleeWeapon::getEnemies() -> GameObject&
{
	auto& player = getParent();
	auto& root = player.getParent();
	return root.getChild("enemy_container");
}

sf::Vector2f MeleeWeapon::getRightHandPosition(const sf::Vector2f attackDirection)
{
	sf::Vector2f position = getWorldPosition();

	if (attackDirection == sf::Vector2f(1, 0))
		position += {10, 20};
	else if(attackDirection == sf::Vector2f(-1, 0))
		position += {5, 15};
	else if(attackDirection == sf::Vector2f(0, 1))
		position += {3, 20};
	else if(attackDirection == sf::Vector2f(0, -1))
		position += {15, 15};
	else if(attackDirection == sf::Vector2f(0.7f, -0.7f))
		position += {20, 3};
	else if(attackDirection == sf::Vector2f(-0.7f, -0.7f))
		position += {3, 3};
	else if(attackDirection == sf::Vector2f(0.7f, 0.7f))
		position += {10, 20};
	else if(attackDirection == sf::Vector2f(-0.7f, 0.7f))
		position += {0, 10};
	else
		PH_UNEXPECTED_SITUATION("Direction vector like this shouldn't exist.");

	return position;
}

void MeleeWeapon::initializeAttackGraphics(const Swing& swing)
{
	mHitGraphics = swing.getPositionFromBeginning();
	mShouldDrawSwing = true;
}

void MeleeWeapon::updateHitGraphicsRotation()
{
	sf::Transform rotate;
	rotate.rotate(15.f, mHitGraphics[0].position);
	mHitGraphics[1] = rotate.transformPoint(mHitGraphics[1].position);
	mGraphicsRotation += 15.f;
}

void MeleeWeapon::resetAttackGraphics()
{
	mHitGraphics[0].position = {0, 0};
	mHitGraphics[1].position = {0, 0};
	mGraphicsRotation = 0.f;
	mShouldDrawSwing = false;
}

void MeleeWeapon::updateCurrent(const sf::Time delta)
{
	if (mGraphicsRotation < mRotationRange && mShouldDrawSwing)
		updateHitGraphicsRotation();
	else
		resetAttackGraphics();
}

void MeleeWeapon::drawCurrent(sf::RenderTarget& target, sf::RenderStates) const
{
	target.draw(mHitGraphics.data(), 2, sf::Lines);
}

}
