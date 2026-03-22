#include "AfterglowUtils.h"

#include "Kismet/GameplayStatics.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerController.h"

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

    // 🔥 Convertimos FOV a dot threshold real
    const auto halfFOV = FMath::DegreesToRadians(cam->GetFOVAngle() * 0.5f);
    const auto fovDot = FMath::Cos(halfFOV);

    return dot > fovDot;
}

FRotator UAfterglowUtils::GetSpawnRotationFromLocations(FVector SpawnLocation, FVector TargetLocation, bool bIgnorePitch)
{
    FVector Direction = (TargetLocation - SpawnLocation).GetSafeNormal();
    if (Direction.IsNearlyZero())
    {
        return FRotator::ZeroRotator;
    }

    if (bIgnorePitch)
    {
        Direction.Z = 0.f;
        Direction = Direction.GetSafeNormal();
        if (Direction.IsNearlyZero())
        {
            return FRotator::ZeroRotator;
        }
    }

    return Direction.Rotation();
}
