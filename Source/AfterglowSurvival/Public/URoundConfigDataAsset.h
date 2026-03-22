// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "URoundConfigDataAsset.generated.h"

USTRUCT(BlueprintType)
struct FEnemyRoundSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round")
	int32 EnemyTypeIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round")
	int32 MaxTotalSpawns = 10;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round")
	int32 MaxAlive = 10;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round")
	float SpawnInterval = 1.0f;
};

USTRUCT(BlueprintType)
struct FRoundDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round")
	TArray<FEnemyRoundSettings> EnemySettings;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round")
	float IntermissionSeconds = 5.0f;
};

UCLASS()
class AFTERGLOWSURVIVAL_API UURoundConfigDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UURoundConfigDataAsset();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rounds")
	TArray<FRoundDefinition> Rounds;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rounds")
	float DefaultIntermissionSeconds;

	UFUNCTION(BlueprintPure, Category = "Rounds")
	bool GetRoundDefinition(int32 RoundIndex, FRoundDefinition& OutRound) const;
};
