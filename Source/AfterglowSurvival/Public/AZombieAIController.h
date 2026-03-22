// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Navigation/PathFollowingComponent.h"
#include "AZombieAIController.generated.h"

class AAZombieCharacter;
class UUZombieConfigDataAsset;

UCLASS()
class AFTERGLOWSURVIVAL_API AAZombieAIController : public AAIController
{
	GENERATED_BODY()

public:
	AAZombieAIController();

	virtual void BeginPlay() override;
	virtual void OnUnPossess() override;
	virtual void OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result) override;

private:
	void Think();
	void RefreshTarget();
	void HandleChaseOrAttack();
	bool TryMoveNearTargetFallback();
	bool IsTargetActorAlive(AActor* TargetActor) const;

	AAZombieCharacter* GetZombieCharacter() const;
	const UUZombieConfigDataAsset* GetZombieConfig() const;

	UPROPERTY()
	TObjectPtr<AActor> CurrentTarget;

	FTimerHandle ThinkTimer;
	float LastTargetRefreshTime;
	float LastRepathTime;
	int32 ConsecutivePathFailures;
};
