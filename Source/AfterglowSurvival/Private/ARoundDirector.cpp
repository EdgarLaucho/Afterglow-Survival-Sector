#include "ARoundDirector.h"

#include "URoundConfigDataAsset.h"
#include "USpawnPoolRoundBridgeComponent.h"

#include "Engine/World.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"

AARoundDirector::AARoundDirector()
{
	PrimaryActorTick.bCanEverTick = false;
	bEnableDebugLogs = false;
	bAutoStartOnBeginPlay = true;
	RoundCheckInterval = 0.25f;
	CurrentRoundIndex = -1;
	bRoundActive = false;
	PendingIntermissionSeconds = 5.0f;
	bAutoResolveSpawnPoolBridge = true;
	SpawnPoolActorTag = TEXT("SpawnPoolEnemyType");
}

void AARoundDirector::BeginPlay()
{
	Super::BeginPlay();

	if (bEnableDebugLogs)
	{
		UE_LOG(LogTemp, Warning, TEXT("[RoundDirector] BeginPlay"));
	}

	ResolveSpawnPoolBridge();

	if (bAutoStartOnBeginPlay)
	{
		StartRounds();
	}
}

void AARoundDirector::ResolveSpawnPoolBridge()
{
	if (SpawnPoolBridge)
	{
		return;
	}

	SpawnPoolBridge = FindComponentByClass<UUSpawnPoolRoundBridgeComponent>();
	if (bEnableDebugLogs)
	{
		UE_LOG(LogTemp, Warning, TEXT("[RoundDirector] Resolve local bridge: %s"), SpawnPoolBridge ? TEXT("found") : TEXT("not found"));
	}
	if (SpawnPoolBridge || !bAutoResolveSpawnPoolBridge)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (!SpawnPoolActorTag.IsNone())
	{
		TArray<AActor*> TaggedActors;
		UGameplayStatics::GetAllActorsWithTag(World, SpawnPoolActorTag, TaggedActors);
		for (AActor* Actor : TaggedActors)
		{
			if (!Actor) continue;
			if (UUSpawnPoolRoundBridgeComponent* Bridge = Actor->FindComponentByClass<UUSpawnPoolRoundBridgeComponent>())
			{
				SpawnPoolBridge = Bridge;
				if (bEnableDebugLogs)
				{
					UE_LOG(LogTemp, Warning, TEXT("[RoundDirector] Bridge found by tag on actor '%s'."), *Actor->GetName());
				}
				return;
			}
		}
	}

	TArray<AActor*> AllActors;
	UGameplayStatics::GetAllActorsOfClass(World, AActor::StaticClass(), AllActors);
	for (AActor* Actor : AllActors)
	{
		if (!Actor || Actor == this) continue;
		if (UUSpawnPoolRoundBridgeComponent* Bridge = Actor->FindComponentByClass<UUSpawnPoolRoundBridgeComponent>())
		{
			SpawnPoolBridge = Bridge;
			if (bEnableDebugLogs)
			{
				UE_LOG(LogTemp, Warning, TEXT("[RoundDirector] Bridge found by fallback scan on actor '%s'."), *Actor->GetName());
			}
			return;
		}
	}
}

void AARoundDirector::StartRounds()
{
	if (bEnableDebugLogs)
	{
		UE_LOG(LogTemp, Warning, TEXT("[RoundDirector] StartRounds called."));
	}

	if (!RoundConfig || !SpawnPoolBridge)
	{
		UE_LOG(LogTemp, Warning, TEXT("RoundDirector: missing RoundConfig or SpawnPoolBridge."));
		return;
	}

	StopRounds();
	StartRound(0);
}

void AARoundDirector::StopRounds()
{
	bRoundActive = false;
	CurrentRoundIndex = -1;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RoundCheckTimer);
		World->GetTimerManager().ClearTimer(IntermissionTimer);
	}

	if (SpawnPoolBridge)
	{
		SpawnPoolBridge->StopSpawnTimers();
	}
}

void AARoundDirector::StartRound(int32 RoundIndex)
{
	if (bEnableDebugLogs)
	{
		UE_LOG(LogTemp, Warning, TEXT("[RoundDirector] StartRound index=%d"), RoundIndex);
	}

	if (!RoundConfig || !SpawnPoolBridge)
	{
		return;
	}

	FRoundDefinition RoundDef;
	if (!RoundConfig->GetRoundDefinition(RoundIndex, RoundDef))
	{
		if (bEnableDebugLogs)
		{
			UE_LOG(LogTemp, Warning, TEXT("[RoundDirector] No round definition for index=%d. Finishing rounds."), RoundIndex);
		}
		bRoundActive = false;
		SpawnPoolBridge->StopSpawnTimers();
		BP_OnRoundsFinished();
		return;
	}

	CurrentRoundIndex = RoundIndex;
	bRoundActive = true;
	PendingIntermissionSeconds = RoundDef.IntermissionSeconds;

	SpawnPoolBridge->NotifyRoundChanged(CurrentRoundIndex);
	SpawnPoolBridge->ApplyRoundConfig(RoundConfig, CurrentRoundIndex);
	SpawnPoolBridge->StartSpawnTimers();

	if (bEnableDebugLogs)
	{
		UE_LOG(LogTemp, Warning, TEXT("[RoundDirector] Round %d started. Intermission=%.2f"), CurrentRoundIndex, PendingIntermissionSeconds);
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(IntermissionTimer);
		World->GetTimerManager().SetTimer(
			RoundCheckTimer,
			this,
			&AARoundDirector::EvaluateRoundCompletion,
			FMath::Max(0.05f, RoundCheckInterval),
			true
		);
	}

	BP_OnRoundStarted(CurrentRoundIndex);
}

void AARoundDirector::StartIntermission(float IntermissionSeconds)
{
	if (bEnableDebugLogs)
	{
		UE_LOG(LogTemp, Warning, TEXT("[RoundDirector] StartIntermission after round %d for %.2fs"), CurrentRoundIndex, IntermissionSeconds);
	}

	bRoundActive = false;

	if (!SpawnPoolBridge)
	{
		return;
	}

	SpawnPoolBridge->StopSpawnTimers();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RoundCheckTimer);
		World->GetTimerManager().SetTimer(
			IntermissionTimer,
			this,
			&AARoundDirector::HandleIntermissionFinished,
			FMath::Max(0.01f, IntermissionSeconds),
			false
		);
	}

	BP_OnIntermissionStarted(CurrentRoundIndex, IntermissionSeconds);
}

void AARoundDirector::HandleIntermissionFinished()
{
	const int32 NextRoundIndex = CurrentRoundIndex + 1;
	if (bEnableDebugLogs)
	{
		UE_LOG(LogTemp, Warning, TEXT("[RoundDirector] Intermission finished. NextRound=%d"), NextRoundIndex);
	}
	StartRound(NextRoundIndex);
}

void AARoundDirector::EvaluateRoundCompletion()
{
	if (!bRoundActive || !SpawnPoolBridge)
	{
		return;
	}

	if (SpawnPoolBridge->AreRoundSpawnsCompleted())
	{
		if (bEnableDebugLogs)
		{
			UE_LOG(LogTemp, Warning, TEXT("[RoundDirector] Round %d completed."), CurrentRoundIndex);
		}
		StartIntermission(PendingIntermissionSeconds);
	}
}

