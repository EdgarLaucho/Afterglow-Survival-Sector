#include "AfterglowUtils.h"

#include "Kismet/GameplayStatics.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"

bool UAfterglowUtils::IsActorInViewCone(AActor* Observer, AActor* Target, float DotThreshold)
{
    if (!Target) return false;

    const auto world = Target->GetWorld();
    if (!world) return false;

    const auto pc = UGameplayStatics::GetPlayerController(world, 0);
    if (!pc) return false;

    const auto cam = pc->PlayerCameraManager;
    if (!cam) return false;

    const auto camLoc = cam->GetCameraLocation();
    const auto camForward = cam->GetCameraRotation().Vector();

    const auto targetLoc = Target->GetActorLocation() + FVector(0, 0, 80);
    const auto dir = (targetLoc - camLoc).GetSafeNormal();

    const auto dot = FVector::DotProduct(camForward, dir);

    const auto halfFOV = FMath::DegreesToRadians(cam->GetFOVAngle() * 0.5f);
    const auto fovDot = FMath::Cos(halfFOV);

    return dot > fovDot;
}

bool UAfterglowUtils::GetPlayerSpawnAnchorLocation(
    const UObject* WorldContextObject,
    FVector FallbackLocation,
    FVector& OutLocation,
    bool bUseAverageWhenMultiple
)
{
    OutLocation = FallbackLocation;

    if (!WorldContextObject)
    {
        return false;
    }

    UWorld* World = WorldContextObject->GetWorld();
    if (!World)
    {
        return false;
    }

    TArray<FVector> ValidPlayerLocations;
    ValidPlayerLocations.Reserve(4);

    for (int32 PlayerIndex = 0; PlayerIndex < 4; ++PlayerIndex)
    {
        APlayerController* PC = UGameplayStatics::GetPlayerController(World, PlayerIndex);
        if (!PC)
        {
            continue;
        }

        APawn* Pawn = PC->GetPawn();
        if (!Pawn || Pawn->IsPendingKillPending() || Pawn->IsActorBeingDestroyed())
        {
            continue;
        }

        ValidPlayerLocations.Add(Pawn->GetActorLocation());
    }

    if (ValidPlayerLocations.Num() == 0)
    {
        return false;
    }

    if (ValidPlayerLocations.Num() == 1)
    {
        OutLocation = ValidPlayerLocations[0];
        return true;
    }

    if (bUseAverageWhenMultiple)
    {
        FVector Sum = FVector::ZeroVector;
        for (const FVector& Loc : ValidPlayerLocations)
        {
            Sum += Loc;
        }

        OutLocation = Sum / static_cast<float>(ValidPlayerLocations.Num());
        return true;
    }

    const int32 RandomIndex = FMath::RandRange(0, ValidPlayerLocations.Num() - 1);
    OutLocation = ValidPlayerLocations[RandomIndex];
    return true;
}

FVector UAfterglowUtils::GetRandomSpawnLocationForPlayers(
    const UObject* WorldContextObject,
    FVector FallbackLocation,
    float MinSpawnDistance,
    float MaxSpawnDistance,
    bool bUseAverageWhenMultiple,
    bool bProjectToGround,
    float TraceHalfHeight,
    TEnumAsByte<ECollisionChannel> TraceChannel,
    bool bTraceComplex
)
{
    FVector AnchorLocation = FallbackLocation;
    GetPlayerSpawnAnchorLocation(WorldContextObject, FallbackLocation, AnchorLocation, bUseAverageWhenMultiple);

    const float MinDist = FMath::Max(0.f, MinSpawnDistance);
    const float MaxDist = FMath::Max(MinDist, MaxSpawnDistance);

    const float RandomDistance = FMath::FRandRange(MinDist, MaxDist);
    const float RandomAngleDeg = FMath::FRandRange(0.f, 360.f);

    const FVector Offset = FVector(
        FMath::Cos(FMath::DegreesToRadians(RandomAngleDeg)),
        FMath::Sin(FMath::DegreesToRadians(RandomAngleDeg)),
        0.f
    ) * RandomDistance;

    const FVector CandidateLocation = AnchorLocation + Offset;

    if (!bProjectToGround || !WorldContextObject)
    {
        return CandidateLocation;
    }

    UWorld* World = WorldContextObject->GetWorld();
    if (!World)
    {
        return CandidateLocation;
    }

    FHitResult Hit;
    const FVector TraceStart = CandidateLocation + FVector(0.f, 0.f, FMath::Abs(TraceHalfHeight));
    const FVector TraceEnd = CandidateLocation - FVector(0.f, 0.f, FMath::Abs(TraceHalfHeight));

    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(GetRandomSpawnLocationForPlayers), bTraceComplex);

    const bool bHit = World->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, TraceChannel, QueryParams);

    if (bHit)
    {
        return Hit.ImpactPoint;
    }

    return CandidateLocation;
}