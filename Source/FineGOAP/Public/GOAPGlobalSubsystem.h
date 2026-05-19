// Copyright WuGuanyu Productions, All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GOAPGlobalSubsystem.generated.h"

UCLASS(BlueprintType)
class FINEGOAP_API UGOAPGlobalSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintPure, Category = "GOAP|Global State", meta = (WorldContext = "WorldContextObject", DisplayName = "Get GOAP Global Subsystem"))
	static UGOAPGlobalSubsystem* GetGOAPGlobalSubsystem(const UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category = "GOAP|Global State", meta = (ToolTip = "Modify a global state. Delta = 1 means claim a token, Delta = -1 means release it."))
	void ModifyGlobalState(FName Key, int32 Delta);

	UFUNCTION(BlueprintPure, Category = "GOAP|Global State", meta = (ToolTip = "Get the current integer value of a specific global state."))
	int32 GetGlobalState(FName Key) const;

	UFUNCTION(BlueprintCallable, Category = "GOAP|Global State", meta = (ToolTip = "Directly overwrite the value of a global state."))
	void SetGlobalState(FName Key, int32 Value);

	UFUNCTION(BlueprintCallable, Category = "GOAP|Debug", meta = (ToolTip = "Print current global states on screen. Pass a SpecificKey to monitor only one state, or leave it 'None' to monitor all."))
	void DebugPrintGlobalStates(bool bEnable = true, FName SpecificKey = NAME_None);

private:

	TMap<FName, int32> GlobalStates;
};