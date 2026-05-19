// Copyright WuGuanyu Productions, All Rights Reserved.

#include "GOAPGlobalSubsystem.h"
#include "Engine/Engine.h" 

UGOAPGlobalSubsystem* UGOAPGlobalSubsystem::GetGOAPGlobalSubsystem(const UObject* WorldContextObject)
{
	if (!WorldContextObject) return nullptr;

	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	if (World)
	{
		return World->GetSubsystem<UGOAPGlobalSubsystem>();
	}

	return nullptr;
}

void UGOAPGlobalSubsystem::ModifyGlobalState(FName Key, int32 Delta)
{
	int32& Value = GlobalStates.FindOrAdd(Key);
	Value += Delta;
}

int32 UGOAPGlobalSubsystem::GetGlobalState(FName Key) const
{
	if (const int32* Ptr = GlobalStates.Find(Key))
	{
		return *Ptr;
	}
	return 0;
}

void UGOAPGlobalSubsystem::SetGlobalState(FName Key, int32 Value)
{
	GlobalStates.Add(Key, Value);
}

void UGOAPGlobalSubsystem::DebugPrintGlobalStates(bool bEnable, FName SpecificKey)
{
	if (!bEnable || !GEngine) return;

	FString DebugMsg = TEXT("=== 🌍 GOAP Global States ===\n");

	if (SpecificKey != NAME_None)
	{
		if (const int32* Val = GlobalStates.Find(SpecificKey))
		{
			DebugMsg.Appendf(TEXT("%s : %d\n"), *SpecificKey.ToString(), *Val);
		}
		else
		{
			DebugMsg.Appendf(TEXT("%s : [No Data/Unregistered]\n"), *SpecificKey.ToString());
		}
	}
	else
	{
		if (GlobalStates.IsEmpty())
		{
			DebugMsg += TEXT(" (No global state data currently)\n");
		}
		else
		{
			for (const TPair<FName, int32>& Pair : GlobalStates)
			{
				DebugMsg.Appendf(TEXT("%s : %d\n"), *Pair.Key.ToString(), Pair.Value);
			}
		}
	}

	GEngine->AddOnScreenDebugMessage(1001, 0.0f, FColor::Cyan, DebugMsg);
}