// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SDTBaseAIController.h"
#include "SDTAIController.generated.h"

#define WALK_ANIMATION_SPEED 300.0f
#define RUN_ANIMATION_SPEED 600.0f

enum Behaviour
{
	PickingUp,
	Chase,
	Flee,
	Fleeing
};

/**
 * 
 */
UCLASS(ClassGroup = AI, config = Game)
class SOFTDESIGNTRAINING_API ASDTAIController : public ASDTBaseAIController
{
	GENERATED_BODY()

public:
    ASDTAIController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AI)
    float m_DetectionCapsuleHalfLength = 500.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AI)
    float m_DetectionCapsuleRadius = 250.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AI)
    float m_DetectionCapsuleForwardStartingOffset = 100.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AI)
    UCurveFloat* JumpCurve;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AI)
    float JumpApexHeight = 300.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AI)
    float JumpSpeed = 1.f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = AI)
    bool AtJumpSegment = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = AI)
    bool InAir = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = AI)
    bool Landing = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = AI)
	float AnimationNavigationSpeed = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = AI)
	bool StartJumping;

public:
    virtual void OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result) override;
    void AIStateInterrupted();


protected:
    void OnMoveToTarget();
    void GetHightestPriorityDetectionHit(const TArray<FHitResult>& hits, FHitResult& outDetectionHit);
    void UpdatePlayerInteraction(float deltaTime);

private:
    virtual void GoToBestTarget(float deltaTime) override;
	virtual void ChooseBehavior(float deltaTime) override;
    virtual void ShowNavigationPath() override;
	FVector GetFleeLocation();
	void DefineState(FHitResult HitResult);
	void CalculateSpeed(float deltaTime);

	void PickingBehaviour(float deltaTime);
	bool SweepSingleObstacle(FVector Start, FVector End) const;
	void Jump(float deltaTime);
	Behaviour m_state = Behaviour::PickingUp;
	FVector m_playerPos = FVector::ZeroVector;
	FVector m_FleePos = FVector::ZeroVector;
	FVector m_target = FVector::ZeroVector;
};