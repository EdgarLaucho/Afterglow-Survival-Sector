// Fill out your copyright notice in the Description page of Project Settings.


#include "UZombieTargetRegistrySubsystem.h"

#include "Kismet/GameplayStatics.h"
#include "GameFramework/Pawn.h"
#include "AZombieCharacter.h"
#include "CompanionCharacter.h"

AActor* UUZombieTargetRegistrySubsystem::GetClosestTarget(const FVector& FromLocation, const APawn* Requester, float MaxDistance)
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return nullptr;
    }

    const float now = World->GetTimeSeconds();
    if ((now - LastRefreshTime) >= RefreshInterval || CachedTargets.Num() == 0)
    {
        RefreshTargets();
        LastRefreshTime = now;
    }

    AActor* BestTarget = nullptr;
    float BestDistSq = FMath::Square(MaxDistance > 0.f ? MaxDistance : FLT_MAX);

    for (const TWeakObjectPtr<APawn>& TargetPtr : CachedTargets)
    {
        const APawn* TargetPawn = TargetPtr.Get();
        if (!IsValidTargetPawn(TargetPawn, Requester))
        {
            continue;
        }

        const float DistSq = FVector::DistSquared(TargetPawn->GetActorLocation(), FromLocation);
        if (DistSq < BestDistSq)
        {
            BestDistSq = DistSq;
            BestTarget = const_cast<APawn*>(TargetPawn);
        }
    }

    return BestTarget;
}

void UUZombieTargetRegistrySubsystem::RefreshTargets()
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    CachedTargets.Reset();

    for (int32 PlayerIndex = 0; PlayerIndex < 4; ++PlayerIndex)
    {
        if (APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(World, PlayerIndex))
        {
            CachedTargets.Add(PlayerPawn);
        }
    }

    TArray<AActor*> CompanionActors;
    UGameplayStatics::GetAllActorsOfClass(World, ACompanionCharacter::StaticClass(), CompanionActors);
    for (AActor* Actor : CompanionActors)
    {
        if (APawn* CompanionPawn = Cast<APawn>(Actor))
        {
            CachedTargets.AddUnique(CompanionPawn);
        }
    }

    TArray<AActor*> TaggedTargets;
    UGameplayStatics::GetAllActorsWithTag(World, FName(TEXT("ZombieTarget")), TaggedTargets);
    for (AActor* Actor : TaggedTargets)
    {
        if (APawn* TaggedPawn = Cast<APawn>(Actor))
        {
            CachedTargets.AddUnique(TaggedPawn);
        }
    }
}

bool UUZombieTargetRegistrySubsystem::IsValidTargetPawn(const APawn* Pawn, const APawn* Requester) const
{
    if (!Pawn)
    {
        return false;
    }

    if (Pawn == Requester)
    {
        return false;
    }

    if (Pawn->IsPendingKillPending())
    {
        return false;
    }

    if (Pawn->IsHidden() || Pawn->GetActorEnableCollision() == false)
    {
        return false;
    }

    if (Pawn->IsA<AAZombieCharacter>())
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
        if (const FBoolProperty* DeadProp = FindFProperty<FBoolProperty>(Pawn->GetClass(), PropertyName))
        {
            if (DeadProp->GetPropertyValue_InContainer(Pawn))
            {
                return false;
            }
        }
    }

    for (TFieldIterator<FBoolProperty> It(Pawn->GetClass()); It; ++It)
    {
        const FString PropName = It->GetName().ToLower();
        if (PropName.Contains(TEXT("dead")) && It->GetPropertyValue_InContainer(Pawn))
        {
            return false;
        }
    }

    auto IsDeadByFunction = [Pawn](const FName& FunctionName) -> bool
    {
        UFunction* Fn = Pawn->FindFunction(FunctionName);
        if (!Fn)
        {
            return false;
        }

        uint8* ParamsBuffer = (Fn->ParmsSize > 0) ? (uint8*)FMemory::Malloc(Fn->ParmsSize) : nullptr;
        if (ParamsBuffer)
        {
            FMemory::Memzero(ParamsBuffer, Fn->ParmsSize);
        }

        const_cast<APawn*>(Pawn)->ProcessEvent(Fn, ParamsBuffer);

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

