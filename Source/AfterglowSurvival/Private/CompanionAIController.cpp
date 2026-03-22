#include "CompanionAIController.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "CompanionCharacter.h"
#include "NavigationSystem.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerController.h"

ACompanionAIController::ACompanionAIController()
    : PlayerRef(nullptr)
    , TargetEnemy(nullptr)
    , LastShotTime(-1000.f)
    , ThinkInterval(0.15f)
    , MinFollowDistance(150.f)
    , MaxFollowDistance(450.f)
    , SideOffsetDistance(200.f)
    , FollowAcceptanceRadius(100.f)
    , HardLeashDistance(1600.f)
    , UnreachableTimeout(1.5f)
    , RecoveryCooldown(4.f)
    , OffCameraRecoveryDelay(0.2f)
    , OffCameraScreenMargin(20.f)
    , bRecoverOnlyOffCamera(true)
    , MaxConsecutiveMoveFailures(3)
    , EnemyAvoidanceRadius(500.f)
    , EnemyAvoidanceStrength(250.f)
    , CombatOrbitRadius(250.f)
    , CombatRetreatStrength(280.f)
    , CombatPositionAcceptanceRadius(120.f)
    , EnemySearchRadius(1300.f)
    , AttackRange(700.f)
    , LoseTargetRange(1800.f)
    , FireInterval(0.25f)
    , MinAdaptiveFireInterval(0.12f)
    , MaxAdaptiveFireInterval(0.35f)
    , TargetSwitchScoreDelta(0.2f)
    , CurrentTargetScoreBias(0.15f)
    , PlayerLineSafetyRadius(130.f)
    , CandidateDistanceWeight(0.55f)
    , CandidatePlayerThreatWeight(0.3f)
    , CandidateFacingWeight(0.15f)
    , ConsecutiveMoveFailures(0)
    , UnreachableSinceTime(-1.f)
    , LastRecoveryTime(-1000.f)
{
}

void ACompanionAIController::BeginPlay()
{
    Super::BeginPlay();

    PlayerRef = UGameplayStatics::GetPlayerPawn(this, 0);

    GetWorld()->GetTimerManager().SetTimer(
        ThinkTimer,
        this,
        &ACompanionAIController::Think,
        ThinkInterval,
        true
    );
}

void ACompanionAIController::OnUnPossess()
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(ThinkTimer);
    }

    Super::OnUnPossess();
}

void ACompanionAIController::OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result)
{
    Super::OnMoveCompleted(RequestID, Result);

    if (Result.IsSuccess())
    {
        ConsecutiveMoveFailures = 0;
        UnreachableSinceTime = -1.f;
        return;
    }

    if (Result.Code != EPathFollowingResult::Aborted)
    {
        ++ConsecutiveMoveFailures;
    }
}

void ACompanionAIController::Think()
{
    if (!PlayerRef)
    {
        PlayerRef = UGameplayStatics::GetPlayerPawn(this, 0);
    }

    APawn* MyPawn = GetPawn();
    if (!MyPawn)
    {
        return;
    }

    if (const ACompanionCharacter* Companion = Cast<ACompanionCharacter>(MyPawn))
    {
        if (Companion->IsCompanionDead())
        {
            StopMovement();
            ClearFocus(EAIFocusPriority::Gameplay);
            TargetEnemy = nullptr;
            return;
        }
    }

    if (TargetEnemy)
    {
        const float DistToTarget = FVector::Dist(MyPawn->GetActorLocation(), TargetEnemy->GetActorLocation());
        if (!IsValidEnemy(TargetEnemy) || DistToTarget > LoseTargetRange)
        {
            TargetEnemy = nullptr;
        }
    }

    APawn* BestEnemy = FindClosestEnemy();
    if (!TargetEnemy)
    {
        TargetEnemy = BestEnemy;
    }
    else if (BestEnemy && BestEnemy != TargetEnemy)
    {
        const float CurrentScore = ScoreEnemyCandidate(MyPawn, TargetEnemy) + CurrentTargetScoreBias;
        const float BestScore = ScoreEnemyCandidate(MyPawn, BestEnemy);
        if (BestScore > (CurrentScore + TargetSwitchScoreDelta))
        {
            TargetEnemy = BestEnemy;
        }
    }

    if (TargetEnemy)
    {
        HandleAttack();
    }
    else
    {
        ClearFocus(EAIFocusPriority::Gameplay);
        HandleFollow();
    }
}

void ACompanionAIController::HandleFollow()
{
    if (!PlayerRef) return;

    APawn* myPawn = GetPawn();
    if (!myPawn) return;

    const float distance = FVector::Dist(myPawn->GetActorLocation(), PlayerRef->GetActorLocation());
    const float now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;

    if (distance < MinFollowDistance)
    {
        StopMovement();
        ConsecutiveMoveFailures = 0;
        UnreachableSinceTime = -1.f;
        return;
    }

    if (distance > MaxFollowDistance)
    {
        const FVector playerLocation = PlayerRef->GetActorLocation();
        const float SideSign = ((GetUniqueID() & 1) == 0) ? 1.f : -1.f;
        const FVector offset = PlayerRef->GetActorRightVector() * (SideOffsetDistance * SideSign);
        FVector desiredFollowLocation = playerLocation + offset;

        desiredFollowLocation += ComputeEnemyAvoidanceOffset(myPawn);

        const FVector fromPlayerToDesired = desiredFollowLocation - playerLocation;
        const float maxAllowed = MaxFollowDistance * 0.95f;
        if (fromPlayerToDesired.Size2D() > maxAllowed)
        {
            const FVector clamped2D = fromPlayerToDesired.GetSafeNormal2D() * maxAllowed;
            desiredFollowLocation = playerLocation + FVector(clamped2D.X, clamped2D.Y, fromPlayerToDesired.Z);
        }

        const EPathFollowingRequestResult::Type moveRequest = MoveToLocation(desiredFollowLocation, FollowAcceptanceRadius);
        if (moveRequest == EPathFollowingRequestResult::Failed)
        {
            ++ConsecutiveMoveFailures;
        }
        else if (moveRequest == EPathFollowingRequestResult::AlreadyAtGoal)
        {
            ConsecutiveMoveFailures = 0;
            UnreachableSinceTime = -1.f;
            return;
        }
    }

    const bool bHardLeashed = distance > HardLeashDistance;
    const bool bMoveUnreachable = ConsecutiveMoveFailures >= MaxConsecutiveMoveFailures;
    const bool bOffCamera = IsPawnOffCamera(myPawn);
    const float RecoveryDelay = bOffCamera ? OffCameraRecoveryDelay : UnreachableTimeout;

    if (bHardLeashed || bMoveUnreachable)
    {
        if (UnreachableSinceTime < 0.f)
        {
            UnreachableSinceTime = now;
        }

        if ((now - UnreachableSinceTime) >= RecoveryDelay)
        {
            TryRecoverNearPlayer(myPawn, PlayerRef);
        }
    }
    else
    {
        UnreachableSinceTime = -1.f;
    }
}

bool ACompanionAIController::TryRecoverNearPlayer(APawn* MyPawn, APawn* PlayerPawn)
{
    if (!MyPawn || !PlayerPawn)
    {
        return false;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return false;
    }

    const float now = World->GetTimeSeconds();
    if ((now - LastRecoveryTime) < RecoveryCooldown)
    {
        return false;
    }

    if (bRecoverOnlyOffCamera && !IsPawnOffCamera(MyPawn))
    {
        return false;
    }

    const float SideSign = ((GetUniqueID() & 1) == 0) ? 1.f : -1.f;
    const FVector offset = PlayerPawn->GetActorRightVector() * (SideOffsetDistance * SideSign);
    const FVector preferred = PlayerPawn->GetActorLocation() + offset;

    FVector recoverLocation = preferred;
    if (UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World))
    {
        FNavLocation NavLocation;
        const bool bProjected = NavSys->ProjectPointToNavigation(
            preferred,
            NavLocation,
            FVector(300.f, 300.f, 700.f)
        );

        if (bProjected)
        {
            recoverLocation = NavLocation.Location;
        }
    }

    recoverLocation.Z += 20.f;

    const bool bTeleported = MyPawn->SetActorLocation(recoverLocation, false, nullptr, ETeleportType::TeleportPhysics);
    if (!bTeleported)
    {
        return false;
    }

    StopMovement();
    LastRecoveryTime = now;
    ConsecutiveMoveFailures = 0;
    UnreachableSinceTime = -1.f;
    return true;
}

bool ACompanionAIController::IsPawnOffCamera(APawn* InPawn) const
{
    if (!InPawn)
    {
        return true;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return true;
    }

    APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);
    if (!PC || !PC->PlayerCameraManager)
    {
        return true;
    }

    const FVector PawnLocation = InPawn->GetActorLocation();
    const FVector CamLocation = PC->PlayerCameraManager->GetCameraLocation();
    const FVector CamForward = PC->PlayerCameraManager->GetCameraRotation().Vector();
    const FVector ToPawnDir = (PawnLocation - CamLocation).GetSafeNormal();
    if (ToPawnDir.IsNearlyZero())
    {
        return false;
    }

    // Si está detrás de la cámara, está fuera de pantalla.
    if (FVector::DotProduct(CamForward, ToPawnDir) <= 0.f)
    {
        return true;
    }

    FVector2D ScreenPos;
    if (!PC->ProjectWorldLocationToScreen(PawnLocation, ScreenPos, true))
    {
        return true;
    }

    int32 ViewX = 0;
    int32 ViewY = 0;
    PC->GetViewportSize(ViewX, ViewY);
    if (ViewX <= 0 || ViewY <= 0)
    {
        return true;
    }

    const float MinX = -OffCameraScreenMargin;
    const float MinY = -OffCameraScreenMargin;
    const float MaxX = static_cast<float>(ViewX) + OffCameraScreenMargin;
    const float MaxY = static_cast<float>(ViewY) + OffCameraScreenMargin;

    if (ScreenPos.X < MinX || ScreenPos.X > MaxX || ScreenPos.Y < MinY || ScreenPos.Y > MaxY)
    {
        return true;
    }

    const float Dot = FVector::DotProduct(CamForward, ToPawnDir);
    const float HalfFovRadians = FMath::DegreesToRadians(PC->PlayerCameraManager->GetFOVAngle() * 0.5f);
    const float FovDot = FMath::Cos(HalfFovRadians);

    // Fuera de cámara si queda fuera del cono frontal.
    return Dot < FovDot;
}

FVector ACompanionAIController::ComputeEnemyAvoidanceOffset(APawn* MyPawn) const
{
    if (!MyPawn || EnemyAvoidanceRadius <= 0.f || EnemyAvoidanceStrength <= 0.f)
    {
        return FVector::ZeroVector;
    }

    TArray<AActor*> Actors;
    UGameplayStatics::GetAllActorsOfClass(this, ACharacter::StaticClass(), Actors);

    FVector2D Avoidance2D = FVector2D::ZeroVector;
    int32 InfluenceCount = 0;

    for (AActor* Actor : Actors)
    {
        APawn* EnemyPawn = Cast<APawn>(Actor);
        if (!EnemyPawn || !IsValidEnemy(EnemyPawn))
        {
            continue;
        }

        const FVector Delta = MyPawn->GetActorLocation() - EnemyPawn->GetActorLocation();
        const FVector2D Delta2D(Delta.X, Delta.Y);
        const float Distance = Delta2D.Size();
        if (Distance <= KINDA_SMALL_NUMBER || Distance > EnemyAvoidanceRadius)
        {
            continue;
        }

        const float Alpha = 1.f - (Distance / EnemyAvoidanceRadius);
        const float Weight = Alpha * Alpha;
        Avoidance2D += Delta2D.GetSafeNormal() * Weight;
        ++InfluenceCount;
    }

    if (InfluenceCount == 0 || Avoidance2D.IsNearlyZero())
    {
        return FVector::ZeroVector;
    }

    const FVector2D Offset2D = Avoidance2D.GetSafeNormal() * EnemyAvoidanceStrength;
    return FVector(Offset2D.X, Offset2D.Y, 0.f);
}

FVector ACompanionAIController::ComputeCombatPosition(APawn* MyPawn, APawn* Candidate) const
{
    if (!MyPawn || !PlayerRef)
    {
        return MyPawn ? MyPawn->GetActorLocation() : FVector::ZeroVector;
    }

    const FVector playerLocation = PlayerRef->GetActorLocation();
    const float SideSign = ((GetUniqueID() & 1) == 0) ? 1.f : -1.f;
    FVector desired = playerLocation + (PlayerRef->GetActorRightVector() * (CombatOrbitRadius * SideSign));

    if (Candidate)
    {
        const FVector awayFromEnemy = (MyPawn->GetActorLocation() - Candidate->GetActorLocation()).GetSafeNormal2D();
        desired += awayFromEnemy * CombatRetreatStrength;
    }

    desired += ComputeEnemyAvoidanceOffset(MyPawn);

    const FVector fromPlayer = desired - playerLocation;
    const float maxAllowed = MaxFollowDistance * 0.95f;
    if (fromPlayer.Size2D() > maxAllowed)
    {
        const FVector clamped = fromPlayer.GetSafeNormal2D() * maxAllowed;
        desired = playerLocation + FVector(clamped.X, clamped.Y, fromPlayer.Z);
    }

    return desired;
}

bool ACompanionAIController::HasSafeShotLane(APawn* MyPawn, APawn* Candidate) const
{
    if (!MyPawn || !Candidate || !PlayerRef)
    {
        return true;
    }

    const FVector shotStart = MyPawn->GetPawnViewLocation();
    const FVector shotEnd = Candidate->GetActorLocation() + FVector(0.f, 0.f, 70.f);

    const float distToShotSegment = FMath::PointDistToSegment(PlayerRef->GetActorLocation(), shotStart, shotEnd);
    if (distToShotSegment <= PlayerLineSafetyRadius)
    {
        return false;
    }

    return true;
}

float ACompanionAIController::GetAdaptiveFireInterval(APawn* Candidate) const
{
    if (!Candidate)
    {
        return FireInterval;
    }

    APawn* MyPawn = GetPawn();
    if (!MyPawn)
    {
        return FireInterval;
    }

    const float distance = FVector::Dist(MyPawn->GetActorLocation(), Candidate->GetActorLocation());
    const float alpha = FMath::Clamp(distance / FMath::Max(AttackRange, 1.f), 0.f, 1.f);
    return FMath::Lerp(MinAdaptiveFireInterval, MaxAdaptiveFireInterval, alpha);
}

void ACompanionAIController::HandleAttack()
{
    if (!TargetEnemy) return;

    APawn* myPawn = GetPawn();
    if (!myPawn) return;

    // Mantener lógica de recovery también durante combate.
    if (PlayerRef)
    {
        const float distanceToPlayer = FVector::Dist(myPawn->GetActorLocation(), PlayerRef->GetActorLocation());
        const float now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
        const bool bHardLeashed = distanceToPlayer > HardLeashDistance;
        const bool bMoveUnreachable = ConsecutiveMoveFailures >= MaxConsecutiveMoveFailures;
        const bool bOffCamera = IsPawnOffCamera(myPawn);
        const float recoveryDelay = bOffCamera ? OffCameraRecoveryDelay : UnreachableTimeout;

        if (bHardLeashed || bMoveUnreachable)
        {
            if (UnreachableSinceTime < 0.f)
            {
                UnreachableSinceTime = now;
            }

            if ((now - UnreachableSinceTime) >= recoveryDelay)
            {
                if (TryRecoverNearPlayer(myPawn, PlayerRef))
                {
                    // Tras recuperar posición, esperar al siguiente think para reacoplar combate.
                    return;
                }
            }
        }
        else
        {
            UnreachableSinceTime = -1.f;
        }
    }

    // Mantener proximidad al player con posicionamiento táctico alrededor de él.
    const FVector tacticalPos = ComputeCombatPosition(myPawn, TargetEnemy);
    MoveToLocation(tacticalPos, CombatPositionAcceptanceRadius);

    const float distance = FVector::Dist(myPawn->GetActorLocation(), TargetEnemy->GetActorLocation());
    if (distance > AttackRange)
    {
        ClearFocus(EAIFocusPriority::Gameplay);
        return;
    }

    // Mirada más precisa: focal point en torso + pequeño lead por velocidad.
    FVector FocalPoint = TargetEnemy->GetActorLocation() + FVector(0.f, 0.f, 70.f);
    const FVector TargetVelocity = TargetEnemy->GetVelocity();
    FocalPoint += TargetVelocity * 0.08f;
    SetFocalPoint(FocalPoint, EAIFocusPriority::Gameplay);

    if (CanShootTarget(TargetEnemy))
    {
        Shoot(TargetEnemy);
    }
}

APawn* ACompanionAIController::FindClosestEnemy()
{
    TArray<AActor*> actors;
    UGameplayStatics::GetAllActorsOfClass(this, ACharacter::StaticClass(), actors);

    APawn* myPawn = GetPawn();
    if (!myPawn) return nullptr;

    APawn* best = nullptr;
    float bestScore = -FLT_MAX;

    for (AActor* actor : actors)
    {
        APawn* candidate = Cast<APawn>(actor);
        if (!candidate) continue;

        if (!IsValidEnemy(candidate)) continue;

        const float dist = FVector::Dist(candidate->GetActorLocation(), myPawn->GetActorLocation());
        if (dist > EnemySearchRadius)
        {
            continue;
        }

        const float score = ScoreEnemyCandidate(myPawn, candidate);
        if (score > bestScore)
        {
            best = candidate;
            bestScore = score;
        }
    }

    return best;
}

float ACompanionAIController::ScoreEnemyCandidate(APawn* MyPawn, APawn* Candidate) const
{
    if (!MyPawn || !Candidate)
    {
        return -FLT_MAX;
    }

    const float distToCompanion = FVector::Dist(MyPawn->GetActorLocation(), Candidate->GetActorLocation());
    const float distNorm = 1.f - FMath::Clamp(distToCompanion / FMath::Max(EnemySearchRadius, 1.f), 0.f, 1.f);

    float playerThreatNorm = 0.f;
    if (PlayerRef)
    {
        const float distToPlayer = FVector::Dist(PlayerRef->GetActorLocation(), Candidate->GetActorLocation());
        playerThreatNorm = 1.f - FMath::Clamp(distToPlayer / FMath::Max(EnemySearchRadius, 1.f), 0.f, 1.f);
    }

    float facingNorm = 0.f;
    if (PlayerRef)
    {
        const FVector toCandidate = (Candidate->GetActorLocation() - PlayerRef->GetActorLocation()).GetSafeNormal2D();
        const FVector playerForward = PlayerRef->GetActorForwardVector().GetSafeNormal2D();
        const float dot = FVector::DotProduct(playerForward, toCandidate);
        facingNorm = (dot + 1.f) * 0.5f;
    }

    const float score =
        (distNorm * CandidateDistanceWeight) +
        (playerThreatNorm * CandidatePlayerThreatWeight) +
        (facingNorm * CandidateFacingWeight);

    return score;
}

bool ACompanionAIController::IsValidEnemy(APawn* Candidate) const
{
    if (!Candidate) return false;
    if (Candidate == GetPawn()) return false;
    if (Candidate == PlayerRef) return false;
    if (Candidate->IsPendingKillPending()) return false;

    // Descartar enemigos marcados como muertos (aunque sigan en animación de death).
    static const FName IsDeadName(TEXT("IsDead"));
    static const FName bIsDeadName(TEXT("bIsDead"));

    if (const FBoolProperty* IsDeadProp = FindFProperty<FBoolProperty>(Candidate->GetClass(), IsDeadName))
    {
        if (IsDeadProp->GetPropertyValue_InContainer(Candidate))
        {
            return false;
        }
    }

    if (const FBoolProperty* bIsDeadProp = FindFProperty<FBoolProperty>(Candidate->GetClass(), bIsDeadName))
    {
        if (bIsDeadProp->GetPropertyValue_InContainer(Candidate))
        {
            return false;
        }
    }

    return true;
}

bool ACompanionAIController::CanShootTarget(APawn* Candidate) const
{
    if (!Candidate) return false;

    APawn* myPawn = GetPawn();
    if (!myPawn) return false;

    if (!HasSafeShotLane(myPawn, Candidate))
    {
        return false;
    }

    FHitResult hit;
    FCollisionQueryParams queryParams(SCENE_QUERY_STAT(CompanionShootLOS), true, myPawn);
    queryParams.AddIgnoredActor(myPawn);
    if (PlayerRef)
    {
        queryParams.AddIgnoredActor(PlayerRef);
    }

    const FVector shootFrom = myPawn->GetPawnViewLocation();
    const FVector shootTo = Candidate->GetActorLocation() + FVector(0.f, 0.f, 70.f);

    const bool bHit = GetWorld()->LineTraceSingleByChannel(
        hit,
        shootFrom,
        shootTo,
        ECC_Visibility,
        queryParams
    );

    if (!bHit)
    {
        // Fallback cuando el zombie no bloquea Visibility:
        // solo permitimos disparo si el companion está mirando al target.
        const FVector ToTarget = (shootTo - shootFrom).GetSafeNormal();
        const FVector Forward = myPawn->GetActorForwardVector().GetSafeNormal();
        const float Dot = FVector::DotProduct(Forward, ToTarget);
        return Dot >= 0.8f;
    }

    // Si impacta al propio target o a algo adjunto al target, permitimos disparo.
    const AActor* HitActor = hit.GetActor();
    if (!HitActor)
    {
        return false;
    }

    if (HitActor == Candidate || HitActor->IsOwnedBy(Candidate))
    {
        return true;
    }

    // Si hay cualquier otro actor en medio, bloqueamos.
    return false;
}

void ACompanionAIController::Shoot(APawn* Target)
{
    if (!Target) return;

    APawn* myPawn = GetPawn();
    if (!myPawn) return;

    ACompanionCharacter* Companion = Cast<ACompanionCharacter>(myPawn);
    if (!Companion) return;

    UWorld* world = GetWorld();
    if (!world) return;

    const float now = world->GetTimeSeconds();
    const float adaptiveInterval = FMath::Max(GetAdaptiveFireInterval(Target), 0.01f);
    if ((now - LastShotTime) < adaptiveInterval) return;

    if (Companion->FireAt(Target))
    {
        LastShotTime = now;
    }
}