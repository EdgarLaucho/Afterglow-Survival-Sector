#include "USpawnPoolRoundBridgeComponent.h"

#include "URoundConfigDataAsset.h"
#include "SpawnPoolRoundHooks.h"
#include "UObject/UnrealType.h"

UUSpawnPoolRoundBridgeComponent::UUSpawnPoolRoundBridgeComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	bEnableDebugLogs = false;
	ApplyEnemyRoundSettingFunctionName = TEXT("ApplyEnemyRoundSetting");
	StartSpawnTimersFunctionName = TEXT("StartSpawnTimers");
	StopSpawnTimersFunctionName = TEXT("StopSpawnTimers");
	IsRoundCompletedFunctionName = TEXT("IsRoundCompleted");
	OnRoundChangedFunctionName = TEXT("OnRoundChanged");
}

void UUSpawnPoolRoundBridgeComponent::BeginPlay()
{
	Super::BeginPlay();
}

bool UUSpawnPoolRoundBridgeComponent::ApplyRoundConfig(UURoundConfigDataAsset* RoundConfig, int32 RoundIndex)
{
	if (bEnableDebugLogs)
	{
		UE_LOG(LogTemp, Warning, TEXT("[RoundBridge] ApplyRoundConfig RoundIndex=%d"), RoundIndex);
	}

	if (!RoundConfig)
	{
		if (bEnableDebugLogs)
		{
			UE_LOG(LogTemp, Warning, TEXT("[RoundBridge] ApplyRoundConfig failed: RoundConfig is null."));
		}
		return false;
	}

	FRoundDefinition RoundDefinition;
	if (!RoundConfig->GetRoundDefinition(RoundIndex, RoundDefinition))
	{
		if (bEnableDebugLogs)
		{
			UE_LOG(LogTemp, Warning, TEXT("[RoundBridge] ApplyRoundConfig failed: invalid RoundIndex=%d."), RoundIndex);
		}
		return false;
	}

	AActor* OwnerActor = GetPoolOwner();
	if (OwnerActor)
	{
		TSet<int32> EnabledTypeIndices;
		for (const FEnemyRoundSettings& Settings : RoundDefinition.EnemySettings)
		{
			EnabledTypeIndices.Add(Settings.EnemyTypeIndex);
		}

		if (const FArrayProperty* ConfigsArrayProp = FindFProperty<FArrayProperty>(OwnerActor->GetClass(), TEXT("EnemySpawnConfigs")))
		{
			void* ArrayContainerPtr = ConfigsArrayProp->ContainerPtrToValuePtr<void>(OwnerActor);
			FScriptArrayHelper ArrayHelper(ConfigsArrayProp, ArrayContainerPtr);

			for (int32 i = 0; i < ArrayHelper.Num(); ++i)
			{
				if (!EnabledTypeIndices.Contains(i))
				{
					// Desactivar tipos no presentes en la ronda actual.
					if (bEnableDebugLogs)
					{
						UE_LOG(LogTemp, Warning, TEXT("[RoundBridge] Disabling EnemyTypeIndex=%d (not in round)."), i);
					}
					ApplyEnemyRoundSettingByReflection(OwnerActor, i, 0, 0, 9999.f);
				}
			}
		}
	}

	bool bAnyApplied = false;
	for (const FEnemyRoundSettings& Settings : RoundDefinition.EnemySettings)
	{
		if (bEnableDebugLogs)
		{
			UE_LOG(LogTemp, Warning, TEXT("[RoundBridge] Applying EnemyTypeIndex=%d MaxTotal=%d MaxAlive=%d Interval=%.2f"), Settings.EnemyTypeIndex, Settings.MaxTotalSpawns, Settings.MaxAlive, Settings.SpawnInterval);
		}
		bAnyApplied |= CallApplyEnemyRoundSetting(
			Settings.EnemyTypeIndex,
			Settings.MaxTotalSpawns,
			Settings.MaxAlive,
			Settings.SpawnInterval
		);
	}

	return bAnyApplied;
}

void UUSpawnPoolRoundBridgeComponent::StartSpawnTimers()
{
	if (bEnableDebugLogs)
	{
		UE_LOG(LogTemp, Warning, TEXT("[RoundBridge] StartSpawnTimers requested."));
	}

	if (AActor* OwnerActor = GetPoolOwner())
	{
		if (OwnerActor->GetClass()->ImplementsInterface(USpawnPoolRoundHooks::StaticClass()))
		{
			ISpawnPoolRoundHooks::Execute_StartSpawnTimers(OwnerActor);
			return;
		}
	}

	CallVoidFunction(StartSpawnTimersFunctionName);
}

void UUSpawnPoolRoundBridgeComponent::StopSpawnTimers()
{
	if (bEnableDebugLogs)
	{
		UE_LOG(LogTemp, Warning, TEXT("[RoundBridge] StopSpawnTimers requested."));
	}

	if (AActor* OwnerActor = GetPoolOwner())
	{
		if (OwnerActor->GetClass()->ImplementsInterface(USpawnPoolRoundHooks::StaticClass()))
		{
			ISpawnPoolRoundHooks::Execute_StopSpawnTimers(OwnerActor);
			return;
		}
	}

	CallVoidFunction(StopSpawnTimersFunctionName);
}

bool UUSpawnPoolRoundBridgeComponent::AreRoundSpawnsCompleted() const
{
	AActor* OwnerActor = GetPoolOwner();
	if (!OwnerActor)
	{
		return false;
	}

	const bool bByReflection = IsRoundCompletedByReflection(OwnerActor);
	if (bEnableDebugLogs)
	{
		UE_LOG(LogTemp, Warning, TEXT("[RoundBridge] Completion(reflection)=%s"), bByReflection ? TEXT("true") : TEXT("false"));
	}

	if (OwnerActor->GetClass()->ImplementsInterface(USpawnPoolRoundHooks::StaticClass()))
	{
		const bool bByInterface = ISpawnPoolRoundHooks::Execute_IsRoundCompleted(OwnerActor);
		if (bEnableDebugLogs)
		{
			UE_LOG(LogTemp, Warning, TEXT("[RoundBridge] Completion(interface)=%s -> final=%s"), bByInterface ? TEXT("true") : TEXT("false"), (bByInterface || bByReflection) ? TEXT("true") : TEXT("false"));
		}
		return bByInterface || bByReflection;
	}

	if (!IsRoundCompletedFunctionName.IsNone() && OwnerActor->FindFunction(IsRoundCompletedFunctionName))
	{
		const bool bByFunction = CallBoolFunction(IsRoundCompletedFunctionName);
		if (bEnableDebugLogs)
		{
			UE_LOG(LogTemp, Warning, TEXT("[RoundBridge] Completion(function=%s)=%s -> final=%s"), *IsRoundCompletedFunctionName.ToString(), bByFunction ? TEXT("true") : TEXT("false"), (bByFunction || bByReflection) ? TEXT("true") : TEXT("false"));
		}
		return bByFunction || bByReflection;
	}

	return bByReflection;
}

void UUSpawnPoolRoundBridgeComponent::NotifyRoundChanged(int32 RoundIndex)
{
	if (AActor* OwnerActor = GetPoolOwner())
	{
		if (OwnerActor->GetClass()->ImplementsInterface(USpawnPoolRoundHooks::StaticClass()))
		{
			ISpawnPoolRoundHooks::Execute_OnRoundChanged(OwnerActor, RoundIndex);
			return;
		}
	}

	CallRoundChanged(RoundIndex);
}

AActor* UUSpawnPoolRoundBridgeComponent::GetPoolOwner() const
{
	return GetOwner();
}

bool UUSpawnPoolRoundBridgeComponent::CallApplyEnemyRoundSetting(int32 EnemyTypeIndex, int32 MaxTotalSpawns, int32 MaxAlive, float SpawnInterval)
{
	AActor* OwnerActor = GetPoolOwner();
	if (!OwnerActor)
	{
		return false;
	}

	// Ruta principal robusta: aplicar directamente por reflexión al array EnemySpawnConfigs.
	const bool bAppliedByReflection = ApplyEnemyRoundSettingByReflection(OwnerActor, EnemyTypeIndex, MaxTotalSpawns, MaxAlive, SpawnInterval);

	if (OwnerActor->GetClass()->ImplementsInterface(USpawnPoolRoundHooks::StaticClass()))
	{
		// Capa opcional para lógica de BP (UI/debug), sin depender de ella para aplicar datos.
		ISpawnPoolRoundHooks::Execute_ApplyEnemyRoundSetting(OwnerActor, EnemyTypeIndex, MaxTotalSpawns, MaxAlive, SpawnInterval);
	}

	// Ruta secundaria opcional: llamar a Blueprint para lógica adicional (debug/UI/telemetría).
	if (ApplyEnemyRoundSettingFunctionName.IsNone())
	{
		return bAppliedByReflection;
	}

	UFunction* Fn = OwnerActor->FindFunction(ApplyEnemyRoundSettingFunctionName);
	if (!Fn)
	{
		if (!bAppliedByReflection)
		{
			UE_LOG(LogTemp, Warning, TEXT("SpawnPoolRoundBridge: function '%s' not found on '%s' and reflection apply failed."), *ApplyEnemyRoundSettingFunctionName.ToString(), *OwnerActor->GetName());
		}
		return bAppliedByReflection;
	}

	struct FApplyEnemyRoundSettingParams
	{
		int32 EnemyTypeIndex;
		int32 MaxTotalSpawns;
		int32 MaxAlive;
		float SpawnInterval;
	};

	FApplyEnemyRoundSettingParams Params;
	Params.EnemyTypeIndex = EnemyTypeIndex;
	Params.MaxTotalSpawns = MaxTotalSpawns;
	Params.MaxAlive = MaxAlive;
	Params.SpawnInterval = SpawnInterval;

	OwnerActor->ProcessEvent(Fn, &Params);

	if (!bAppliedByReflection)
	{
		UE_LOG(LogTemp, Warning, TEXT("SpawnPoolRoundBridge: apply executed via BP only on '%s'."), *OwnerActor->GetName());
	}

	return bAppliedByReflection;
}

bool UUSpawnPoolRoundBridgeComponent::ApplyEnemyRoundSettingByReflection(AActor* OwnerActor, int32 EnemyTypeIndex, int32 MaxTotalSpawns, int32 MaxAlive, float SpawnInterval) const
{
	if (!OwnerActor)
	{
		return false;
	}

	const FArrayProperty* ConfigsArrayProp = FindFProperty<FArrayProperty>(OwnerActor->GetClass(), TEXT("EnemySpawnConfigs"));
	if (!ConfigsArrayProp)
	{
		UE_LOG(LogTemp, Warning, TEXT("SpawnPoolRoundBridge: EnemySpawnConfigs property not found on '%s'."), *OwnerActor->GetName());
		return false;
	}

	const FStructProperty* StructInner = CastField<FStructProperty>(ConfigsArrayProp->Inner);
	if (!StructInner || !StructInner->Struct)
	{
		UE_LOG(LogTemp, Warning, TEXT("SpawnPoolRoundBridge: EnemySpawnConfigs is not a struct array on '%s'."), *OwnerActor->GetName());
		return false;
	}

	void* ArrayContainerPtr = ConfigsArrayProp->ContainerPtrToValuePtr<void>(OwnerActor);
	FScriptArrayHelper ArrayHelper(ConfigsArrayProp, ArrayContainerPtr);
	if (!ArrayHelper.IsValidIndex(EnemyTypeIndex))
	{
		UE_LOG(LogTemp, Warning, TEXT("SpawnPoolRoundBridge: EnemyTypeIndex %d out of range on '%s'."), EnemyTypeIndex, *OwnerActor->GetName());
		return false;
	}

	void* ElementPtr = ArrayHelper.GetRawPtr(EnemyTypeIndex);
	UStruct* ElementStruct = StructInner->Struct;

	auto SetIntField = [ElementStruct, ElementPtr](const FName& FieldName, int32 Value) -> bool
	{
		if (const FIntProperty* IntProp = FindFProperty<FIntProperty>(ElementStruct, FieldName))
		{
			void* ValuePtr = IntProp->ContainerPtrToValuePtr<void>(ElementPtr);
			IntProp->SetPropertyValue(ValuePtr, Value);
			return true;
		}
		return false;
	};

	auto SetFloatField = [ElementStruct, ElementPtr](const FName& FieldName, float Value) -> bool
	{
		if (const FFloatProperty* FloatProp = FindFProperty<FFloatProperty>(ElementStruct, FieldName))
		{
			void* ValuePtr = FloatProp->ContainerPtrToValuePtr<void>(ElementPtr);
			FloatProp->SetPropertyValue(ValuePtr, Value);
			return true;
		}
		return false;
	};

	bool bApplied = false;
	bApplied |= SetIntField(TEXT("MaxTotalSpawns"), MaxTotalSpawns);
	bApplied |= SetIntField(TEXT("MaxAlive"), MaxAlive);
	bApplied |= SetFloatField(TEXT("SpawnInterval"), SpawnInterval);

	// Round reset values
	SetIntField(TEXT("TotalSpawned"), 0);
	SetIntField(TEXT("CurrentAlive"), 0);
	SetIntField(TEXT("NextIndex"), 0);
	SetFloatField(TEXT("SpawnElapsedTime"), 0.f);

	if (!bApplied)
	{
		UE_LOG(LogTemp, Warning, TEXT("SpawnPoolRoundBridge: reflection fallback could not set round fields on '%s'."), *OwnerActor->GetName());
	}
	else if (bEnableDebugLogs)
	{
		UE_LOG(LogTemp, Warning, TEXT("[RoundBridge] Reflection apply index=%d MaxTotal=%d MaxAlive=%d Interval=%.2f"), EnemyTypeIndex, MaxTotalSpawns, MaxAlive, SpawnInterval);
	}

	return bApplied;
}

bool UUSpawnPoolRoundBridgeComponent::IsRoundCompletedByReflection(AActor* OwnerActor) const
{
	if (!OwnerActor)
	{
		return false;
	}

	const FArrayProperty* ConfigsArrayProp = FindFProperty<FArrayProperty>(OwnerActor->GetClass(), TEXT("EnemySpawnConfigs"));
	if (!ConfigsArrayProp)
	{
		UE_LOG(LogTemp, Warning, TEXT("SpawnPoolRoundBridge: EnemySpawnConfigs property not found on '%s' while checking round completion."), *OwnerActor->GetName());
		return false;
	}

	const FStructProperty* StructInner = CastField<FStructProperty>(ConfigsArrayProp->Inner);
	if (!StructInner || !StructInner->Struct)
	{
		UE_LOG(LogTemp, Warning, TEXT("SpawnPoolRoundBridge: EnemySpawnConfigs is not a struct array on '%s' while checking round completion."), *OwnerActor->GetName());
		return false;
	}

	void* ArrayContainerPtr = ConfigsArrayProp->ContainerPtrToValuePtr<void>(OwnerActor);
	FScriptArrayHelper ArrayHelper(ConfigsArrayProp, ArrayContainerPtr);

	UStruct* ElementStruct = StructInner->Struct;
	const FIntProperty* MaxTotalSpawnsProp = FindFProperty<FIntProperty>(ElementStruct, TEXT("MaxTotalSpawns"));
	const FIntProperty* TotalSpawnedProp = FindFProperty<FIntProperty>(ElementStruct, TEXT("TotalSpawned"));
	const FIntProperty* CurrentAliveProp = FindFProperty<FIntProperty>(ElementStruct, TEXT("CurrentAlive"));

	if (!MaxTotalSpawnsProp || !TotalSpawnedProp || !CurrentAliveProp)
	{
		UE_LOG(LogTemp, Warning, TEXT("SpawnPoolRoundBridge: round completion fields missing on EnemySpawnConfigs struct in '%s'."), *OwnerActor->GetName());
		return false;
	}

	for (int32 i = 0; i < ArrayHelper.Num(); ++i)
	{
		void* ElementPtr = ArrayHelper.GetRawPtr(i);

		const int32 MaxTotalSpawns = MaxTotalSpawnsProp->GetPropertyValue(MaxTotalSpawnsProp->ContainerPtrToValuePtr<void>(ElementPtr));
		const int32 TotalSpawned = TotalSpawnedProp->GetPropertyValue(TotalSpawnedProp->ContainerPtrToValuePtr<void>(ElementPtr));
		const int32 CurrentAlive = CurrentAliveProp->GetPropertyValue(CurrentAliveProp->ContainerPtrToValuePtr<void>(ElementPtr));

		// Tipos desactivados en esta ronda no bloquean la transición.
		if (MaxTotalSpawns <= 0)
		{
			if (bEnableDebugLogs)
			{
				UE_LOG(LogTemp, Warning, TEXT("[RoundBridge] Check index=%d skipped (MaxTotalSpawns<=0)."), i);
			}
			continue;
		}

		if (bEnableDebugLogs)
		{
			UE_LOG(LogTemp, Warning, TEXT("[RoundBridge] Check index=%d TotalSpawned=%d MaxTotal=%d CurrentAlive=%d"), i, TotalSpawned, MaxTotalSpawns, CurrentAlive);
		}

		if (TotalSpawned < MaxTotalSpawns || CurrentAlive > 0)
		{
			if (bEnableDebugLogs)
			{
				UE_LOG(LogTemp, Warning, TEXT("[RoundBridge] Check failed at index=%d"), i);
			}
			return false;
		}
	}

	if (bEnableDebugLogs)
	{
		UE_LOG(LogTemp, Warning, TEXT("[RoundBridge] Reflection completion = true"));
	}

	return true;
}

void UUSpawnPoolRoundBridgeComponent::CallVoidFunction(FName FunctionName) const
{
	AActor* OwnerActor = GetPoolOwner();
	if (!OwnerActor || FunctionName.IsNone())
	{
		return;
	}

	if (UFunction* Fn = OwnerActor->FindFunction(FunctionName))
	{
		OwnerActor->ProcessEvent(Fn, nullptr);
	}
}

bool UUSpawnPoolRoundBridgeComponent::CallBoolFunction(FName FunctionName) const
{
	AActor* OwnerActor = GetPoolOwner();
	if (!OwnerActor || FunctionName.IsNone())
	{
		return false;
	}

	UFunction* Fn = OwnerActor->FindFunction(FunctionName);
	if (!Fn)
	{
		UE_LOG(LogTemp, Warning, TEXT("SpawnPoolRoundBridge: bool function '%s' not found on '%s'."), *FunctionName.ToString(), *OwnerActor->GetName());
		return false;
	}

	if (Fn->ParmsSize <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("SpawnPoolRoundBridge: bool function '%s' has invalid parameter size on '%s'."), *FunctionName.ToString(), *OwnerActor->GetName());
		return false;
	}

	uint8* ParamsBuffer = (Fn->ParmsSize > 0) ? (uint8*)FMemory::Malloc(Fn->ParmsSize) : nullptr;
	if (ParamsBuffer)
	{
		FMemory::Memzero(ParamsBuffer, Fn->ParmsSize);
	}

	OwnerActor->ProcessEvent(Fn, ParamsBuffer);

	bool bReturnValue = false;
	if (const FBoolProperty* ReturnValueProp = CastField<FBoolProperty>(Fn->GetReturnProperty()))
	{
		if (ParamsBuffer)
		{
			const void* ReturnValuePtr = ReturnValueProp->ContainerPtrToValuePtr<void>(ParamsBuffer);
			if (ReturnValuePtr)
			{
				bReturnValue = ReturnValueProp->GetPropertyValue(ReturnValuePtr);
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("SpawnPoolRoundBridge: bool function '%s' has no bool return property on '%s'."), *FunctionName.ToString(), *OwnerActor->GetName());
	}

	if (ParamsBuffer)
	{
		FMemory::Free(ParamsBuffer);
	}

	return bReturnValue;
}

void UUSpawnPoolRoundBridgeComponent::CallRoundChanged(int32 RoundIndex) const
{
	AActor* OwnerActor = GetPoolOwner();
	if (!OwnerActor || OnRoundChangedFunctionName.IsNone())
	{
		return;
	}

	UFunction* Fn = OwnerActor->FindFunction(OnRoundChangedFunctionName);
	if (!Fn)
	{
		return;
	}

	struct FRoundChangedParams
	{
		int32 RoundIndex;
	};

	FRoundChangedParams Params;
	Params.RoundIndex = RoundIndex;

	OwnerActor->ProcessEvent(Fn, &Params);
}

