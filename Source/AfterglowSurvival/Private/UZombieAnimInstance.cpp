#include "UZombieAnimInstance.h"

#include "AZombieCharacter.h"

void UUZombieAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	CachedZombie = Cast<AAZombieCharacter>(TryGetPawnOwner());

	Speed = 0.f;
	Direction = 0.f;
	bIsMoving = false;
	bIsDead = false;
	bIsAttacking = false;
}

void UUZombieAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (!CachedZombie)
	{
		CachedZombie = Cast<AAZombieCharacter>(TryGetPawnOwner());
	}

	if (!CachedZombie)
	{
		Speed = 0.f;
		Direction = 0.f;
		bIsMoving = false;
		bIsDead = false;
		bIsAttacking = false;
		return;
	}

	CachedZombie->GetAnimMoveData(Speed, Direction, bIsAttacking, bIsDead);
	bIsMoving = Speed > 5.f;
}

void UUZombieAnimInstance::HandleAttackHitWindowNotify()
{
	if (CachedZombie)
	{
		CachedZombie->PerformAttackHit();
	}
}

void UUZombieAnimInstance::HandleAttackEndNotify()
{
	if (CachedZombie)
	{
		CachedZombie->OnAttackFinished();
	}
}

void UUZombieAnimInstance::HandleDeathFinishedNotify()
{
	if (CachedZombie)
	{
		CachedZombie->OnDeathFinished();
	}
}

