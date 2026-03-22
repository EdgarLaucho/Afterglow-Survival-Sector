#include "AZombieAIController.h"

#include "AZombieCharacter.h"
#include "UZombieConfigDataAsset.h"
#include "UZombieTargetRegistrySubsystem.h"

#include "Engine/World.h"
#include "TimerManager.h"
#include "NavigationSystem.h"

AAZombieAIController::AAZombieAIController()
	: CurrentTarget(nullptr)
	, LastTargetRefreshTime(-1000.f)
	, LastRepathTime(-1000.f)
	, ConsecutivePathFailures(0)
{
}

void AAZombieAIController::BeginPlay()
{
	Super::BeginPlay();

	const UUZombieConfigDataAsset* Config = GetZombieConfig();
	const float ThinkInterval = Config ? Config->ThinkInterval : 0.2f;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(ThinkTimer, this, &AAZombieAIController::Think, ThinkInterval, true);
	}
}

void AAZombieAIController::OnUnPossess()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ThinkTimer);
	}

	CurrentTarget = nullptr;
	Super::OnUnPossess();
}

void AAZombieAIController::OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result)
{
	Super::OnMoveCompleted(RequestID, Result);

	if (Result.IsSuccess())
	{
		ConsecutivePathFailures = 0;
		return;
	}

	if (Result.Code != EPathFollowingResult::Aborted)
	{
		++ConsecutivePathFailures;
	}
}

void AAZombieAIController::Think()
{
	AAZombieCharacter* Zombie = GetZombieCharacter();
	if (!Zombie || !Zombie->IsZombieAlive())
	{
		StopMovement();
		return;
	}

	RefreshTarget();

	Zombie->SetCurrentTarget(CurrentTarget);
	HandleChaseOrAttack();
}

void AAZombieAIController::RefreshTarget()
{
	AAZombieCharacter* Zombie = GetZombieCharacter();
	if (!Zombie)
	{
		CurrentTarget = nullptr;
		return;
	}

	const UUZombieConfigDataAsset* Config = GetZombieConfig();
	const float RefreshInterval = Config ? Config->TargetRefreshInterval : 0.35f;
	const float SearchRadius = Config ? Config->TargetSearchRadius : 3000.f;

	UWorld* World = GetWorld();
	if (!World)
	{
		CurrentTarget = nullptr;
		return;
	}

	const float now = World->GetTimeSeconds();
	if (CurrentTarget && !IsTargetActorAlive(CurrentTarget))
	{
		CurrentTarget = nullptr;
	}

	if (CurrentTarget && (now - LastTargetRefreshTime) < RefreshInterval)
	{
		return;
	}

	if (UUZombieTargetRegistrySubsystem* Registry = World->GetSubsystem<UUZombieTargetRegistrySubsystem>())
	{
		CurrentTarget = Registry->GetClosestTarget(Zombie->GetActorLocation(), Zombie, SearchRadius);
	}

	LastTargetRefreshTime = now;
}

void AAZombieAIController::HandleChaseOrAttack()
{
	AAZombieCharacter* Zombie = GetZombieCharacter();
	if (!Zombie)
	{
		return;
	}

	AActor* TargetActor = CurrentTarget.Get();
	if (!IsTargetActorAlive(TargetActor))
	{
		CurrentTarget = nullptr;
		StopMovement();
		return;
	}

	const UUZombieConfigDataAsset* Config = GetZombieConfig();
	const float AttackRange = Config ? Config->AttackRange : 170.f;
	const float MoveAcceptance = Config ? Config->MoveAcceptanceRadius : 90.f;

	const float Distance = FVector::Dist(Zombie->GetActorLocation(), TargetActor->GetActorLocation());
	if (Distance <= AttackRange)
	{
		StopMovement();
		SetFocalPoint(TargetActor->GetActorLocation());
		Zombie->TryAttackTarget();
		return;
	}

	UWorld* World = GetWorld();
	const float now = World ? World->GetTimeSeconds() : 0.f;
	const float RepathCooldown = Config ? Config->RepathCooldown : 0.2f;
	if ((now - LastRepathTime) < RepathCooldown)
	{
		return;
	}

	LastRepathTime = now;

	const EPathFollowingRequestResult::Type MoveResult = MoveToActor(TargetActor, MoveAcceptance, true, true, true);
	if (MoveResult == EPathFollowingRequestResult::Failed)
	{
		++ConsecutivePathFailures;
		TryMoveNearTargetFallback();
	}
}

bool AAZombieAIController::TryMoveNearTargetFallback()
{
	AAZombieCharacter* Zombie = GetZombieCharacter();
	if (!Zombie)
	{
		return false;
	}

	AActor* TargetActor = CurrentTarget.Get();
	if (!TargetActor)
	{
		return false;
	}

	const UUZombieConfigDataAsset* Config = GetZombieConfig();
	const int32 MaxFailures = Config ? Config->MaxConsecutivePathFailures : 3;
	if (ConsecutivePathFailures < MaxFailures)
	{
		return false;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	const float FallbackRadius = Config ? Config->NoPathFallbackRadius : 220.f;
	const float Acceptance = Config ? Config->MoveAcceptanceRadius : 90.f;

	const FVector ToZombie = (Zombie->GetActorLocation() - TargetActor->GetActorLocation()).GetSafeNormal2D();
	const FVector Preferred = TargetActor->GetActorLocation() + (ToZombie * FallbackRadius);

	FVector FallbackLocation = Preferred;
	if (UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World))
	{
		FNavLocation NavLoc;
		if (NavSys->ProjectPointToNavigation(Preferred, NavLoc, FVector(400.f, 400.f, 500.f)))
		{
			FallbackLocation = NavLoc.Location;
		}
	}

	const EPathFollowingRequestResult::Type MoveResult = MoveToLocation(FallbackLocation, Acceptance, true, true, false);
	if (MoveResult != EPathFollowingRequestResult::Failed)
	{
		ConsecutivePathFailures = 0;
		return true;
	}

	return false;
}

AAZombieCharacter* AAZombieAIController::GetZombieCharacter() const
{
	return Cast<AAZombieCharacter>(GetPawn());
}

const UUZombieConfigDataAsset* AAZombieAIController::GetZombieConfig() const
{
	const AAZombieCharacter* Zombie = GetZombieCharacter();
	return Zombie ? Zombie->GetZombieConfig() : nullptr;
}

bool AAZombieAIController::IsTargetActorAlive(AActor* TargetActor) const
{
	if (!TargetActor)
	{
		return false;
	}

	if (TargetActor->IsPendingKillPending())
	{
		return false;
	}

	if (TargetActor->IsHidden() || !TargetActor->GetActorEnableCollision())
	{
		return false;
	}

	static const FName DeadPropertyNames[] = {
		FName(TEXT("bIsDead")),
		FName(TEXT("IsDead")),
		FName(TEXT("bDead")),
		FName(TEXT("Dead"))
	};

	for (const FName& PropertyName : DeadPropertyNames)
	{
		if (const FBoolProperty* DeadProp = FindFProperty<FBoolProperty>(TargetActor->GetClass(), PropertyName))
		{
			if (DeadProp->GetPropertyValue_InContainer(TargetActor))
			{
				return false;
			}
		}
	}

	for (TFieldIterator<FBoolProperty> It(TargetActor->GetClass()); It; ++It)
	{
		const FString PropName = It->GetName().ToLower();
		if (PropName.Contains(TEXT("dead")) && It->GetPropertyValue_InContainer(TargetActor))
		{
			return false;
		}
	}

	auto IsDeadByFunction = [TargetActor](const FName& FunctionName) -> bool
	{
		UFunction* Fn = TargetActor->FindFunction(FunctionName);
		if (!Fn)
		{
			return false;
		}

		uint8* ParamsBuffer = (Fn->ParmsSize > 0) ? (uint8*)FMemory::Malloc(Fn->ParmsSize) : nullptr;
		if (ParamsBuffer)
		{
			FMemory::Memzero(ParamsBuffer, Fn->ParmsSize);
		}

		TargetActor->ProcessEvent(Fn, ParamsBuffer);

		bool bReturnValue = false;
		if (const FBoolProperty* ReturnValueProp = FindFProperty<FBoolProperty>(Fn, TEXT("ReturnValue")))
		{
			if (ParamsBuffer)
			{
				bReturnValue = ReturnValueProp->GetPropertyValue_InContainer(ParamsBuffer);
			}
		}

		if (ParamsBuffer)
		{
			FMemory::Free(ParamsBuffer);
		}

		return bReturnValue;
	};

	if (IsDeadByFunction(FName(TEXT("IsDead"))) ||
		IsDeadByFunction(FName(TEXT("GetIsDead"))) ||
		IsDeadByFunction(FName(TEXT("IsCompanionDead"))) ||
		IsDeadByFunction(FName(TEXT("IsPlayerDead"))))
	{
		return false;
	}

	return true;
}

