// Fill out your copyright notice in the Description page of Project Settings.

#include "SoftDesignTraining.h"
#include "SDTAIController.h"
#include <EngineGlobals.h>
#include <Runtime/Engine/Classes/Engine/Engine.h>
#include "DrawDebugHelpers.h"

#include "SDTCollectible.h"


/*
 * TO REMOVE
 * N'hesitez pas a me contacter pour n'importe quoi. Voici quelques info qui peuvent vous aider:
 *
 * Voir CalculateAccelerationAndMaxSpeed pour le facteur d'acceleration et le max speed. Max speed est une valeur entre 0.0f et 1.0f.
 * Alors que le acceleration rate est multipli� par le delta time.
 *
 * Voir mRotationFactor, mSearchDistanceForward et mSearchDistanceSides dans le .h.
 * mRotationFactor est multipli� par le delta time et change la vitesse a laquelle le personnage tourne
 * mSearchDistanceForward et mSearchDistanceSides change les distances de recheche de murs et de death trapes (sont utilis�s aussi par les debuglines)
 *
 * Pour l'instant le AI se deplace de facon orthogonale selon son etat mCurrentMovement (Up = 0 degrees, Down = 180 degrees, Left = -90 degrees, Right = 90 degrees)
 * Lorsqu'un obstacle le bloque, son etat mNextMovement indique dans quelle direction il doit essayer de se diriger,
 * et son etat mCurrentMovement devient soit Turning90Degrees ou Turning180Degrees, qui applique une rotation graduelle vers l'angle d�sir�
 *
 * Pour Youssef :
 * J'ai mis un choix "PickingUp" dans l'enum de type de Movement. Tu pourrais te servir de cette option dans mCurrentMovement afin d'utiliser des angles
 * custom afin de te diriger vers un pick up que tu as detecter. Si tu te sers de mNextMovement pour storer mCurrentMovement avant de le passer
 * a "PickingUp", tu pourrais t'en servir pour rediriger le AI dans la bonne direction orthogonale apres qu'il ait pick up son truc (mCurrentMovement = mNextMovement)
 * 
 * Pour Anoir :
 * J'ai fait la logistique de detection de murs et de death traps ainsi que le comportement de base pour tourner a 90 degres ou 180 degree.
 * Pour l'instant, lorsqu'il est possible de tourner a gauche ou a droite, ca choisit une direction au random. Tu peux aller changer ca dans
 * la fonction ChooseDirectionToTake afin que le AI choisisse la meilleure direction selon son ChasingState.
 * Aussi, je n'ai pas fait la partie ou le AI rencontre une intersection alors qu'il n'a pas d'obstacle devant lui (il peut soit tourner ou continuer tout droit).
 * Je te laisse cette partie.
 *
 */


void ASDTAIController::Tick(float deltaTime)
{
	if (mCurrentMovement != Turning90Degrees && mCurrentMovement != Turning180Degrees && mCurrentMovement != PickingUp)
	{
		UWorld* world = GetWorld();
		APawn* pawn = GetPawn();
		FVector pawnPosition = pawn->GetActorLocation();

		//DEBUG LINES
		DrawDebugLine(world, pawnPosition, pawnPosition + pawn->GetActorForwardVector() * mSearchDistanceForward, FColor::Red, false, -1.0f, 000, 5.0f);
		DrawDebugLine(world, pawnPosition, pawnPosition + pawn->GetActorRightVector() * mSearchDistanceSides, FColor::Green, false, -1.0f, 000, 5.0f);
		DrawDebugLine(world, pawnPosition, pawnPosition - pawn->GetActorRightVector() * mSearchDistanceSides, FColor::Blue, false, -1.0f, 000, 5.0f);
		
		//pour l'instant le personnage va en ligne droite tant qu'il ne detecte pas un mur en avant de lui
		//il faudra traiter les intersections ou il est possible soit de tourner ou soit de continuer tout droit
		bool obstacleForward = SearchObstacleForward(world, pawn, pawnPosition);
		if (obstacleForward)
		{
			bool canTurnRight = CanTurnRight(world, pawn, pawnPosition);
			bool canTurnLeft = CanTurnLeft(world, pawn, pawnPosition);
			CalculateTurningDirection(canTurnRight, canTurnLeft, obstacleForward);
		}
        else {
            DrawDebugSphere(GetWorld(), GetPawn()->GetActorLocation(), 300, 50, FColor::Red);
        }
        CorrectPathDirection(deltaTime);
        DebugLog("Pawn " + GetPawn()->GetActorLabel() + " is advancing");
	}

	if (mCurrentMovement == Turning90Degrees || mCurrentMovement == Turning180Degrees)
	{
		if (CalculateSpeedVectorRotation(deltaTime, GetTurningAngle()))
			mCurrentMovement = mNextMovement;
            DebugLog("Pawn " + GetPawn()->GetActorLabel() + " is turning");
	}

    if (mCurrentMovement == PickingUp)
    {
        if (mNextMovement == Turning90Degrees || mNextMovement == Turning180Degrees)
        {
            mCurrentMovement = mNextMovement;
        }
        else
        {
            //mNextMovement = Turning180Degrees;
            CalculateSpeedVectorRotation(deltaTime, FMath::RadiansToDegrees(mVectorToReach.HeadingAngle()));
            //mCurrentMovement = mNextMovement;
            mCurrentMovement = Right;
            //mCurrentMovement = Right;
            DebugLog("pawn " + GetPawn()->GetActorLabel() + " is picking up");
        }
    }

	float acceleration;

	float maxSpeed;
	CalculateAccelerationAndMaxSpeed(acceleration, maxSpeed);
	CalculateSpeedVectorSpeed(deltaTime, acceleration, maxSpeed);
	DetectPickUP();


    /*{
        SweepSingleObstacle(GetWorld(), GetPawn()->GetActorLocation(), GetPawn()->GetActorLocation()
            + GetPawn()->GetActorForwardVector() * 20);
        bool obstacleForward = SweepSingleObstacle(GetWorld(), GetPawn()->GetActorLocation(), GetPawn()->GetActorLocation()
            + GetPawn()->GetActorForwardVector() * 20);
        if (obstacleForward)
        {
            bool canTurnRight = CanTurnRight(GetWorld(), GetPawn(), GetPawn()->GetActorLocation());
            bool canTurnLeft = CanTurnLeft(GetWorld(), GetPawn(), GetPawn()->GetActorLocation());
            
            CalculateTurningDirection(canTurnRight, canTurnLeft, obstacleForward);
        }




    }*/
}

void ASDTAIController::CorrectPathDirection(float deltaTime)
{
    switch (mCurrentMovement)
    {
    case Up:
        if (mSpeedVector != FVector::ForwardVector)
        {
            mSpeedVector = FVector::ForwardVector;
            CalculateSpeedVectorRotation(deltaTime, FMath::RadiansToDegrees(mVectorToReach.HeadingAngle()));
            DebugLog(GetPawn()->GetActorLabel() + " is correcting it's path forward");
        }
        break;
    case Down:
        if (mSpeedVector != -FVector::ForwardVector)
        {
            mSpeedVector = -FVector::ForwardVector;
            CalculateSpeedVectorRotation(deltaTime, FMath::RadiansToDegrees(mVectorToReach.HeadingAngle()));
            DebugLog(GetPawn()->GetActorLabel() + " is correcting it's path backward");
        }
        break;
    case Right:
        if (mSpeedVector != FVector::RightVector)
        {
            mSpeedVector = FVector::RightVector;
            CalculateSpeedVectorRotation(deltaTime, FMath::RadiansToDegrees(mVectorToReach.HeadingAngle()));
            DebugLog(GetPawn()->GetActorLabel() + " is correcting it's path right");
        }
        break;
    case Left:
        if (mSpeedVector != -FVector::RightVector)
        {
            mSpeedVector = -FVector::RightVector;
            CalculateSpeedVectorRotation(deltaTime, FMath::RadiansToDegrees(mVectorToReach.HeadingAngle()));
            DebugLog(GetPawn()->GetActorLabel() + " is correcting it's path left");
        }
        break;
    default:
        if (mSpeedVector != FVector::ForwardVector)
        {
            mSpeedVector = FVector::ForwardVector;
            CalculateSpeedVectorRotation(deltaTime, FMath::RadiansToDegrees(mVectorToReach.HeadingAngle()));
            DebugLog(GetPawn()->GetActorLabel() + " is correcting it's path forward");
        }
        break;
    }
}

/**
 * Rotate the speed vector around the Z axis.
 *
 * This rotate the speed vector gradually until it reach the desired angle.
 *
 * @param deltaTime Time between ticks
 * @param angleToReach Desired world angle (0.0f is Up, 180.0f is Down, 90.0f is Right, -90.0f is Left)
 * @return true if the rotation is complete
 */
bool ASDTAIController::CalculateSpeedVectorRotation(float deltaTime, float angleToReach)
{
	float currentAngle = GetPawn()->GetActorRotation().Yaw;
	if (!FMath::IsNearlyEqual(currentAngle, angleToReach, 0.01f))
	{
		float angleVariation = mRotationFactor * deltaTime;
		float differenceAbsolue = FMath::Abs(currentAngle - angleToReach);

		if (angleVariation > differenceAbsolue)
			angleVariation = differenceAbsolue;

		if (angleToReach < currentAngle)
			angleVariation *= -1.0f;

		if (differenceAbsolue > 180.0f)
			angleVariation *= -1.0f;

		mSpeedVector = mSpeedVector.RotateAngleAxis(angleVariation, FVector(0.0f, 0.0f, 1.0f));
		CalculatePawnOrientation();

		return false;
	}

	return true;
}

/**
 * Rotate the pawn towards the speed vector.
 *
 * This rotate the pawn so that it face the way it is moving.
 */
void ASDTAIController::CalculatePawnOrientation()
{
	FVector lookAt = GetPawn()->GetActorForwardVector();
	float angle = FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(lookAt, mSpeedVector) / mSpeedVector.Size()));
	float orientation = lookAt.X * mSpeedVector.Y - lookAt.Y * mSpeedVector.X;

	if (orientation > 0.0f)
		orientation = 1.0f;
	else
		orientation = -1.0f;

	if (angle == 180.0f)
		GetPawn()->AddActorWorldRotation(FRotator(0.0f, angle, 0.0f));
	else if (angle > 0.0f)
		GetPawn()->AddActorWorldRotation(FRotator(0.0f, orientation * angle, 0.0f));
}

/**
 * Calculate the speed vector speed.
 *
 * This accelerate (or decelerate) the speed vector until it reach its max speed.
 *
 * @param deltaTime Time between ticks
 * @param acceleration Acceleration rate
 * @param maxSpeed Max Speed (between 0.0f and 1.0f)
 */
void ASDTAIController::CalculateSpeedVectorSpeed(float deltaTime, float acceleration, float maxSpeed)
{
	if (acceleration >= 0.0f)
	{
		if (mSpeedScale < maxSpeed)
			mSpeedScale += acceleration * deltaTime;

		if (mSpeedScale > maxSpeed)
			mSpeedScale = maxSpeed;
	}
	else
	{
		if (mSpeedScale > maxSpeed)
			mSpeedScale += acceleration * deltaTime;

		if (mSpeedScale < maxSpeed)
			mSpeedScale = maxSpeed;
	}
	
	GetPawn()->AddMovementInput(mSpeedVector, mSpeedScale, false);	
}

/**
 * Select the correct movement states.
 *
 * Select the correct states for the next movement and current movement states.
 * No turn option avec obstacleForward = 180
 * No turn option sans obstacleForward = pas de changement de direction
 *
 * @param turnOptionRight Whether it can turn right or not.
 * @param turnOptionLeft Whether it can turn left or not.
 * @param obstacleForward Whether or not there is a blocking obstacle forward.
 */
void ASDTAIController::CalculateTurningDirection(bool turnOptionRight, bool turnOptionLeft, bool obstacleForward)
{
	//Choose direction depending on chasing state
	ChooseDirectionToTake(turnOptionRight, turnOptionLeft, obstacleForward);

	//the only options is to do a 180
	if (!turnOptionRight && !turnOptionLeft && obstacleForward)
	{
		switch (mCurrentMovement)
		{
		case Up:
			mNextMovement = Down;
			break;
		case Down:
			mNextMovement = Up;
			break;
		case Right:
			mNextMovement = Left;
			break;
		case Left:
			mNextMovement = Right;
			break;
		default:
			mNextMovement = Up;
			break;
		}

		mCurrentMovement = Turning180Degrees;
	}
	else if (turnOptionRight)
	{
		switch (mCurrentMovement)
		{
		case Up:
			mNextMovement = Right;
			break;
		case Down:
			mNextMovement = Left;
			break;
		case Right:
			mNextMovement = Down;
			break;
		case Left:
			mNextMovement = Up;
			break;
		default:
			mNextMovement = Right;
			break;
		}

		mCurrentMovement = Turning90Degrees;
	}
	else if (turnOptionLeft)
	{
		switch (mCurrentMovement)
		{
		case Up:
			mNextMovement = Left;
			break;
		case Down:
			mNextMovement = Right;
			break;
		case Right:
			mNextMovement = Up;
			break;
		case Left:
			mNextMovement = Down;
			break;
		default:
			mNextMovement = Up;
			break;
		}

		mCurrentMovement = Turning90Degrees;
	}
}

/**
 * Get Turning Angle
 *
 * Get the world angle that the character is trying to reach
 *
 * @return the world angle that the character is trying to reach
 */
float ASDTAIController::GetTurningAngle()
{
	switch (mNextMovement)
	{
	case Up:
		return 0.0f;
		break;
	case Down:
		return 180.0f;
		break;
	case Right:
		return 90.0f;
		break;
	case Left:
		return -90.0f;
		break;
	default:
		return 0.0f;
		break;
	}
}

/**
 * Search Obstacle Forward
 *
 * This search for a wall or a death trap forward
 *
 * @param world The world that this is in
 * @param pawn This controller's pawn
 * @param pawnPosition The position of this controller's pawn
 * @return true if an obstacle have been found forward
 */
bool ASDTAIController::SearchObstacleForward(UWorld* world, APawn* pawn, FVector pawnPosition)
{
	if (SweepSingleObstacle(world, pawnPosition, pawnPosition + pawn->GetActorForwardVector() * mSearchDistanceForward))
	{
		DebugLog("Obstacle Devant");
		return true;
	}

	return false;	
}

/**
 * Search Obstacle on the right
 *
 * This search for a wall or a death trap on the right side of the character
 *
 * @param world The world that this is in
 * @param pawn This controller's pawn
 * @param pawnPosition The position of this controller's pawn
 * @return true if an obstacle have been found on the right side of the character
 */
bool ASDTAIController::CanTurnRight(UWorld* world, APawn* pawn, FVector pawnPosition)
{
	if (SweepSingleObstacle(world, pawnPosition, pawnPosition + pawn->GetActorRightVector() * mSearchDistanceSides))
	{
		DebugLog("Obstacle Droite");
	}
	else
	{
		DebugLog("Pas Obstacle Droite");
		return true;
	}

	return false;
}

/**
 * Search Obstacle on the left
 *
 * This search for a wall or a death trap on the left side of the character
 *
 * @param world The world that this is in
 * @param pawn This controller's pawn
 * @param pawnPosition The position of this controller's pawn
 * @return true if an obstacle have been found on the left side of the character
 */
bool ASDTAIController::CanTurnLeft(UWorld* world, APawn* pawn, FVector pawnPosition)
{
	if (SweepSingleObstacle(world, pawnPosition, pawnPosition - pawn->GetActorRightVector() * mSearchDistanceSides))
	{
		//DebugLog("Obstacle Gauche");
	}
	else
	{
		//DebugLog("Pas Obstacle Gauche");
		return true;
	}

	return false;
}

/**
 * Sweep Single Obstacle
 *
 * This sweeps a shape the same dimension of the character's capsule in a direction
 * and looks for an obstacle (a static object like a wall, or a DeathObject like the death traps).
 *
 * @param world TThe world that this is in
 * @param start Where the sweep starts
 * @param end Where the sweep ends
 * @return true if an obstacle have been found
 */
bool ASDTAIController::SweepSingleObstacle(UWorld* world, FVector start, FVector end)
{
	FHitResult firstHit;
	UCapsuleComponent* capsule = GetCharacter()->GetCapsuleComponent();
	FCollisionShape shape = FCollisionShape::MakeCapsule(capsule->GetScaledCapsuleRadius(), capsule->GetScaledCapsuleHalfHeight());
	FCollisionObjectQueryParams objectQueryParams;
	objectQueryParams.AddObjectTypesToQuery(ECollisionChannel::ECC_WorldStatic);
	objectQueryParams.AddObjectTypesToQuery(ECollisionChannel::ECC_GameTraceChannel3); //DeathObject
	FCollisionQueryParams queryParams = FCollisionQueryParams();
	bool detected = world->SweepSingleByObjectType(firstHit, start, end, FRotator(0.0f, 0.0f, 0.0f).Quaternion(), objectQueryParams, shape, queryParams);
	return detected && firstHit.bBlockingHit;
}

/**
 * Choose Direction To Take
 *
 * This Choose the correct direction to take depending on the chasing state of the AI.
 * It tries to get away from the player character if the mChasingState is "Fleeing".
 * It tries to chase the player character if the mChasingState is "Chasing".
 *
 * If obstacleForward is true, then to continue forward cannot be an option.
 * So if turnOptionRight and turnOptionLeft are both false, the AI will do a 180.
 *
 * If obstacleForward is false, then it can possibly continue forward.
 * So if turnOptionRight and turnOptionLeft are both false, the AI will continue forward.
 *
 * In both cases, let only one of turnOptionRight or turnOptionLeft to true to make the AI turn that way.
 *
 * @param turnOptionRight Whether it can turn right or not (this is passed by reference and can be changed to false).
 * @param turnOptionLeft Whether it can turn left or not (this is passed by reference and can be changed to false).
 * @param obstacleForward Whether or not there is a blocking obstacle forward.
 */
void ASDTAIController::ChooseDirectionToTake(bool& turnOptionRight, bool& turnOptionLeft, bool obstacleForward)
{
	//TODO Anoir : a faire la logique selon si chasing or fleeing. Pour l'instant ca utilise du random
	if (obstacleForward)
	{
		if (turnOptionRight && turnOptionLeft)
		{
			if (FMath::RandBool())
				turnOptionLeft = false;
			else
				turnOptionRight = false;
		}
	}
	else
	{
		if (turnOptionRight && turnOptionLeft)
		{
			int random = FMath::RandRange(1, 3);
			if (random == 1)
			{
				turnOptionLeft = false;
				turnOptionRight = false;
			}
			else if (random == 2)
			{
				turnOptionLeft = false;
			}
			else if (random == 3)
			{
				turnOptionRight = false;
			}
		}
		else
		{
			if (FMath::RandBool())
			{
				turnOptionLeft = false;
				turnOptionRight = false;
			}
		}		
	}
}

/**
 * Calculate Acceleration And Max Speed
 *
 * This calculate the correct acceleration and maximum speed depending on the states of the AI.
 *
 * @param acceleration Acceleration rate
 * @param maxSpeed Max Speed (between 0.0f and 1.0f)
 */
void  ASDTAIController::CalculateAccelerationAndMaxSpeed(float& acceleration, float& maxSpeed)
{
	switch (mChasingState)
	{
	case Chasing:
		if (mCurrentMovement == Turning90Degrees)
		{
			acceleration = -12.0f;
			maxSpeed = 0.5f;
		}
		else if (mCurrentMovement == Turning180Degrees)
		{
			acceleration = -120.0f;
			maxSpeed = 0.0f;
		}
		else
		{
			acceleration = 1.2f;
			maxSpeed = 1.0f;
		}
		break;

	case Fleeing:
		if (mCurrentMovement == Turning90Degrees)
		{
			acceleration = -12.0f;
			maxSpeed = 0.5f;
		}
		else if (mCurrentMovement == Turning180Degrees)
		{
			acceleration = -120.0f;
			maxSpeed = 0.0f;
		}
		else
		{
			acceleration = 1.2f;
			maxSpeed = 1.0f;
		}
		break;

	default:
		acceleration = 1.2f;
		maxSpeed = 1.0f;
		break;
	}
}

TArray<FOverlapResult> ASDTAIController::SphereOverLap(float radius, FVector position)
{
	TArray<FOverlapResult> results;
	FCollisionObjectQueryParams collisionObjectParams;
	FCollisionQueryParams collisionQueryParams = FCollisionQueryParams::DefaultQueryParam;
	FCollisionShape collisionShape;
	collisionShape.SetSphere(radius);
	GetWorld()->OverlapMultiByObjectType(results,position,FQuat::Identity,collisionObjectParams,collisionShape,collisionQueryParams);
	DrawDebugSphere(GetWorld(),position, radius,50,FColor::Green);

	return results;
}

void ASDTAIController::DetectPickUP()
{
	float radius = 250.0f;
	FVector pos = GetPawn()->GetActorLocation() + GetPawn()->GetActorForwardVector() * radius;
	TArray<FOverlapResult> results = SphereOverLap(radius,pos);
	TArray<AActor*> pickUps;
	
	for (int i = 0; i < results.Num(); ++i)
	{
		AActor* actor = results[i].Actor.Get();
        if (actor->GetActorLabel().Contains("Collectible") && !actor->bHidden)
		{
            ASDTCollectible* collectible = Cast<ASDTCollectible>(actor);

            if (!collectible->IsOnCooldown()) {

                /*FHitResult outHit;
                FVector pawnLocation = GetPawn()->GetActorLocation();
                FCollisionObjectQueryParams collisionObjectQueryParams;
                FCollisionQueryParams collisionQueryParams;
                collisionQueryParams.AddIgnoredActor(GetPawn());
                //collisionObjectQueryParams.AddObjectTypesToQuery(ECollisionChannel::ECC_GameTraceChannel3);
                collisionObjectQueryParams.AddObjectTypesToQuery(ECollisionChannel::ECC_WorldStatic);
                GetWorld()->LineTraceSingleByObjectType(outHit, pawnLocation, actor->GetActorLocation(), collisionObjectQueryParams,collisionQueryParams);

                DrawDebugLine(GetWorld(), pawnLocation, actor->GetActorLocation(), FColor::Red);*/
                //DebugLog("pickup detected" + outHit.Actor.Get()->GetActorLabel());

                /*if (outHit.Actor == nullptr)
                {
                    pickUps.Add(actor);
                }*/
                if (!SweepSingleObstacle(GetWorld(), GetPawn()->GetActorLocation(), actor->GetActorLocation())) {
                    pickUps.Add(actor);
                }
            }
		}
	}

	AActor* closestActor = nullptr;
	float closestDistance = -1.f;

	for (int i = 0; i < pickUps.Num(); ++i)
	{
		float newDistance = (pickUps[i]->GetActorLocation() - GetPawn()->GetActorLocation()).Size();

		if (closestActor == nullptr || (closestDistance > newDistance))
		{
			closestActor = pickUps[i];
			closestDistance = newDistance;
		}
	}

	if (closestActor != nullptr)
	{
        DebugLog("pickup detected by " + GetPawn()->GetActorLabel());
        DrawDebugLine(GetWorld(), GetPawn()->GetActorLocation(), closestActor->GetActorLocation(), FColor::Red);

		mNextMovement = mCurrentMovement;
		mCurrentMovement = PickingUp;

		CalculateDirectionToPickUP(closestActor);
	}
    else
    {
        if (mCurrentMovement == PickingUp)
            mCurrentMovement = Left;
    }
}

void ASDTAIController::CalculateDirectionToPickUP(AActor* target)
{
	FVector pawnPosition = GetPawn()->GetActorLocation();
	FVector pickUpPosition = target->GetActorLocation();
	FVector direction = pickUpPosition - pawnPosition;
	direction.Normalize();
	//mAngleToReach = FMath::RadiansToDegrees(direction.HeadingAngle());
	mSpeedVector = direction;
    DrawDebugSphere(GetWorld(), GetPawn()->GetActorLocation(), 350, 50, FColor::Blue);
}

/**
 * Debug Log
 *
 * TEMP SHIT TO REMOVE : Write funny stuff in the output log in Unreal Engine
 *
 * @param s The fkn string
 */
void ASDTAIController::DebugLog(FString s)
{
	UE_LOG(LogTemp, Log, TEXT("%s"), *s);
}




