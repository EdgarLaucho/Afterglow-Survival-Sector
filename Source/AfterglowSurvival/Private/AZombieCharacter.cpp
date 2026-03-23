#include "AZombieCharacter.h"

#include "AZombieAIController.h"
#include "UZombieConfigDataAsset.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "Components/SkeletalMeshComponent.h"

AAZombieCharacter::AAZombieCharacter()
{
	PrimaryActorTick.bCanEverTick = false;
	AIControllerClass = AAZombieAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
   bUseControllerRotationYaw = false;

	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->bOrientRotationToMovement = true;
		MoveComp->bUseControllerDesiredRotation = false;
	}

	AttackTraceSocketName = TEXT("AttackSocket");
	bIsDead = false;
	bIsAttacking = false;
	bPoolActive = true;
	bIsActive = true;
	SpawnPoolRef = nullptr;
	TypeIndex = INDEX_NONE;
	NotifyEnemyReturnedFunctionName = TEXT("NotifyEnemyReturned");
	ZombieCorpseClass = nullptr;
	ZombieCorpseSpawnOffset = FVector::ZeroVector;
	CurrentHealth = 100.f;
	LastAttackTime = -1000.f;
}

void AAZombieCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (ZombieConfig)
	{
		CurrentHealth = ZombieConfig->MaxHealth;
	}
}

float AAZombieCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (!IsZombieAlive() || DamageAmount <= 0.f)
	{
		return 0.f;
	}

	const float AppliedDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	const float EffectiveDamage = AppliedDamage > 0.f ? AppliedDamage : DamageAmount;
	CurrentHealth -= EffectiveDamage;

	if (CurrentHealth <= 0.f)
	{
		Die();
	}

	return EffectiveDamage;
}

void AAZombieCharacter::ActivateFromPool(FVector NewLocation)
{
	bPoolActive = true;
	bIsActive = true;
	bIsDead = false;
	bIsAttacking = false;
	CurrentTarget = nullptr;
	LastAttackTime = -1000.f;

	SetActorLocation(NewLocation, false, nullptr, ETeleportType::TeleportPhysics);
	SetActorHiddenInGame(false);
	SetActorEnableCollision(true);
	SetActorTickEnabled(true);

	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->SetMovementMode(MOVE_Walking);
	}

	CurrentHealth = ZombieConfig ? ZombieConfig->MaxHealth : 100.f;
	BP_OnActivatedFromPool();
}

void AAZombieCharacter::DeactivateToPool()
{
	if (!bIsActive)
	{
		SetActorHiddenInGame(true);
		SetActorEnableCollision(false);
		SetActorTickEnabled(false);
		bPoolActive = false;
		bIsActive = false;
		return;
	}

	NotifyEnemyReturnedToPool();

	bPoolActive = false;
	bIsActive = false;
	bIsAttacking = false;
	CurrentTarget = nullptr;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(AttackRetryTimer);
		World->GetTimerManager().ClearTimer(AttackFinishFailsafeTimer);
	}

	SetActorEnableCollision(false);
	SetActorHiddenInGame(true);
	SetActorTickEnabled(false);

	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->StopMovementImmediately();
		MoveComp->DisableMovement();
	}

	if (AAIController* AI = Cast<AAIController>(GetController()))
	{
		AI->StopMovement();
	}

	BP_OnDeactivatedToPool();
}

void AAZombieCharacter::ConfigurePoolContext(AActor* InSpawnPoolRef, int32 InTypeIndex, bool bStartActive)
{
	SpawnPoolRef = InSpawnPoolRef;
	TypeIndex = InTypeIndex;
	bIsActive = bStartActive;
}

void AAZombieCharacter::SetCurrentTarget(AActor* NewTarget)
{
	CurrentTarget = NewTarget;
}

bool AAZombieCharacter::TryAttackTarget()
{
	if (!IsZombieAlive() || !IsValidCombatTarget(CurrentTarget))
	{
		return false;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	const float AttackCooldown = ZombieConfig ? ZombieConfig->AttackCooldown : 1.f;
	const float now = World->GetTimeSeconds();
	if ((now - LastAttackTime) < AttackCooldown || bIsAttacking)
	{
		return false;
	}

	bIsAttacking = true;
	LastAttackTime = now;

	const float FailsafeDelay = FMath::Max(0.2f, AttackCooldown);
	World->GetTimerManager().SetTimer(
		AttackFinishFailsafeTimer,
		this,
		&AAZombieCharacter::OnAttackFinished,
		FailsafeDelay,
		false
	);

	BP_OnAttackStarted();
	return true;
}

void AAZombieCharacter::PerformAttackHit()
{
	if (!IsZombieAlive() || !IsValidCombatTarget(CurrentTarget))
	{
		return;
	}

	AActor* TargetActor = CurrentTarget.Get();
	if (!TargetActor)
	{
		return;
	}

	const float AttackRange = ZombieConfig ? ZombieConfig->AttackRange : 170.f;
	const float Distance = FVector::Dist(GetActorLocation(), TargetActor->GetActorLocation());
	if (Distance > AttackRange)
	{
		return;
	}

	const float Damage = ZombieConfig ? ZombieConfig->AttackDamage : 15.f;
	UGameplayStatics::ApplyDamage(TargetActor, Damage, GetController(), this, nullptr);
}

void AAZombieCharacter::OnAttackFinished()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(AttackFinishFailsafeTimer);
	}

	bIsAttacking = false;
	TryContinueAttackLoop();
}

void AAZombieCharacter::OnDeathFinished()
{
	if (ZombieCorpseClass)
	{
		if (UWorld* World = GetWorld())
		{
			FTransform CorpseTransform = GetActorTransform();
			CorpseTransform.AddToTranslation(ZombieCorpseSpawnOffset);

            FActorSpawnParameters SpawnParams;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

			AActor* SpawnedCorpse = World->SpawnActor<AActor>(ZombieCorpseClass, CorpseTransform, SpawnParams);
			if (SpawnedCorpse)
			{
               SpawnedCorpse->SetActorHiddenInGame(true);

				UFunction* InitFn = SpawnedCorpse->FindFunction(FName(TEXT("Init")));
				if (InitFn)
				{
					struct FCorpseInitParams
					{
						USkeletalMeshComponent* SourceMesh;
					};

					FCorpseInitParams Params;
					Params.SourceMesh = GetMesh();

					SpawnedCorpse->ProcessEvent(InitFn, &Params);
				}

             SetActorHiddenInGame(true);
				SpawnedCorpse->SetActorHiddenInGame(false);
			}
		}
	}

	if (ZombieConfig && ZombieConfig->bReturnToPoolOnDeath)
	{
		DeactivateToPool();
	}
}

bool AAZombieCharacter::IsZombieAlive() const
{
	return bIsActive && bPoolActive && !bIsDead;
}

float AAZombieCharacter::GetZombieSpeed() const
{
	const FVector Velocity2D(GetVelocity().X, GetVelocity().Y, 0.f);
	return Velocity2D.Size();
}

AActor* AAZombieCharacter::GetCurrentTarget() const
{
	return CurrentTarget.Get();
}

const UUZombieConfigDataAsset* AAZombieCharacter::GetZombieConfig() const
{
	return ZombieConfig;
}

void AAZombieCharacter::GetAnimMoveData(float& OutSpeed, float& OutDirection, bool& bOutIsAttacking, bool& bOutIsDead) const
{
	const FVector Velocity2D = FVector(GetVelocity().X, GetVelocity().Y, 0.f);
	OutSpeed = Velocity2D.Size();

	if (Velocity2D.IsNearlyZero())
	{
		OutDirection = 0.f;
	}
	else
	{
		const FVector MoveDir = Velocity2D.GetSafeNormal();
		const FVector Forward = GetActorForwardVector().GetSafeNormal2D();
		const FVector Right = GetActorRightVector().GetSafeNormal2D();
		const float ForwardDot = FVector::DotProduct(Forward, MoveDir);
		const float RightDot = FVector::DotProduct(Right, MoveDir);
		OutDirection = FMath::RadiansToDegrees(FMath::Atan2(RightDot, ForwardDot));
	}

	bOutIsAttacking = bIsAttacking;
	bOutIsDead = bIsDead;
}

void AAZombieCharacter::TryContinueAttackLoop()
{
	if (!IsZombieAlive() || bIsAttacking || !IsValidCombatTarget(CurrentTarget))
	{
		return;
	}

	AActor* TargetActor = CurrentTarget.Get();
	if (!TargetActor)
	{
		return;
	}

	const float AttackRange = ZombieConfig ? ZombieConfig->AttackRange : 170.f;
	const float Distance = FVector::Dist(GetActorLocation(), TargetActor->GetActorLocation());
	if (Distance > AttackRange)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const float AttackCooldown = ZombieConfig ? ZombieConfig->AttackCooldown : 1.f;
	const float now = World->GetTimeSeconds();
	const float Remaining = FMath::Max(0.f, AttackCooldown - (now - LastAttackTime));
	const float RetryDelay = FMath::Max(Remaining, 0.05f);

	World->GetTimerManager().SetTimer(
		AttackRetryTimer,
		this,
		&AAZombieCharacter::TryContinueAttackLoop,
		RetryDelay,
		false
	);
}

bool AAZombieCharacter::IsValidCombatTarget(AActor* TargetActor) const
{
	if (!TargetActor)
	{
		return false;
	}

	if (TargetActor->IsPendingKillPending())
	{
		return false;
	}

	static const FName IsDeadName(TEXT("IsDead"));
	static const FName bIsDeadName(TEXT("bIsDead"));

	if (const FBoolProperty* IsDeadProp = FindFProperty<FBoolProperty>(TargetActor->GetClass(), IsDeadName))
	{
		if (IsDeadProp->GetPropertyValue_InContainer(TargetActor))
		{
			return false;
		}
	}

	if (const FBoolProperty* bIsDeadProp = FindFProperty<FBoolProperty>(TargetActor->GetClass(), bIsDeadName))
	{
		if (bIsDeadProp->GetPropertyValue_InContainer(TargetActor))
		{
			return false;
		}
	}

	return true;
}

void AAZombieCharacter::Die()
{
	if (bIsDead)
	{
		return;
	}

	bIsDead = true;
	bIsAttacking = false;
	CurrentTarget = nullptr;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(AttackRetryTimer);
		World->GetTimerManager().ClearTimer(AttackFinishFailsafeTimer);
	}

	if (AAIController* AI = Cast<AAIController>(GetController()))
	{
		AI->StopMovement();
	}

	SetActorEnableCollision(false);
	BP_OnDied();
}

void AAZombieCharacter::NotifyEnemyReturnedToPool()
{
	if (!SpawnPoolRef || NotifyEnemyReturnedFunctionName.IsNone())
	{
		return;
	}

	UFunction* NotifyFn = SpawnPoolRef->FindFunction(NotifyEnemyReturnedFunctionName);
	if (!NotifyFn)
	{
		return;
	}

	struct FNotifyEnemyReturnedParams
	{
		int32 TypeIndex;
	};

	FNotifyEnemyReturnedParams Params;
	Params.TypeIndex = TypeIndex;

	SpawnPoolRef->ProcessEvent(NotifyFn, &Params);
}

