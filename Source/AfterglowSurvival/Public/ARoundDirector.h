// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ARoundDirector.generated.h"

class UURoundConfigDataAsset;
class UUSpawnPoolRoundBridgeComponent;

UCLASS()
class AFTERGLOWSURVIVAL_API AARoundDirector : public AActor
{
	GENERATED_BODY()
	
public:	
	AARoundDirector();

	UFUNCTION(BlueprintCallable, Category = "Rounds")
	void StartRounds();

	UFUNCTION(BlueprintCallable, Category = "Rounds")
	void StopRounds();

	UFUNCTION(BlueprintPure, Category = "Rounds")
	int32 GetCurrentRoundIndex() const { return CurrentRoundIndex; }

	UFUNCTION(BlueprintPure, Category = "Rounds")
	bool IsRoundActive() const { return bRoundActive; }

	UFUNCTION(BlueprintImplementableEvent, Category = "Rounds")
	void BP_OnRoundStarted(int32 RoundIndex);

	UFUNCTION(BlueprintImplementableEvent, Category = "Rounds")
	void BP_OnIntermissionStarted(int32 FinishedRoundIndex, float IntermissionSeconds);

	UFUNCTION(BlueprintImplementableEvent, Category = "Rounds")
	void BP_OnRoundsFinished();

protected:
	virtual void BeginPlay() override;
	void ResolveSpawnPoolBridge();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rounds")
	TObjectPtr<UURoundConfigDataAsset> RoundConfig;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rounds")
	TObjectPtr<UUSpawnPoolRoundBridgeComponent> SpawnPoolBridge;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rounds")
	bool bAutoResolveSpawnPoolBridge;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rounds")
	FName SpawnPoolActorTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rounds")
	bool bAutoStartOnBeginPlay;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rounds")
	float RoundCheckInterval;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rounds|Debug")
	bool bEnableDebugLogs;

private:
	void StartRound(int32 RoundIndex);
	void StartIntermission(float IntermissionSeconds);
	void HandleIntermissionFinished();
	void EvaluateRoundCompletion();

	FTimerHandle RoundCheckTimer;
	FTimerHandle IntermissionTimer;
	int32 CurrentRoundIndex;
	bool bRoundActive;
    float PendingIntermissionSeconds;

};
