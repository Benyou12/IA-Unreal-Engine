// Fill out your copyright notice in the Description page of Project Settings.

#include "SoftDesignTraining.h"
#include "SDTPathFollowingComponent.h"
#include "SDTUtils.h"
#include "SDTAIController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "DrawDebugHelpers.h"

USDTPathFollowingComponent::USDTPathFollowingComponent(const FObjectInitializer& ObjectInitializer)
{

}

void USDTPathFollowingComponent::FollowPathSegment(float DeltaTime)
{
	const TArray<FNavPathPoint>& points = Path->GetPathPoints();

	if (!mIsJumping)
	{
		mSegmentStart = points[MoveSegmentStartIndex];

		if (MoveSegmentStartIndex < points.Num())
			mSegmentEnd = points[MoveSegmentStartIndex + 1];
	}

	ASDTAIController* ai = dynamic_cast<ASDTAIController*>(GetOwner());
	FVector start = mSegmentStart.Location;
	FVector end = mSegmentEnd.Location;

	if (SDTUtils::HasJumpFlag(mSegmentStart) || mIsJumping)
	{
		ai->AtJumpSegment = true;

		FVector2D toEnd = FVector2D(end - ai->GetPawn()->GetActorLocation());
		toEnd.Normalize();
		FVector forward = ai->GetPawn()->GetActorForwardVector();
		forward.Normalize();
		bool isOnDirection = std::abs(FMath::Acos(FVector::DotProduct(forward, FVector(toEnd, 0.0f)))) <= 0.25;
		if (!isOnDirection && !ai->StartJumping)
		{
			const FRotator pawnRotator = ai->GetPawn()->GetActorRotation();
			const  float alpha = 0.1f;
			const FRotator rotator = FMath::Lerp(pawnRotator, FVector(toEnd, 0.f).Rotation(), alpha);
			ai->GetPawn()->SetActorRotation(rotator);
			return;
		}

		ai->Landing = false;

		if (!mIsJumping)
		{
			mIsJumping = true;
			mJumpStartPoint = FVector2D(mSegmentStart.Location);
			mJumpEndPoint = FVector2D(points[MoveSegmentStartIndex + 1].Location);
			mJumpDirection = FVector2D(mJumpEndPoint - mJumpStartPoint);
			mJumpDirection.Normalize();
			return;
		}

		ai->StartJumping = true;

		FVector2D previousActorLocation = FVector2D(MovementComp->GetActorFeetLocation());
		FVector2D nextActorLocation = previousActorLocation + mJumpDirection * ai->AnimationNavigationSpeed * DeltaTime;

		float percentage = FVector2D::Distance(nextActorLocation, mJumpStartPoint) / FVector2D::Distance(mJumpStartPoint, mJumpEndPoint);
		float Z = ai->JumpCurve->GetFloatValue(percentage) * ai->JumpApexHeight + 216.0f;

		ai->GetPawn()->SetActorLocation(FVector(nextActorLocation, Z));

		ai->GetPawn()->AddMovementInput(FVector(mJumpDirection, 0.f), 1.0f);

		if (Z >= 45)
			ai->InAir = true;

		if (Z <= 50)
			ai->Landing = true;

		if (percentage > 0.99)
		{
			ai->GetPawn()->SetActorLocation(end, true);
			mIsJumping = false;
			ai->AtJumpSegment = false;
			ai->StartJumping = false;
			ai->InAir = false;
			ai->Landing = true;
		}
	}
	else
	{
		Super::FollowPathSegment(DeltaTime);
		ai->Landing = false;
	}
}

void USDTPathFollowingComponent::SetMoveSegment(int32 segmentStartIndex)
{
	Super::SetMoveSegment(segmentStartIndex);

	if (MovementComp == NULL)
		return;

	const TArray<FNavPathPoint>& points = Path->GetPathPoints();

	const FNavPathPoint& segmentStart = points[MoveSegmentStartIndex];

	if ((SDTUtils::HasJumpFlag(segmentStart) && FNavMeshNodeFlags(segmentStart.Flags).IsNavLink()) || mIsJumping)
	{
		Cast<UCharacterMovementComponent>(MovementComp)->SetMovementMode(MOVE_Flying);
	}
	else
	{
		Cast<UCharacterMovementComponent>(MovementComp)->SetMovementMode(MOVE_Walking);
	}
}