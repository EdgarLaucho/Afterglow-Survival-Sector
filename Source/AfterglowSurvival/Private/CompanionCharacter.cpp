#include "CompanionCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "CompanionAIController.h"
#include "Engine/World.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SkeletalMeshComponent.h"

ACompanionCharacter::ACompanionCharacter()
{
    // No necesitamos Tick para la IA (optimización)
    PrimaryActorTick.bCanEverTick = false;

    // Conectar AI Controller
    AIControllerClass = ACompanionAIController::StaticClass();

    // Auto posesión al spawnear
    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

    // Rotación suave: usar rotación deseada del controller con límite de velocidad.
    bUseControllerRotationYaw = false;
    if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
    {
        MoveComp->bOrientRotationToMovement = false;
        MoveComp->bUseControllerDesiredRotation = true;
        MoveComp->RotationRate = FRotator(0.f, 320.f, 0.f);
    }

    ProjectileSpeed = 2200.f;
    AimPredictionTime = 0.18f;
    MaxAimPredictionDistance = 450.f;
    ProjectileSpreadDegrees = 2.f;
    MuzzleForwardOffset = 70.f;
    MuzzleHeightOffset = 60.f;
    MuzzleSocketName = TEXT("ProjectileSocket");

    SpawnPoolActorTag = TEXT("ProjectilePool");
    SpawnFromPoolFunctionName = TEXT("SpawnFromPool");
    CachedSpawnPoolActor = nullptr;
}

float ACompanionCharacter::GetAnimSpeed() const
{
    const FVector Velocity2D = FVector(GetVelocity().X, GetVelocity().Y, 0.f);
    return Velocity2D.Size();
}

float ACompanionCharacter::GetAnimDirection() const
{
    const FVector Velocity2D = FVector(GetVelocity().X, GetVelocity().Y, 0.f);
    if (Velocity2D.IsNearlyZero())
    {
        return 0.f;
    }

    const FVector MoveDir = Velocity2D.GetSafeNormal();
    const FVector Forward = GetActorForwardVector().GetSafeNormal2D();
    const FVector Right = GetActorRightVector().GetSafeNormal2D();

    const float ForwardDot = FVector::DotProduct(Forward, MoveDir);
    const float RightDot = FVector::DotProduct(Right, MoveDir);

    return FMath::RadiansToDegrees(FMath::Atan2(RightDot, ForwardDot));
}

bool ACompanionCharacter::GetAnimIsMoving(float MovementThreshold) const
{
    return GetAnimSpeed() > FMath::Max(0.f, MovementThreshold);
}

void ACompanionCharacter::GetAnimMoveData(float& OutSpeed, float& OutDirection, bool& bOutIsMoving, float MovementThreshold) const
{
    OutSpeed = GetAnimSpeed();
    OutDirection = GetAnimDirection();
    bOutIsMoving = OutSpeed > FMath::Max(0.f, MovementThreshold);
}

void ACompanionCharacter::BeginPlay()
{
    Super::BeginPlay();

    ResolveSpawnPoolActor();
}

bool ACompanionCharacter::FireAt(AActor* Target)
{
    if (!Target) return false;

    UWorld* World = GetWorld();
    if (!World) return false;

    FVector MuzzleLocation =
        GetActorLocation() +
        (GetActorForwardVector() * MuzzleForwardOffset) +
        FVector(0.f, 0.f, MuzzleHeightOffset);

    if (USkeletalMeshComponent* MeshComp = GetMesh())
    {
        if (!MuzzleSocketName.IsNone() && MeshComp->DoesSocketExist(MuzzleSocketName))
        {
            MuzzleLocation = MeshComp->GetSocketLocation(MuzzleSocketName);
        }
    }

    FVector AimPoint = Target->GetActorLocation() + FVector(0.f, 0.f, 70.f);

    const FVector TargetVelocity = Target->GetVelocity();
    const float DistanceToTarget = FVector::Dist(MuzzleLocation, AimPoint);
    const float TravelTime = DistanceToTarget / FMath::Max(ProjectileSpeed, 1.f);
    const float PredictionTime = FMath::Min(TravelTime, FMath::Max(AimPredictionTime, 0.f));
    const FVector PredictedOffset = (TargetVelocity * PredictionTime).GetClampedToMaxSize(MaxAimPredictionDistance);
    AimPoint += PredictedOffset;

    FVector ShootDirection = (AimPoint - MuzzleLocation).GetSafeNormal();
    ShootDirection = FMath::VRandCone(ShootDirection, FMath::DegreesToRadians(ProjectileSpreadDegrees));
    const FRotator ProjectileRotation = ShootDirection.Rotation();

    AActor* Projectile = AcquireProjectileFromPool(MuzzleLocation, ProjectileRotation);

    if (!Projectile && ProjectileClass)
    {
        FActorSpawnParameters SpawnParams;
        SpawnParams.Owner = this;
        SpawnParams.Instigator = this;
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

        Projectile = World->SpawnActor<AActor>(ProjectileClass, MuzzleLocation, ProjectileRotation, SpawnParams);
    }

    if (!Projectile)
    {
        UE_LOG(LogTemp, Warning, TEXT("Companion FireAt failed: no pooled projectile and ProjectileClass is not set."));
        return false;
    }

    Projectile->SetOwner(this);
    Projectile->SetInstigator(this);
    Projectile->SetActorLocationAndRotation(MuzzleLocation, ProjectileRotation, false, nullptr, ETeleportType::TeleportPhysics);
    Projectile->SetActorHiddenInGame(false);
    Projectile->SetActorEnableCollision(true);

    TArray<UPrimitiveComponent*> PrimitiveComponents;
    Projectile->GetComponents<UPrimitiveComponent>(PrimitiveComponents);
    for (UPrimitiveComponent* PrimitiveComp : PrimitiveComponents)
    {
        if (!PrimitiveComp) continue;
        PrimitiveComp->IgnoreActorWhenMoving(this, true);
        if (AActor* OwnerActor = GetOwner())
        {
            PrimitiveComp->IgnoreActorWhenMoving(OwnerActor, true);
        }
    }

    if (UProjectileMovementComponent* ProjectileMovement = Projectile->FindComponentByClass<UProjectileMovementComponent>())
    {
        ProjectileMovement->StopMovementImmediately();
        ProjectileMovement->Velocity = ShootDirection * ProjectileSpeed;
        ProjectileMovement->Activate(true);
    }

    ConfigurePooledProjectile(Projectile, Target, ShootDirection);

    UE_LOG(LogTemp, Verbose, TEXT("Companion fired projectile at %s"), *Target->GetName());

    return true;
}

AActor* ACompanionCharacter::ResolveSpawnPoolActor()
{
    if (CachedSpawnPoolActor && !CachedSpawnPoolActor->IsPendingKillPending())
    {
        return CachedSpawnPoolActor;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return nullptr;
    }

    if (SpawnPoolActorClass)
    {
        TArray<AActor*> FoundActors;
        UGameplayStatics::GetAllActorsOfClass(World, SpawnPoolActorClass, FoundActors);
        if (FoundActors.Num() > 0)
        {
            CachedSpawnPoolActor = FoundActors[0];
            return CachedSpawnPoolActor;
        }
    }

    if (!SpawnPoolActorTag.IsNone())
    {
        TArray<AActor*> TaggedActors;
        UGameplayStatics::GetAllActorsWithTag(World, SpawnPoolActorTag, TaggedActors);
        if (TaggedActors.Num() > 0)
        {
            CachedSpawnPoolActor = TaggedActors[0];
            return CachedSpawnPoolActor;
        }
    }

    return nullptr;
}

AActor* ACompanionCharacter::AcquireProjectileFromPool_Implementation(FVector SpawnLocation, FRotator SpawnRotation)
{
    AActor* PoolActor = ResolveSpawnPoolActor();
    if (!PoolActor)
    {
        UE_LOG(LogTemp, Warning, TEXT("Companion: pool actor not found. Set SpawnPoolActorClass or SpawnPoolActorTag."));
        return nullptr;
    }

    if (SpawnFromPoolFunctionName.IsNone())
    {
        return nullptr;
    }

    UFunction* SpawnFn = PoolActor->FindFunction(SpawnFromPoolFunctionName);
    if (!SpawnFn)
    {
        UE_LOG(LogTemp, Warning, TEXT("Companion: function '%s' not found on pool actor '%s'."), *SpawnFromPoolFunctionName.ToString(), *PoolActor->GetName());
        return nullptr;
    }

    struct FSpawnFromPoolParams
    {
        FVector SpawnLocation;
        FRotator SpawnRotation;
        AActor* SpawnedActor;
    };

    FSpawnFromPoolParams Params;
    Params.SpawnLocation = SpawnLocation;
    Params.SpawnRotation = SpawnRotation;
    Params.SpawnedActor = nullptr;

    PoolActor->ProcessEvent(SpawnFn, &Params);
    return Params.SpawnedActor;
}