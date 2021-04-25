// Fill out your copyright notice in the Description page of Project Settings.

#include "SoftDesignTraining.h"
#include "SDTUtils.h"
#include "SoftDesignTrainingMainCharacter.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"

#include "Engine.h"

/*static*/ bool SDTUtils::Raycast(UWorld* uWorld, FVector sourcePoint, FVector targetPoint)
{
    FHitResult hitData;
    FCollisionQueryParams TraceParams(FName(TEXT("VictoreCore Trace")), true);
    
    // Fake cost for the exercise
    //Sleep(1);
    // End fake cost

    return uWorld->LineTraceSingleByChannel(hitData, sourcePoint, targetPoint, ECC_Pawn, TraceParams);
}

bool SDTUtils::IsPlayerPoweredUp(UWorld * uWorld)
{
    ACharacter* playerCharacter = UGameplayStatics::GetPlayerCharacter(uWorld, 0);
    if (!playerCharacter)
        return false;

    ASoftDesignTrainingMainCharacter* castedPlayerCharacter = Cast<ASoftDesignTrainingMainCharacter>(playerCharacter);
    if (!castedPlayerCharacter)
        return false;

    return castedPlayerCharacter->IsPoweredUp();
}

/**
 * 
 * @brief 
 * @param Radius 
 * @param Position 
 * @param CollisionChannel 
 * @return 
 */
TArray<FOverlapResult> SDTUtils::SphereOverLap(UWorld* world, float radius, FVector position, ECollisionChannel collisionChannel)
{
	TArray<FOverlapResult> results;
    FCollisionObjectQueryParams collisionObjectQueryParams;
	//collisionObjectQueryParams.AddObjectTypesToQuery(CollisionChannel);
	const FCollisionQueryParams collisionQueryParams = FCollisionQueryParams::DefaultQueryParam;
	FCollisionShape collisionShape;
	collisionShape.SetSphere(radius);
	world->OverlapMultiByObjectType(results, position, FQuat::Identity, collisionObjectQueryParams, collisionShape, collisionQueryParams);

	return results;
}

void SDTUtils::DebugLog(FString str)
{
	UE_LOG(LogTemp, Log, TEXT("%s"), *str);
}
