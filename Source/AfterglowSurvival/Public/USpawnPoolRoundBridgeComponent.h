// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "USpawnPoolRoundBridgeComponent.generated.h"

class UURoundConfigDataAsset;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class AFTERGLOWSURVIVAL_API UUSpawnPoolRoundBridgeComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UUSpawnPoolRoundBridgeComponent();

	UFUNCTION(BlueprintCallable, Category = "Rounds|Bridge")
	bool ApplyRoundConfig(UURoundConfigDataAsset* RoundConfig, int32 RoundIndex);

	UFUNCTION(BlueprintCallable, Category = "Rounds|Bridge")
	void StartSpawnTimers();

	UFUNCTION(BlueprintCallable, Category = "Rounds|Bridge")
	void StopSpawnTimers();

	UFUNCTION(BlueprintCallable, Category = "Rounds|Bridge")
	bool AreRoundSpawnsCompleted() const;

	UFUNCTION(BlueprintCallable, Category = "Rounds|Bridge")
	void NotifyRoundChanged(int32 RoundIndex);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, Category = "Rounds|Bridge")
	FName ApplyEnemyRoundSettingFunctionName;

	UPROPERTY(EditAnywhere, Category = "Rounds|Bridge")
	FName StartSpawnTimersFunctionName;

	UPROPERTY(EditAnywhere, Category = "Rounds|Bridge")
	FName StopSpawnTimersFunctionName;

	UPROPERTY(EditAnywhere, Category = "Rounds|Bridge")
	FName IsRoundCompletedFunctionName;

	UPROPERTY(EditAnywhere, Category = "Rounds|Bridge")
	FName OnRoundChangedFunctionName;

	UPROPERTY(EditAnywhere, Category = "Rounds|Debug")
	bool bEnableDebugLogs;

private:
	AActor* GetPoolOwner() const;
	bool CallApplyEnemyRoundSetting(int32 EnemyTypeIndex, int32 MaxTotalSpawns, int32 MaxAlive, float SpawnInterval);
	bool ApplyEnemyRoundSettingByReflection(AActor* OwnerActor, int32 EnemyTypeIndex, int32 MaxTotalSpawns, int32 MaxAlive, float SpawnInterval) const;
	bool IsRoundCompletedByReflection(AActor* OwnerActor) const;
	void CallVoidFunction(FName FunctionName) const;
	bool CallBoolFunction(FName FunctionName) const;
	void CallRoundChanged(int32 RoundIndex) const;

};
