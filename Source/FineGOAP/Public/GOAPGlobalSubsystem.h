#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GOAPGlobalSubsystem.generated.h"

/**
 * 🌍 GOAP 全局状态子系统 (充当所有 AI 的公共告示牌)
 */
UCLASS()
class FINEGOAP_API UGOAPGlobalSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	// 增加或减少全局状态的值 (用于占位/释放令牌)
	UFUNCTION(BlueprintCallable, Category = "GOAP|Global State", meta = (ToolTip = "Modify a global state. Delta = 1 means claim a token, Delta = -1 means release it."))
	void ModifyGlobalState(FName Key, int32 Delta);

	// 获取当前全局状态的值
	UFUNCTION(BlueprintPure, Category = "GOAP|Global State", meta = (ToolTip = "Get the current integer value of a specific global state."))
	int32 GetGlobalState(FName Key) const;

	// 直接覆盖设置全局状态的值
	UFUNCTION(BlueprintCallable, Category = "GOAP|Global State", meta = (ToolTip = "Directly overwrite the value of a global state."))
	void SetGlobalState(FName Key, int32 Value);

	// ==========================================
	// 🐛 Debug 专用函数
	// ==========================================
	/*
	 * 在屏幕上打印当前的 Global State 状态
	 */
	UFUNCTION(BlueprintCallable, Category = "GOAP|Debug", meta = (ToolTip = "Print current global states on screen. Pass a SpecificKey to monitor only one state, or leave it 'None' to monitor all."))
	void DebugPrintGlobalStates(bool bEnable = true, FName SpecificKey = NAME_None);

private:
	// 存储所有的全局状态
	TMap<FName, int32> GlobalStates;
};