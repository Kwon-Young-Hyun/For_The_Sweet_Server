#pragma once
#include "header.h"
#include "PxPhysicsAPI.h"
#include "Util.h"

using namespace physx;

class CPhysx {
public:
	// PxFoundation�� �����ϴ� Allocator, ErrorCallback
	physx::PxDefaultAllocator m_Allocator;
	physx::PxDefaultErrorCallback m_ErrorCallback;

	// Px�� �ֻ��� ��Ʈ(Foundation), Px�� ������ ��Ʈ(Physics)
	physx::PxFoundation* m_Foundation;
	physx::PxPhysics* m_Physics;

	// �浹�߻� ���θ� �˷��ִ� dispatcher����
	physx::PxDefaultCpuDispatcher* m_Dispatcher;

	// �������� Ȱ���� å���� Scene
	physx::PxScene*	m_Scene;

	// Physics�� �� matarial(���� �������, ���� �������, ź�����)
	physx::PxMaterial* m_Material;

	// Player�� ������ manager
	physx::PxControllerManager *m_PlayerManager;
	// Player�� �����ϴ� manager�� ������ controller
	//physx::PxController *m_PlayerController;

	// �丮
	physx::PxCooking* m_Cooking;

	//PlayerHitReport hitreport;

	// Player �浹 ������ Capsule Or Box
	//physx::PxCapsuleControllerDesc m_CapsuleDesc;
	//physx::PxBoxControllerDesc m_BoxDesc;

public:
	CPhysx();
	~CPhysx() = default;

	void initPhysics();
	void move(int direction, float distance);

	PxTriangleMesh*	GetTriangleMesh(vector<PxVec3> ver, vector<int> index);
	PxCapsuleController* getCapsuleController(PxVec3 pos, float height, float radius, PxUserControllerHitReport* collisionCallback);
};

inline PxExtendedVec3 PXtoPXEx(const PxVec3& pos) {
	return PxExtendedVec3(pos.x, pos.y, pos.z);
}

inline PxVec3 PXExtoPX(const PxExtendedVec3& pos) {
	return PxVec3(pos.x, pos.y, pos.z);
}

inline PxVec3 Normalize(PxVec3& vec)
{
	PxVec3 Normal;
	
	float vec_size = sqrt(vec.x*vec.x + vec.y*vec.y + vec.z*vec.z);
	Normal.x = vec.x / vec_size;
	Normal.y = vec.y / vec_size;
	Normal.z = vec.z / vec_size;

	return(Normal);
}
