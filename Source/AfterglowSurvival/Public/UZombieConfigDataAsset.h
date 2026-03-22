// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "UZombieConfigDataAsset.generated.h"

/**
 * 
 */
UCLASS()
class AFTERGLOWSURVIVAL_API UUZombieConfigDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
    UUZombieConfigDataAsset();

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Zombie|Core")
    float MaxHealth;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Zombie|Core")
    float ThinkInterval;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Zombie|Target")
    float TargetRefreshInterval;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Zombie|Target")
    float TargetSearchRadius;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Zombie|Combat")
    float AttackRange;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Zombie|Combat")
    float AttackDamage;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Zombie|Combat")
    float AttackCooldown;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Zombie|Combat")
    float AttackHitRadius;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Zombie|Movement")
    float MoveAcceptanceRadius;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Zombie|Movement")
    float NoPathFallbackRadius;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Zombie|Movement")
    int32 MaxConsecutivePathFailures;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Zombie|Movement")
    float RepathCooldown;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Zombie|Pool")
    bool bReturnToPoolOnDeath;
};
