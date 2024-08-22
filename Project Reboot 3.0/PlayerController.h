#pragma once

#include "Class.h"
#include "Controller.h"
#include "CheatManager.h"

#include "UnrealString.h"
#include "Rotator.h"

class APlayerController : public AController
{
public:
	UCheatManager*& GetCheatManager()
	{
		static auto CheatManagerOffset = this->GetOffset("CheatManager");
		return this->Get<UCheatManager*>(CheatManagerOffset);
	}

	class UNetConnection*& GetNetConnection()
	{
		static auto NetConnectionOffset = GetOffset("NetConnection");
		return Get<class UNetConnection*>(NetConnectionOffset);
	}

	void SetPlayerIsWaiting(bool NewValue);
	bool IsPlayerWaiting();
	void ServerChangeName(const FString& S);
	void ClientReturnToMainMenu(FString ReturnReason);
	UCheatManager*& SpawnCheatManager(UClass* CheatManagerClass);
	FRotator GetControlRotation();
	void ServerRestartPlayer();
	void ClientReturnToMainMenu(struct FString KickReason);

	static UClass* StaticClass();
};