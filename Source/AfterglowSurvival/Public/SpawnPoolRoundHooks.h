#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "SpawnPoolRoundHooks.generated.h"

UINTERFACE(BlueprintType)
class AFTERGLOWSURVIVAL_API USpawnPoolRoundHooks : public UInterface
{
	GENERATED_BODY()
};

class AFTERGLOWSURVIVAL_API ISpawnPoolRoundHooks
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Rounds|SpawnPool")
	void ApplyEnemyRoundSetting(int32 EnemyTypeIndex, int32 MaxTotalSpawns, int32 MaxAlive, float SpawnInterval);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Rounds|SpawnPool")
	void StartSpawnTimers();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Rounds|SpawnPool")
	void StopSpawnTimers();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Rounds|SpawnPool")
	bool IsRoundCompleted() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Rounds|SpawnPool")
	void OnRoundChanged(int32 RoundIndex);
};
