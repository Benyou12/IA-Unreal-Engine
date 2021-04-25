// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "AI/Navigation/RecastNavMesh.h"

#define COLLISION_DEATH_OBJECT		ECollisionChannel::ECC_GameTraceChannel3
#define COLLISION_PLAYER        	ECollisionChannel::ECC_GameTraceChannel4
#define COLLISION_COLLECTIBLE     	ECollisionChannel::ECC_GameTraceChannel5

class SOFTDESIGNTRAINING_API SDTUtils
{
public:
    static bool Raycast(UWorld* uWorld, FVector sourcePoint, FVector targetPoint);
    static bool IsPlayerPoweredUp(UWorld* uWorld);

	static TArray<FOverlapResult> SphereOverLap(UWorld* world, const float Radius, const FVector Position, const ECollisionChannel CollisionChannel);

	static void DebugLog(FString str);

    enum NavType
    {
        Default,
        Jump
    };

    static bool IsNavTypeFlagSet(uint16 flags, NavType type) { return (flags & (1 << type)) != 0; }
    static void SetNavTypeFlag(uint16& flags, NavType type) { flags |= (1 << type); }

    static bool IsNavLink(const FNavPathPoint& PathVert) { return (FNavMeshNodeFlags(PathVert.Flags).PathFlags & RECAST_STRAIGHTPATH_OFFMESH_CONNECTION) != 0; }
    static bool HasJumpFlag(const FNavPathPoint& PathVert) { return     IsNavTypeFlagSet(FNavMeshNodeFlags(PathVert.Flags).AreaFlags, NavType::Jump); }
	static float ExponentialDamp(const float& val, const float& target, float damping, float dt) { return (((val * damping) + (target * dt)) / (damping + dt)); }
};
