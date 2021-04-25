// Fill out your copyright notice in the Description page of Project Settings.

#include "SoftDesignTraining.h"
#include "SDTCollectible.h"

ASDTCollectible::ASDTCollectible()
{
	static ConstructorHelpers::FObjectFinder<UParticleSystem> particleAsset(TEXT("/Game/StarterContent/Particles/P_Explosion"));
	static ConstructorHelpers::FObjectFinder<USoundWave> soundAsset(TEXT("/Game/StarterContent/Audio/Pickup_Sound"));
	particleEffect = particleAsset.Object;
	soundEffect = soundAsset.Object;
}

void ASDTCollectible::Collect()
{
	if (!IsOnCooldown()) {
		UGameplayStatics::PlaySound2D(GetWorld(), soundEffect);
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), particleEffect, GetActorLocation());
	}

    GetWorld()->GetTimerManager().SetTimer(m_CollectCooldownTimer, this, &ASDTCollectible::OnCooldownDone, m_CollectCooldownDuration, false);

    GetStaticMeshComponent()->SetVisibility(false);
}

void ASDTCollectible::OnCooldownDone()
{
    GetWorld()->GetTimerManager().ClearTimer(m_CollectCooldownTimer);

    GetStaticMeshComponent()->SetVisibility(true);
}

bool ASDTCollectible::IsOnCooldown()
{
    return m_CollectCooldownTimer.IsValid();
}
