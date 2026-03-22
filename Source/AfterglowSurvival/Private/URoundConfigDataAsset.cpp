// Fill out your copyright notice in the Description page of Project Settings.


#include "URoundConfigDataAsset.h"

UURoundConfigDataAsset::UURoundConfigDataAsset()
	: DefaultIntermissionSeconds(5.0f)
{
}

bool UURoundConfigDataAsset::GetRoundDefinition(int32 RoundIndex, FRoundDefinition& OutRound) const
{
	if (!Rounds.IsValidIndex(RoundIndex))
	{
		return false;
	}

	OutRound = Rounds[RoundIndex];
	if (OutRound.IntermissionSeconds <= 0.f)
	{
		OutRound.IntermissionSeconds = DefaultIntermissionSeconds;
	}

	return true;
}

