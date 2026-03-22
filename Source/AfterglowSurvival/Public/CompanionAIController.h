#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Navigation/PathFollowingComponent.h"
#include "CompanionAIController.generated.h"

UCLASS()
class AFTERGLOWSURVIVAL_API ACompanionAIController : public AAIController
{
    GENERATED_BODY()

public:
    ACompanionAIController();

    virtual void BeginPlay() override;
    virtual void OnUnPossess() override;
    virtual void OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result) override;

private:
    void Think();

    UPROPERTY()
    APawn* PlayerRef;

    UPROPERTY()
    APawn* TargetEnemy;

    FTimerHandle ThinkTimer;
    float LastShotTime;

    UPROPERTY(EditAnywhere, Category = "Companion|AI")
    float ThinkInterval;

    UPROPERTY(EditAnywhere, Category = "Companion|Follow")
    float MinFollowDistance;

    UPROPERTY(EditAnywhere, Category = "Companion|Follow")
    float MaxFollowDistance;

    UPROPERTY(EditAnywhere, Category = "Companion|Follow")
    float SideOffsetDistance;

    UPROPERTY(EditAnywhere, Category = "Companion|Follow")
    float FollowAcceptanceRadius;

    UPROPERTY(EditAnywhere, Category = "Companion|Follow")
    float HardLeashDistance;

    UPROPERTY(EditAnywhere, Category = "Companion|Follow")
    float UnreachableTimeout;

    UPROPERTY(EditAnywhere, Category = "Companion|Follow")
    float RecoveryCooldown;

    UPROPERTY(EditAnywhere, Category = "Companion|Follow")
    float OffCameraRecoveryDelay;

    UPROPERTY(EditAnywhere, Category = "Companion|Follow")
    float OffCameraScreenMargin;

    UPROPERTY(EditAnywhere, Category = "Companion|Follow")
    bool bRecoverOnlyOffCamera;

    UPROPERTY(EditAnywhere, Category = "Companion|Follow")
    int32 MaxConsecutiveMoveFailures;

    UPROPERTY(EditAnywhere, Category = "Companion|Follow")
    float EnemyAvoidanceRadius;

    UPROPERTY(EditAnywhere, Category = "Companion|Follow")
    float EnemyAvoidanceStrength;

    UPROPERTY(EditAnywhere, Category = "Companion|Follow")
    float CombatOrbitRadius;

    UPROPERTY(EditAnywhere, Category = "Companion|Follow")
    float CombatRetreatStrength;

    UPROPERTY(EditAnywhere, Category = "Companion|Follow")
    float CombatPositionAcceptanceRadius;

    UPROPERTY(EditAnywhere, Category = "Companion|Combat")
    float EnemySearchRadius;

    UPROPERTY(EditAnywhere, Category = "Companion|Combat")
    float AttackRange;

    UPROPERTY(EditAnywhere, Category = "Companion|Combat")
    float LoseTargetRange;

    UPROPERTY(EditAnywhere, Category = "Companion|Combat")
    float FireInterval;

    UPROPERTY(EditAnywhere, Category = "Companion|Combat")
    float MinAdaptiveFireInterval;

    UPROPERTY(EditAnywhere, Category = "Companion|Combat")
    float MaxAdaptiveFireInterval;

    UPROPERTY(EditAnywhere, Category = "Companion|Combat")
    float TargetSwitchScoreDelta;

    UPROPERTY(EditAnywhere, Category = "Companion|Combat")
    float CurrentTargetScoreBias;

    UPROPERTY(EditAnywhere, Category = "Companion|Combat")
    float PlayerLineSafetyRadius;

    UPROPERTY(EditAnywhere, Category = "Companion|Combat")
    float CandidateDistanceWeight;

    UPROPERTY(EditAnywhere, Category = "Companion|Combat")
    float CandidatePlayerThreatWeight;

    UPROPERTY(EditAnywhere, Category = "Companion|Combat")
    float CandidateFacingWeight;

    int32 ConsecutiveMoveFailures;
    float UnreachableSinceTime;
    float LastRecoveryTime;

    APawn* FindClosestEnemy();
    float ScoreEnemyCandidate(APawn* MyPawn, APawn* Candidate) const;
    bool IsValidEnemy(APawn* Candidate) const;
    bool CanShootTarget(APawn* Candidate) const;
    bool HasSafeShotLane(APawn* MyPawn, APawn* Candidate) const;
    bool TryRecoverNearPlayer(APawn* MyPawn, APawn* PlayerPawn);
    bool IsPawnOffCamera(APawn* InPawn) const;
    FVector ComputeEnemyAvoidanceOffset(APawn* MyPawn) const;
    FVector ComputeCombatPosition(APawn* MyPawn, APawn* Candidate) const;
    float GetAdaptiveFireInterval(APawn* Candidate) const;

    void HandleFollow();
    void HandleAttack();

    void Shoot(APawn* Target);
};