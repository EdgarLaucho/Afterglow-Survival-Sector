// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "UZombieTargetRegistrySubsystem.generated.h"

/**
 * 
 */
UCLASS()
class AFTERGLOWSURVIVAL_API UUZombieTargetRegistrySubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Zombie|TargetRegistry")
    AActor* GetClosestTarget(const FVector& FromLocation, const APawn* Requester, float MaxDistance);

private:
    UPROPERTY()
    TArray<TWeakObjectPtr<APawn>> CachedTargets;

    float LastRefreshTime = -1000.f;

    UPROPERTY(EditAnywhere, Category = "Zombie|TargetRegistry")
    float RefreshInterval = 0.25f;

    void RefreshTargets();
    bool IsValidTargetPawn(const APawn* Pawn, const APawn* Requester) const;
};
