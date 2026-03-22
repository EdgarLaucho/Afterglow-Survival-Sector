// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "UZombieAnimInstance.generated.h"

class AAZombieCharacter;

UCLASS()
class AFTERGLOWSURVIVAL_API UUZombieAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	UFUNCTION(BlueprintCallable, Category = "Zombie|Animation")
	void HandleAttackHitWindowNotify();

	UFUNCTION(BlueprintCallable, Category = "Zombie|Animation")
	void HandleAttackEndNotify();

	UFUNCTION(BlueprintCallable, Category = "Zombie|Animation")
	void HandleDeathFinishedNotify();

	UPROPERTY(BlueprintReadOnly, Category = "Zombie|Animation")
	float Speed;

	UPROPERTY(BlueprintReadOnly, Category = "Zombie|Animation")
	float Direction;

	UPROPERTY(BlueprintReadOnly, Category = "Zombie|Animation")
	bool bIsMoving;

	UPROPERTY(BlueprintReadOnly, Category = "Zombie|Animation")
	bool bIsDead;

	UPROPERTY(BlueprintReadOnly, Category = "Zombie|Animation")
	bool bIsAttacking;

private:
	UPROPERTY(Transient)
	TObjectPtr<AAZombieCharacter> CachedZombie;
};
