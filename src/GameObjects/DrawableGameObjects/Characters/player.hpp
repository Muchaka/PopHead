#pragma once

#include "GameObjects/DrawableGameObjects/character.hpp"

namespace ph{

struct PlayerMotion
{
	PlayerMotion();
	bool isMoving();
	bool isMovingDiagonally();
	void clear();

	bool isMovingLeft;
	bool isMovingRight;
	bool isMovingUp;
	bool isMovingDown;
};

class Player : public Character
{
public:
    Player(GameData*);

    void input() override;
    void updateCurrent(sf::Time delta) override;

	float getPickRadius() const;

private:
	void movementInput();
	void gunInput();
	void meleeWeaponInput();
	void pauseMenuInput();
	void die();
	void updateCounters() const;
	void updateMovement(const sf::Time delta);
	void updateAnimation(const sf::Time delta);
    void setAnimationState(const std::string& stateName);
	void shootingUpdate(const sf::Time delta);
	void meleeAttackUpdate(const sf::Time delta);
	void cameraMovement(sf::Time delta) const;
	void updateListenerPosition() const;
	sf::Vector2f attackDirection();
	void pauseMenuUpdate();

private:
	sf::Clock mTimeFromLastMeleeAttack;
	PlayerMotion mMotion;
	PlayerMotion mLastMotion;
	unsigned mNumberOfOwnedBullets;
	bool mIsShooting;
	bool mIsAttacking;
	bool mWasGamePauseButtonClicked;
	float mPickRadius;
};

}