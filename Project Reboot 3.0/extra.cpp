#include "die.h"
#include "gui.h"

void SetZoneToIndexHook(AFortGameModeAthena* GameModeAthena, int OverridePhaseMaybeIDFK)
{
	auto GameState = Cast<AFortGameStateAthena>(GameModeAthena->GetGameState());
	auto SafeZoneIndicator = GameModeAthena->GetSafeZoneIndicator();

	if (!GameState || !SafeZoneIndicator)
	{
		LOG_ERROR(LogZone, "Invalid GameState or SafeZoneIndicator!");
		return SetZoneToIndexOriginal(GameModeAthena, OverridePhaseMaybeIDFK);
	}

	int32 ZoneDurationsOffset = (Fortnite_Version >= 13.40 && Fortnite_Version < 18) ? 0x258
		: (std::floor(Fortnite_Version) >= 18) ? 0x248
		: 0x1F8;
	int32 GameMode_SafeZonePhaseOffset = GameModeAthena->GetOffset("SafeZonePhase");
	int32 GameState_SafeZonePhaseOffset = GameState->GetOffset("SafeZonePhase");
	int32 SafeZonePhaseOffset = GameModeAthena->GetOffset("SafeZonePhase");

	static bool bFilledDurations = false;
	static bool bZoneReversing = false;
	static bool bEnableReverseZone = false;
	static int32 NewLateGameSafeZonePhase = 1;
	static const int32 EndReverseZonePhase = 5;
	static const int32 StartReverseZonePhase = 7;

	int NumPlayers = GameState->GetPlayersLeft();

	if (NumPlayers >= 5)
	{
		if (Fortnite_Version < 8)
		{
			if (Globals::bLateGame.load())
			{
				GameModeAthena->Get<int>(GameMode_SafeZonePhaseOffset) = NewLateGameSafeZonePhase;
				GameState->Get<int>(GameState_SafeZonePhaseOffset) = NewLateGameSafeZonePhase;
				SetZoneToIndexOriginal(GameModeAthena, OverridePhaseMaybeIDFK);

				if (NewLateGameSafeZonePhase == EndReverseZonePhase) bZoneReversing = false;
				if (NewLateGameSafeZonePhase == 1 || NewLateGameSafeZonePhase == 2) SafeZoneIndicator->SkipShrinkSafeZone();
				if (NewLateGameSafeZonePhase >= StartReverseZonePhase) bZoneReversing = false;

				NewLateGameSafeZonePhase = (bZoneReversing && bEnableReverseZone) ? NewLateGameSafeZonePhase - 1 : NewLateGameSafeZonePhase + 1;

				return;
			}
			return SetZoneToIndexOriginal(GameModeAthena, OverridePhaseMaybeIDFK);
		}

		auto SafeZoneFinishShrinkTimeOffset = SafeZoneIndicator->GetOffset("SafeZoneFinishShrinkTime");
		auto SafeZoneStartShrinkTimeOffset = SafeZoneIndicator->GetOffset("SafeZoneStartShrinkTime");
		auto RadiusOffset = SafeZoneIndicator->GetOffset("Radius");
		auto MapInfoOffset = GameState->GetOffset("MapInfo");
		auto MapInfo = GameState->Get<AActor*>(MapInfoOffset);

		if (!MapInfo)
		{
			LOG_ERROR(LogZone, "Invalid MapInfo!");
			return SetZoneToIndexOriginal(GameModeAthena, OverridePhaseMaybeIDFK);
		}

		auto SafeZoneDefinitionOffset = MapInfo->GetOffset("SafeZoneDefinition");
		auto SafeZoneDefinition = MapInfo->GetPtr<__int64>(SafeZoneDefinitionOffset);
		auto ZoneHoldDurationsOffset = ZoneDurationsOffset - 0x10;

		auto& ZoneDurations = *(TArray<float>*)(__int64(SafeZoneDefinition) + ZoneDurationsOffset);
		auto& ZoneHoldDurations = *(TArray<float>*)(__int64(SafeZoneDefinition) + ZoneHoldDurationsOffset);

		if (!bFilledDurations)
		{
			bFilledDurations = true;
			UCurveTable* FortGameData = GameState->GetCurrentPlaylist()
				? GameState->GetCurrentPlaylist()->Get<TSoftObjectPtr<UCurveTable>>(GameState->GetCurrentPlaylist()->GetOffset("GameData")).Get()
				: FindObject<UCurveTable>(L"/Game/Balance/AthenaGameData.AthenaGameData");

			if (!FortGameData)
			{
				LOG_ERROR(LogZone, "Unable to get FortGameData.");
				return SetZoneToIndexOriginal(GameModeAthena, OverridePhaseMaybeIDFK);
			}

			auto ShrinkTimeFName = UKismetStringLibrary::Conv_StringToName(L"Default.SafeZone.ShrinkTime");
			auto HoldTimeFName = UKismetStringLibrary::Conv_StringToName(L"Default.SafeZone.WaitTime");

			for (int i = 0; i < ZoneDurations.Num(); i++)
				ZoneDurations.at(i) = FortGameData->GetValueOfKey(FortGameData->GetKey(ShrinkTimeFName, i));

			for (int i = 0; i < ZoneHoldDurations.Num(); i++)
				ZoneHoldDurations.at(i) = FortGameData->GetValueOfKey(FortGameData->GetKey(HoldTimeFName, i));
		}

		LOG_INFO(LogZone, "SafeZonePhase: {}", GameModeAthena->Get<int>(SafeZonePhaseOffset));
		LOG_INFO(LogZone, "OverridePhaseMaybeIDFK: {}", OverridePhaseMaybeIDFK);
		LOG_INFO(LogZone, "TimeSeconds: {}", UGameplayStatics::GetTimeSeconds(GetWorld()));

		if (Globals::bLateGame.load())
		{
			GameModeAthena->Get<int>(GameMode_SafeZonePhaseOffset) = NewLateGameSafeZonePhase;
			GameState->Get<int>(GameState_SafeZonePhaseOffset) = NewLateGameSafeZonePhase;
			SetZoneToIndexOriginal(GameModeAthena, OverridePhaseMaybeIDFK);

			if (NewLateGameSafeZonePhase == EndReverseZonePhase) bZoneReversing = false;
			if (NewLateGameSafeZonePhase >= StartReverseZonePhase) bZoneReversing = false;

			NewLateGameSafeZonePhase = (bZoneReversing && bEnableReverseZone) ? NewLateGameSafeZonePhase - 1 : NewLateGameSafeZonePhase + 1;
		}
		else
		{
			SetZoneToIndexOriginal(GameModeAthena, OverridePhaseMaybeIDFK);
		}

		LOG_INFO(LogZone, "SafeZonePhase After: {}", GameModeAthena->Get<int>(SafeZonePhaseOffset));

		float ZoneHoldDuration = (GameModeAthena->Get<int>(SafeZonePhaseOffset) >= 0 && GameModeAthena->Get<int>(SafeZonePhaseOffset) < ZoneHoldDurations.Num())
			? ZoneHoldDurations.at(GameModeAthena->Get<int>(SafeZonePhaseOffset)) : 30.0f;

		SafeZoneIndicator->Get<float>(SafeZoneStartShrinkTimeOffset) = GameState->GetServerWorldTimeSeconds() + ZoneHoldDuration;

		float ZoneDuration = (GameModeAthena->Get<int>(SafeZonePhaseOffset) >= 0 && GameModeAthena->Get<int>(SafeZonePhaseOffset) < ZoneDurations.Num())
			? ZoneDurations.at(GameModeAthena->Get<int>(SafeZonePhaseOffset)) : 0.0f;

		LOG_INFO(LogZone, "ZoneDuration: {}", ZoneDuration);
		LOG_INFO(LogZone, "Current Radius: {}", SafeZoneIndicator->Get<float>(RadiusOffset));

		SafeZoneIndicator->Get<float>(SafeZoneFinishShrinkTimeOffset) = SafeZoneIndicator->Get<float>(SafeZoneStartShrinkTimeOffset) + ZoneDuration;

		if (GameModeAthena->Get<int>(SafeZonePhaseOffset) == 0)
		{
			const float FixedInitialZoneSize = 5000.0f;
			SafeZoneIndicator->Get<float>(RadiusOffset) = FixedInitialZoneSize;
			LOG_INFO(LogZone, "Initial Storm Zone Size set to: {}", FixedInitialZoneSize);
		}

		if (GameModeAthena->Get<int>(SafeZonePhaseOffset) == 2 || GameModeAthena->Get<int>(SafeZonePhaseOffset) == 3)
		{
			if (SafeZoneIndicator)
			{
				SafeZoneIndicator->SkipShrinkSafeZone();
			}
			else
			{
				LOG_WARN(LogZone, "SafeZoneIndicator is null during skip.");
			}

			UKismetSystemLibrary::ExecuteConsoleCommand(GetWorld(), L"skipsafezone", nullptr);
		}
	}

	if (NumPlayers < 5)
	{
		static int NewLateGameSafeZonePhase = 2;

		LOG_INFO(LogDev, "NewLateGameSafeZonePhase: {}", NewLateGameSafeZonePhase);

		if (Fortnite_Version < 8)
		{
			if (Globals::bLateGame.load())
			{
				GameModeAthena->Get<int>(GameMode_SafeZonePhaseOffset) = NewLateGameSafeZonePhase;
				GameState->Get<int>(GameState_SafeZonePhaseOffset) = NewLateGameSafeZonePhase;
				SetZoneToIndexOriginal(GameModeAthena, OverridePhaseMaybeIDFK);

				if (NewLateGameSafeZonePhase == EndReverseZonePhase)
				{
					bZoneReversing = false;
				}

				if (NewLateGameSafeZonePhase == 2 || NewLateGameSafeZonePhase == 3)
				{
					if (SafeZoneIndicator)
						SafeZoneIndicator->SkipShrinkSafeZone();
					else
						LOG_WARN(LogZone, "Invalid SafeZoneIndicator!");
				}

				if (NewLateGameSafeZonePhase >= StartReverseZonePhase) // This means instead of going to the 8th phase its gonna go down.
				{
					bZoneReversing = true;
				}

				if (bZoneReversing && bEnableReverseZone) NewLateGameSafeZonePhase--;
				else NewLateGameSafeZonePhase++;

				return;
			}

			return SetZoneToIndexOriginal(GameModeAthena, OverridePhaseMaybeIDFK);
		}

		if (!SafeZoneIndicator)
		{
			LOG_WARN(LogZone, "Invalid SafeZoneIndicator!");
			return SetZoneToIndexOriginal(GameModeAthena, OverridePhaseMaybeIDFK);
		}

		static auto SafeZoneFinishShrinkTimeOffset = SafeZoneIndicator->GetOffset("SafeZoneFinishShrinkTime");
		static auto SafeZoneStartShrinkTimeOffset = SafeZoneIndicator->GetOffset("SafeZoneStartShrinkTime");
		static auto RadiusOffset = SafeZoneIndicator->GetOffset("Radius");

		static auto SafeZonePhaseOffset = GameModeAthena->GetOffset("SafeZonePhase");

		static auto MapInfoOffset = GameState->GetOffset("MapInfo");
		auto MapInfo = GameState->Get<AActor*>(MapInfoOffset);

		if (!MapInfo)
		{
			LOG_WARN(LogZone, "Invalid MapInfo!")
				return SetZoneToIndexOriginal(GameModeAthena, OverridePhaseMaybeIDFK);
		}

		static auto SafeZoneDefinitionOffset = MapInfo->GetOffset("SafeZoneDefinition");
		auto SafeZoneDefinition = MapInfo->GetPtr<__int64>(SafeZoneDefinitionOffset);

		LOG_INFO(LogDev, "SafeZoneDefinitionOffset: 0x{:x}", SafeZoneDefinitionOffset);

		static auto ZoneHoldDurationsOffset = ZoneDurationsOffset - 0x10;

		auto& ZoneDurations = *(TArray<float>*)(__int64(SafeZoneDefinition) + ZoneDurationsOffset);
		auto& ZoneHoldDurations = *(TArray<float>*)(__int64(SafeZoneDefinition) + ZoneHoldDurationsOffset);

		static bool bFilledDurations = false;

		if (!bFilledDurations)
		{
			bFilledDurations = true;

			auto CurrentPlaylist = GameState->GetCurrentPlaylist();
			UCurveTable* FortGameData = nullptr;

			static auto GameDataOffset = CurrentPlaylist->GetOffset("GameData");
			FortGameData = CurrentPlaylist ? CurrentPlaylist->Get<TSoftObjectPtr<UCurveTable>>(GameDataOffset).Get() : nullptr;

			if (!FortGameData)
				FortGameData = FindObject<UCurveTable>(L"/Game/Balance/AthenaGameData.AthenaGameData");

			auto ShrinkTimeFName = UKismetStringLibrary::Conv_StringToName(L"Default.SafeZone.ShrinkTime");
			auto HoldTimeFName = UKismetStringLibrary::Conv_StringToName(L"Default.SafeZone.WaitTime");

			for (int i = 0; i < ZoneDurations.Num(); i++)
			{
				ZoneDurations.at(i) = FortGameData->GetValueOfKey(FortGameData->GetKey(ShrinkTimeFName, i));
			}
			for (int i = 0; i < ZoneHoldDurations.Num(); i++)
			{
				ZoneHoldDurations.at(i) = FortGameData->GetValueOfKey(FortGameData->GetKey(HoldTimeFName, i));
			}
		}

		LOG_INFO(LogZone, "SafeZonePhase: {}", GameModeAthena->Get<int>(SafeZonePhaseOffset));
		LOG_INFO(LogZone, "OverridePhaseMaybeIDFK: {}", OverridePhaseMaybeIDFK);
		LOG_INFO(LogZone, "TimeSeconds: {}", UGameplayStatics::GetTimeSeconds(GetWorld()));

		if (Globals::bLateGame.load())
		{
			GameModeAthena->Get<int>(GameMode_SafeZonePhaseOffset) = NewLateGameSafeZonePhase;
			GameState->Get<int>(GameState_SafeZonePhaseOffset) = NewLateGameSafeZonePhase;
			SetZoneToIndexOriginal(GameModeAthena, OverridePhaseMaybeIDFK);

			if (NewLateGameSafeZonePhase == EndReverseZonePhase)
			{
				bZoneReversing = false;
			}

			if (NewLateGameSafeZonePhase >= StartReverseZonePhase)
			{
				bZoneReversing = true;
			}

			if (bZoneReversing && bEnableReverseZone) NewLateGameSafeZonePhase--;
			else NewLateGameSafeZonePhase++;
		}
		else
		{
			SetZoneToIndexOriginal(GameModeAthena, OverridePhaseMaybeIDFK);
		}

		LOG_INFO(LogZone, "SafeZonePhase After: {}", GameModeAthena->Get<int>(SafeZonePhaseOffset));

		float ZoneHoldDuration = 0;

		if (GameModeAthena->Get<int>(SafeZonePhaseOffset) >= 0 && GameModeAthena->Get<int>(SafeZonePhaseOffset) < ZoneHoldDurations.Num())
			ZoneHoldDuration = ZoneHoldDurations.at(GameModeAthena->Get<int>(SafeZonePhaseOffset));

		SafeZoneIndicator->Get<float>(SafeZoneStartShrinkTimeOffset) = GameState->GetServerWorldTimeSeconds() + ZoneHoldDuration;

		float ZoneDuration = 0;

		if (GameModeAthena->Get<int>(SafeZonePhaseOffset) >= 0 && GameModeAthena->Get<int>(SafeZonePhaseOffset) < ZoneDurations.Num())
			ZoneDuration = ZoneDurations.at(GameModeAthena->Get<int>(SafeZonePhaseOffset));

		LOG_INFO(LogZone, "ZoneDuration: {}", ZoneDuration);
		LOG_INFO(LogZone, "Duration: {}", SafeZoneIndicator->Get<float>(RadiusOffset));

		SafeZoneIndicator->Get<float>(SafeZoneFinishShrinkTimeOffset) = SafeZoneIndicator->Get<float>(SafeZoneStartShrinkTimeOffset) + ZoneDuration;

		if (NewLateGameSafeZonePhase == 3 || NewLateGameSafeZonePhase == 4)
		{
			if (SafeZoneIndicator)
				SafeZoneIndicator->SkipShrinkSafeZone();
			else
				LOG_WARN(LogZone, "Invalid SafeZoneIndicator!");
		}
	}
}
