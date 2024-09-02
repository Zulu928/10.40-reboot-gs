#include "BuildingActor.h"
#include "FortWeapon.h"
#include "BuildingSMActor.h"
#include "FortPlayerControllerAthena.h"
#include "FortPawn.h"
#include "FortWeaponMeleeItemDefinition.h"
#include "CurveTable.h"
#include "DataTable.h"
#include "FortResourceItemDefinition.h"
#include "FortKismetLibrary.h"
#include "DataTableFunctionLibrary.h"
#include "FortGameStateAthena.h"
#include "GameplayStatics.h"
#include "BuildingContainer.h"

void ABuildingActor::OnDamageServerHook(ABuildingActor* BuildingActor, float Damage, FGameplayTagContainer DamageTags,
    FVector Momentum, /* FHitResult */ __int64 HitInfo, APlayerController* InstigatedBy, AActor* DamageCauser,
    /* FGameplayEffectContextHandle */ __int64 EffectContext)
{
    // Check if BuildingSMActor and BuildingActor are valid
    auto BuildingSMActor = Cast<ABuildingSMActor>(BuildingActor);
    if (!BuildingSMActor || !BuildingActor)
    {
        LOG_ERROR(LogDev, "BuildingSMActor or BuildingActor is null.");
        return;
    }

    // Handle BuildingContainer logic
    if (auto Container = Cast<ABuildingContainer>(BuildingActor))
    {
        if ((BuildingSMActor->GetHealth() <= 0 || BuildingActor->GetHealth() <= 0) && !Container->IsAlreadySearched())
        {
            Container->SpawnLoot();
            LOG_INFO(LogDev, "Loot spawned for container.");
        }
    }

    // Get and check attached building actors
    auto AttachedBuildingActors = BuildingSMActor->GetAttachedBuildingActors();
    if (AttachedBuildingActors.Num() == 0)
    {
        //LOG_WARNING(LogDev, "AttachedBuildingActors is empty.");
    }

    for (int32 i = 0; i < AttachedBuildingActors.Num(); ++i)
    {
        auto CurrentBuildingActor = AttachedBuildingActors.IsValidIndex(i) ? AttachedBuildingActors.at(i) : nullptr;

        if (!CurrentBuildingActor)
        {
            //LOG_WARNING(LogDev, "CurrentBuildingActor is null at index {}", i);
            continue;
        }

        auto CurrentActor = Cast<ABuildingActor>(CurrentBuildingActor);
        if (BuildingSMActor->GetHealth() <= 0 || BuildingActor->GetHealth() <= 0)
        {
            if (auto Container = Cast<ABuildingContainer>(CurrentActor))
            {
                if (!Container->IsAlreadySearched())
                {
                    Container->SpawnLoot();
                    LOG_INFO(LogDev, "Loot spawned for attached building actor.");
                }
            }
        }
    }

    // Validate PlayerController and Weapon
    auto PlayerController = Cast<AFortPlayerControllerAthena>(InstigatedBy);
    auto Weapon = Cast<AFortWeapon>(DamageCauser);
    if (!PlayerController || !Weapon)
    {
        return OnDamageServerOriginal(BuildingActor, Damage, DamageTags, Momentum, HitInfo, InstigatedBy, DamageCauser, EffectContext);
    }

    auto WorldInventory = PlayerController->GetWorldInventory();
    if (!WorldInventory)
    {
        return OnDamageServerOriginal(BuildingActor, Damage, DamageTags, Momentum, HitInfo, InstigatedBy, DamageCauser, EffectContext);
    }

    auto WeaponData = Cast<UFortWeaponMeleeItemDefinition>(Weapon->GetWeaponData());
    if (!WeaponData)
    {
        return OnDamageServerOriginal(BuildingActor, Damage, DamageTags, Momentum, HitInfo, InstigatedBy, DamageCauser, EffectContext);
    }

    UFortResourceItemDefinition* ItemDef = UFortKismetLibrary::K2_GetResourceItemDefinition(BuildingSMActor->GetResourceType());
    if (!ItemDef)
    {
        return OnDamageServerOriginal(BuildingActor, Damage, DamageTags, Momentum, HitInfo, InstigatedBy, DamageCauser, EffectContext);
    }

    // Get and evaluate resource amount
    static auto BuildingResourceAmountOverrideOffset = BuildingSMActor->GetOffset("BuildingResourceAmountOverride");
    auto& BuildingResourceAmountOverride = BuildingSMActor->Get<FCurveTableRowHandle>(BuildingResourceAmountOverrideOffset);

    int ResourceCount = 0;
    if (BuildingResourceAmountOverride.RowName.IsValid())
    {
        UCurveTable* CurveTable = FindObject<UCurveTable>(L"/Game/Athena/Balance/DataTables/AthenaResourceRates.AthenaResourceRates");
        if (CurveTable)
        {
            float Out = UDataTableFunctionLibrary::EvaluateCurveTableRow(CurveTable, BuildingResourceAmountOverride.RowName, 0.f);
            const float DamageThatWillAffect = Damage;
            float skid = Out / (BuildingActor->GetMaxHealth() / DamageThatWillAffect);
            ResourceCount = FMath::RoundToInt(skid);
        }
        else
        {
            LOG_ERROR(LogDev, "CurveTable not found.");
        }
    }

    if (ResourceCount <= 0)
    {
        return OnDamageServerOriginal(BuildingActor, Damage, DamageTags, Momentum, HitInfo, InstigatedBy, DamageCauser, EffectContext);
    }

    bool bIsWeakspot = Damage == 100.0f;
    PlayerController->ClientReportDamagedResourceBuilding(BuildingSMActor, BuildingSMActor->GetResourceType(), ResourceCount, false, bIsWeakspot);

    bool bShouldUpdate = false;
    WorldInventory->AddItem(ItemDef, &bShouldUpdate, ResourceCount);

    if (bShouldUpdate)
    {
        WorldInventory->Update();
    }

    return OnDamageServerOriginal(BuildingActor, Damage, DamageTags, Momentum, HitInfo, InstigatedBy, DamageCauser, EffectContext);
}

UClass* ABuildingActor::StaticClass()
{
    static auto Class = FindObject<UClass>(L"/Script/FortniteGame.BuildingActor");
    return Class;
}
