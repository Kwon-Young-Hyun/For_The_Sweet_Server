#pragma once

#include "Physx.h"

enum { Anim_Idle, Anim_Walk, 
	Anim_Weak_Attack1, Anim_Weak_Attack2, Anim_Weak_Attack3, 
	Anim_Hard_Attack1, Anim_Hard_Attack2, 
	Anim_Guard, Anim_PowerUp, Anim_Jump 
};

class PlayerHitReport : public PxUserControllerHitReport {
public:
	void	onShapeHit(const PxControllerShapeHit &hit) {
		cout << "ShapedHit!!!\n";
	}
	void 	onControllerHit(const PxControllersHit &hit) {
		cout << "ControllerHit!!!\n";
	}
	void 	onObstacleHit(const PxControllerObstacleHit &hit) {
		cout << "ObstacleHit!!!\n";
	}

};

class CPlayer
{
public:
	CPlayer();
	~CPlayer();

	void move(int direction, float distance);
	void setPlayerController(CPhysx *physx);
	void setPosition(PxVec3 pos);
	void setVelocity(PxVec3 vel);

public:
	PxVec3 m_Pos;
	PxVec3 m_Vel;

	PxCapsuleController *m_PlayerController;
	PlayerHitReport *m_HitReport;
};

