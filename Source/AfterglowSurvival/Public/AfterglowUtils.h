#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameFramework/Actor.h"
#include "Engine/EngineTypes.h"
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

    UFUNCTION(BlueprintPure, Category = "Spawn|Utils", meta = (WorldContext = "WorldContextObject"))
    static bool GetPlayerSpawnAnchorLocation(
        const UObject* WorldContextObject,
        FVector FallbackLocation,
        FVector& OutLocation,
        bool bUseAverageWhenMultiple = false
    );

    UFUNCTION(BlueprintPure, Category = "Spawn|Utils", meta = (WorldContext = "WorldContextObject"))
    static FVector GetRandomSpawnLocationForPlayers(
        const UObject* WorldContextObject,
        FVector FallbackLocation,
        float MinSpawnDistance,
        float MaxSpawnDistance,
        bool bUseAverageWhenMultiple = false,
        bool bProjectToGround = true,
        float TraceHalfHeight = 5000.f,
        TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility,
        bool bTraceComplex = false
    );
};