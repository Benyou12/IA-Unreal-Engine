// Fill out your copyright notice in the Description page of Project Settings.

#include "SoftDesignTraining.h"
#include "SDTAIController.h"

#include <string>

#include "SDTCollectible.h"
#include "SDTFleeLocation.h"
#include "SDTPathFollowingComponent.h"
#include "DrawDebugHelpers.h"
#include "Kismet/KismetMathLibrary.h"
#include "UnrealMathUtility.h"
#include "SDTUtils.h"
#include "EngineUtils.h"
#include "SoftDesignTrainingCharacter.h"
#include "SoftDesignTrainingMainCharacter.h"

ASDTAIController::ASDTAIController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<USDTPathFollowingComponent>(TEXT("PathFollowingComponent")))
{
}

void ASDTAIController::GoToBestTarget(float deltaTime)
{
	ShowNavigationPath();
	switch (m_state)
	{
		case PickingUp:
			PickingBehaviour(deltaTime);
			break;
		case Chase:
			MoveToLocation(m_playerPos, 40);
			break;
		case Fleeing:
			MoveToLocation(m_FleePos, 0);
			break;
		default:
			break;
	}
}

void ASDTAIController::OnMoveToTarget()
{
	m_ReachedTarget = false;
}

void ASDTAIController::OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result)
{
	Super::OnMoveCompleted(RequestID, Result);

	m_ReachedTarget = true;
}

void ASDTAIController::ShowNavigationPath()
{
	//Show current navigation path DrawDebugLine and DrawDebugSphere
	if (!GetPathFollowingComponent()->GetPath().IsValid()) 
		return;

	FNavPathPoint lastNavPathPoint;
	const TArray<FNavPathPoint>& navPathPoints = GetPathFollowingComponent()->GetPath()->GetPathPoints();
	for (FNavPathPoint currP : navPathPoints)
	{
		DrawDebugSphere(GetWorld(), currP.Location, 45.f, 21, FColor::Yellow);
		if (lastNavPathPoint.HasNodeRef())
		{
			FVector from = lastNavPathPoint.Location;
			FVector to = currP.Location;
			DrawDebugLine(GetWorld(), from, to, FColor::Yellow);
		}
		lastNavPathPoint = currP;
	}

}

void ASDTAIController::ChooseBehavior(float deltaTime)
{
	UpdatePlayerInteraction(deltaTime);
}

void ASDTAIController::UpdatePlayerInteraction(float deltaTime)
{
	//finish jump before updating AI state
	if (AtJumpSegment)
		return;

	APawn* selfPawn = GetPawn();
	if (!selfPawn)
		return;

	ACharacter* playerCharacter = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
	if (!playerCharacter)
		return;

	FVector detectionStartLocation = selfPawn->GetActorLocation() + selfPawn->GetActorForwardVector() * m_DetectionCapsuleForwardStartingOffset;
	FVector detectionEndLocation = detectionStartLocation + selfPawn->GetActorForwardVector() * m_DetectionCapsuleHalfLength * 2;

	TArray<TEnumAsByte<EObjectTypeQuery>> detectionTraceObjectTypes;
	detectionTraceObjectTypes.Add(UEngineTypes::ConvertToObjectType(COLLISION_COLLECTIBLE));
	detectionTraceObjectTypes.Add(UEngineTypes::ConvertToObjectType(COLLISION_PLAYER));

	TArray<FHitResult> allDetectionHits;
	GetWorld()->SweepMultiByObjectType(allDetectionHits, detectionStartLocation, detectionEndLocation, FQuat::Identity, detectionTraceObjectTypes, FCollisionShape::MakeSphere(m_DetectionCapsuleRadius));

	FHitResult detectionHit;
	GetHightestPriorityDetectionHit(allDetectionHits, detectionHit);

	//Set behavior based on hit
	DefineState(detectionHit);

	CalculateSpeed(deltaTime);
	
	DrawDebugCapsule(GetWorld(), detectionStartLocation + m_DetectionCapsuleHalfLength * selfPawn->GetActorForwardVector(), m_DetectionCapsuleHalfLength, m_DetectionCapsuleRadius, selfPawn->GetActorQuat() * selfPawn->GetActorUpVector().ToOrientationQuat(), FColor::Blue);
}

void ASDTAIController::DefineState(FHitResult detectionHit) 
{
	bool detectedPlayer = false;
	bool isPowerdUp = false;
	const FVector pawPos = GetPawn()->GetActorLocation();

	ASoftDesignTrainingMainCharacter* player = dynamic_cast<ASoftDesignTrainingMainCharacter*>(detectionHit.GetActor());
	if (player != nullptr)
	{
		
		const FVector playerPos = detectionHit.GetActor()->GetActorLocation();
		if (!SweepSingleObstacle(pawPos, playerPos))
		{
			detectedPlayer = true;
			m_playerPos = playerPos;
			if (player->IsPoweredUp())
				isPowerdUp = true;
		}
	}

	switch (m_state)
	{
		case PickingUp:
		{
			SDTUtils::DebugLog("__PickingUp__");
			if (detectedPlayer)
				m_state = (isPowerdUp) ? Flee : Chase;
			break;
		}
		case Chase:
		{
			SDTUtils::DebugLog("__Chase__");
			if (detectedPlayer)
				m_state = (isPowerdUp) ? Flee : Chase;
			else if(FVector::Dist(pawPos, m_playerPos) < 110.0)
				m_state = PickingUp;
			break;
		}
		case Flee:
		{
			m_FleePos = GetFleeLocation();
			m_state = Fleeing;
			SDTUtils::DebugLog("__Flee__");
			break;
		}
		case Fleeing:
		{
			float dist = FVector::Dist(pawPos, m_FleePos);
			SDTUtils::DebugLog("__Fleeing__");
			if(dist < 300.0)
			{
				if (detectedPlayer)
					m_state = (isPowerdUp) ? Flee : Chase;
				else 
					m_state = PickingUp;
			}
			break;
		}
		default:
			break;
	}	
}

void ASDTAIController::CalculateSpeed(float deltaTime)
{
	float targetSpeed = RUN_ANIMATION_SPEED;
	if (m_state == PickingUp)
		targetSpeed = WALK_ANIMATION_SPEED;

	AnimationNavigationSpeed = SDTUtils::ExponentialDamp(AnimationNavigationSpeed, targetSpeed, 0.5f, deltaTime);
	GetCharacter()->GetCharacterMovement()->MaxWalkSpeed = AnimationNavigationSpeed;
	return;
}

void ASDTAIController::GetHightestPriorityDetectionHit(const TArray<FHitResult>& hits, FHitResult& outDetectionHit)
{
	for (const FHitResult& hit : hits)
	{
		if (UPrimitiveComponent* component = hit.GetComponent())
		{
			if (component->GetCollisionObjectType() == COLLISION_PLAYER)
			{
				//we can't get more important than the player
				outDetectionHit = hit;
				return;
			}
			else if (component->GetCollisionObjectType() == COLLISION_COLLECTIBLE)
			{
				outDetectionHit = hit;
			}
		}
	}
}

void ASDTAIController::AIStateInterrupted()
{
	StopMovement();
	m_ReachedTarget = true;
}


void ASDTAIController::Jump(float deltaTime)
{

}

void ASDTAIController::PickingBehaviour(float deltaTime)
{
	
	TArray<AActor*> collectibles;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ASDTCollectible::StaticClass(), collectibles);

	float nearstDist = 1e8;
	ASDTCollectible* nearestCollectible = nullptr;
	for (AActor* col : collectibles)
	{
		ASDTCollectible* collectible = dynamic_cast<ASDTCollectible*>(col);
		if(collectible != nullptr && !collectible->IsOnCooldown())
		{
			const FVector pawnPos = GetPawn()->GetActorLocation();
			const float dist = FVector::Dist(pawnPos, col->GetActorLocation());
			if (dist < nearstDist)
			{
				nearstDist = dist;
				nearestCollectible = collectible;
			}
		}
	}
	if (nearestCollectible!=nullptr)
		MoveToLocation(nearestCollectible->GetActorLocation(), 0);
}

/*
 * Determine if there is an obstacle between two vectors
 */
bool ASDTAIController::SweepSingleObstacle(FVector Start, FVector End) const
{
	FHitResult firstHit;
	UCapsuleComponent* capsule = GetCharacter()->GetCapsuleComponent();
	FCollisionShape shape = FCollisionShape::MakeCapsule(capsule->GetScaledCapsuleRadius(), capsule->GetScaledCapsuleHalfHeight());
	FCollisionObjectQueryParams objectQueryParams;
	objectQueryParams.AddObjectTypesToQuery(ECollisionChannel::ECC_WorldStatic);
	objectQueryParams.AddObjectTypesToQuery(COLLISION_DEATH_OBJECT); //DeathObject
	FCollisionQueryParams queryParams = FCollisionQueryParams();
	FQuat quat = FRotator(0.0f, 0.0f, 0.0f).Quaternion();
	bool detected = GetWorld()->SweepSingleByObjectType(firstHit, Start, End, quat, objectQueryParams, shape, queryParams);
	return detected && firstHit.bBlockingHit;
}

FVector ASDTAIController::GetFleeLocation()
{
	SDTUtils::DebugLog("__Get Flee Location__");
	TArray<AActor*> fleeLocations;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ASDTFleeLocation::StaticClass(), fleeLocations);
	float farestDist = -1;
	FVector fleeLoc = FVector::ZeroVector;
	for (auto loc : fleeLocations)
	{
		const FVector pawnPos = GetPawn()->GetActorLocation();
		const float dist = FVector::Dist(pawnPos, loc->GetActorLocation());
		if (dist > farestDist)
		{
			farestDist = dist;
			fleeLoc = loc->GetActorLocation();
		}
	}
	return fleeLoc;
}