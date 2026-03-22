#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameFramework/Actor.h"
#include "AfterglowUtils.generated.h"

UCLASS()
class AFTERGLOWSURVIVAL_API UAfterglowUtils : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:

    UFUNCTION(BlueprintPure, Category = "AI|Visibility")
    static bool IsActorInViewCone(
        AActor* Observer,
        AActor* Target,
        float DotThreshold = 0.3f
    );

    UFUNCTION(BlueprintPure, Category = "Combat|Projectile")
    static FRotator GetSpawnRotationFromLocations(
        FVector SpawnLocation,
        FVector TargetLocation,
        bool bIgnorePitch = false
    );
};