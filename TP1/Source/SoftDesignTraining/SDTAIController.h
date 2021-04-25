// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"

#include "SDTAIController.generated.h"

/**
 * 
 */
UCLASS(ClassGroup = AI, config = Game)
class SOFTDESIGNTRAINING_API ASDTAIController : public AAIController
{
    GENERATED_BODY()
public:
    virtual void Tick(float deltaTime) override;
	bool CalculateSpeedVectorRotation(float deltaTime, float angleToReach);
	void CalculatePawnOrientation();
	void CalculateSpeedVectorSpeed(float deltaTime, float acceleration, float maxSpeed);
	void CalculateTurningDirection(bool turnOptionRight, bool turnOptionLeft, bool obstacleForward);
	bool SweepSingleObstacle(UWorld* world, FVector start, FVector end);
	bool SearchObstacleForward(UWorld* world, APawn* pawn, FVector pawnPosition);
	bool CanTurnRight(UWorld* world, APawn* pawn, FVector pawnPosition);
	bool CanTurnLeft(UWorld* world, APawn* pawn, FVector pawnPosition);
	void CalculateAccelerationAndMaxSpeed(float& acceleration, float& maxSpeed);
	void ChooseDirectionToTake(bool& turnOptionRight, bool& turnOptionLeft, bool obstacleForward);
	void DetectPickUP();
	void CalculateDirectionToPickUP(AActor* target);
	TArray<FOverlapResult> SphereOverLap(float radius, FVector position);
	float GetTurningAngle();
    void CorrectPathDirection(float deltaTime);
	void DebugLog(FString s);

	//variables a ajuster
	float mRotationFactor = 500.0f;
	float mSearchDistanceForward = 150.0;
	float mSearchDistanceSides = 400.0f;

	enum ChasingState
	{
		Chasing,
		Fleeing
	};

	enum Movement
	{
		Up,
		Down,
		Left,
		Right,
		Turning90Degrees,
		Turning180Degrees,
		PickingUp
	};

private:	
	ChasingState mChasingState = Chasing;
	Movement mCurrentMovement = Up;
	Movement mNextMovement;
	float mSpeedScale = 0.0f;
	FVector mSpeedVector = FVector(1.0f, 0.0f, 0.0f);
	FVector mVectorToReach = FVector::ZeroVector;
	float mAngleToReach = 0.0f;
};
