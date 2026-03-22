// Fill out your copyright notice in the Description page of Project Settings.


#include "UZombieConfigDataAsset.h"

UUZombieConfigDataAsset::UUZombieConfigDataAsset()
    : MaxHealth(100.f)
    , ThinkInterval(0.2f)
    , TargetRefreshInterval(0.35f)
    , TargetSearchRadius(3000.f)
    , AttackRange(170.f)
    , AttackDamage(15.f)
    , AttackCooldown(1.0f)
    , AttackHitRadius(120.f)
    , MoveAcceptanceRadius(90.f)
    , NoPathFallbackRadius(220.f)
    , MaxConsecutivePathFailures(3)
    , RepathCooldown(0.2f)
    , bReturnToPoolOnDeath(true)
{
}

