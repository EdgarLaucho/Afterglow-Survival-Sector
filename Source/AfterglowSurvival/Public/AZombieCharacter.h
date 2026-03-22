// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AZombieCharacter.generated.h"

class UUZombieConfigDataAsset;

UCLASS()
class AFTERGLOWSURVIVAL_API AAZombieCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AAZombieCharacter();

	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	UFUNCTION(BlueprintCallable, Category = "Zombie|Pool")
	void ActivateFromPool(FVector NewLocation);

	UFUNCTION(BlueprintCallable, Category = "Zombie|Pool")
	void DeactivateToPool();

	UFUNCTION(BlueprintCallable, Category = "Zombie|Pool")
	void ConfigurePoolContext(AActor* InSpawnPoolRef, int32 InTypeIndex, bool bStartActive = true);

	UFUNCTION(BlueprintCallable, Category = "Zombie|Combat")
	void SetCurrentTarget(AActor* NewTarget);

	UFUNCTION(BlueprintCallable, Category = "Zombie|Combat")
	bool TryAttackTarget();

	UFUNCTION(BlueprintCallable, Category = "Zombie|Combat")
	void PerformAttackHit();

	UFUNCTION(BlueprintCallable, Category = "Zombie|Combat")
	void OnAttackFinished();

	UFUNCTION(BlueprintCallable, Category = "Zombie|Lifecycle")
	void OnDeathFinished();

	UFUNCTION(BlueprintPure, Category = "Zombie|State")
	bool IsZombieAlive() const;

	UFUNCTION(BlueprintPure, Category = "Zombie|State")
	AActor* GetCurrentTarget() const;

	UFUNCTION(BlueprintPure, Category = "Zombie|Config")
	const UUZombieConfigDataAsset* GetZombieConfig() const;

	UFUNCTION(BlueprintPure, Category = "Zombie|Animation")
	void GetAnimMoveData(float& OutSpeed, float& OutDirection, bool& bOutIsAttacking, bool& bOutIsDead) const;

	UFUNCTION(BlueprintPure, Category = "Zombie|Animation")
	float GetZombieSpeed() const;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zombie|Config")
	TObjectPtr<UUZombieConfigDataAsset> ZombieConfig;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zombie|Combat")
	FName AttackTraceSocketName;

	UPROPERTY(BlueprintReadOnly, Category = "Zombie|State")
	bool bIsDead;

	UPROPERTY(BlueprintReadOnly, Category = "Zombie|State")
	bool bIsAttacking;

	UPROPERTY(BlueprintReadOnly, Category = "Zombie|State")
	float CurrentHealth;

	UPROPERTY(BlueprintReadOnly, Category = "Zombie|State")
	bool bPoolActive;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zombie|Pool")
	bool bIsActive;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zombie|Pool")
	TObjectPtr<AActor> SpawnPoolRef;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zombie|Pool")
	int32 TypeIndex;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zombie|Pool")
	FName NotifyEnemyReturnedFunctionName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zombie|Death")
	TSubclassOf<AActor> ZombieCorpseClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zombie|Death")
	FVector ZombieCorpseSpawnOffset;

	UPROPERTY(BlueprintReadOnly, Category = "Zombie|State")
	TObjectPtr<AActor> CurrentTarget;

	UFUNCTION(BlueprintImplementableEvent, Category = "Zombie|Events")
	void BP_OnActivatedFromPool();

	UFUNCTION(BlueprintImplementableEvent, Category = "Zombie|Events")
	void BP_OnDeactivatedToPool();

	UFUNCTION(BlueprintImplementableEvent, Category = "Zombie|Events")
	void BP_OnAttackStarted();

	UFUNCTION(BlueprintImplementableEvent, Category = "Zombie|Events")
	void BP_OnDied();

protected:
	virtual void BeginPlay() override;

private:
	float LastAttackTime;
	FTimerHandle AttackRetryTimer;
	FTimerHandle AttackFinishFailsafeTimer;
	void NotifyEnemyReturnedToPool();
	void TryContinueAttackLoop();

	bool IsValidCombatTarget(AActor* TargetActor) const;
	void Die();

};
