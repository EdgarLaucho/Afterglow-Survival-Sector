#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "CompanionCharacter.generated.h"

UCLASS()
class AFTERGLOWSURVIVAL_API ACompanionCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    ACompanionCharacter();

    UFUNCTION(BlueprintPure, Category = "Companion|Animation")
    float GetAnimSpeed() const;

    UFUNCTION(BlueprintPure, Category = "Companion|Animation")
    float GetAnimDirection() const;

    UFUNCTION(BlueprintPure, Category = "Companion|Animation")
    bool GetAnimIsMoving(float MovementThreshold = 5.f) const;

    UFUNCTION(BlueprintPure, Category = "Companion|Animation")
    void GetAnimMoveData(float& OutSpeed, float& OutDirection, bool& bOutIsMoving, float MovementThreshold = 5.f) const;

    UFUNCTION(BlueprintCallable, Category = "Companion|Combat")
    bool FireAt(AActor* Target);

    UFUNCTION(BlueprintNativeEvent, Category = "Companion|Combat|Pool")
    AActor* AcquireProjectileFromPool(FVector SpawnLocation, FRotator SpawnRotation);
    virtual AActor* AcquireProjectileFromPool_Implementation(FVector SpawnLocation, FRotator SpawnRotation);

    UFUNCTION(BlueprintImplementableEvent, Category = "Companion|Combat|Pool")
    void ConfigurePooledProjectile(AActor* Projectile, AActor* Target, FVector ShootDirection);

protected:
    virtual void BeginPlay() override;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Companion|Combat|Pool")
    TSubclassOf<AActor> SpawnPoolActorClass;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Companion|Combat|Pool")
    FName SpawnPoolActorTag;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Companion|Combat|Pool")
    FName SpawnFromPoolFunctionName;

    UPROPERTY(Transient)
    AActor* CachedSpawnPoolActor;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Companion|Combat")
    TSubclassOf<AActor> ProjectileClass;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Companion|Combat")
    float ProjectileSpeed;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Companion|Combat")
    float AimPredictionTime;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Companion|Combat")
    float MaxAimPredictionDistance;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Companion|Combat")
    float ProjectileSpreadDegrees;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Companion|Combat")
    float MuzzleForwardOffset;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Companion|Combat")
    float MuzzleHeightOffset;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Companion|Combat")
    FName MuzzleSocketName;

private:
    AActor* ResolveSpawnPoolActor();
};