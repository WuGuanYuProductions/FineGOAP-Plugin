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
	// Delta 填 1 表示占用，填 -1 表示释放
	UFUNCTION(BlueprintCallable, Category = "GOAP|Global State")
	void ModifyGlobalState(FName Key, int32 Delta);

	// 获取当前全局状态的值
	UFUNCTION(BlueprintPure, Category = "GOAP|Global State")
	int32 GetGlobalState(FName Key) const;

	// 直接覆盖设置全局状态的值
	UFUNCTION(BlueprintCallable, Category = "GOAP|Global State")
	void SetGlobalState(FName Key, int32 Value);

	// ==========================================
	// 🐛 Debug 专用函数
	// ==========================================
	/*
	 * 在屏幕上打印当前的 Global State 状态
	 * @param bEnable - 是否开启打印
	 * @param SpecificKey - 如果填入具体的名称(例如"MeleeAttacking")，则只打印这一项；如果保留为 "None"，则打印所有项
	 */
	UFUNCTION(BlueprintCallable, Category = "GOAP|Debug")
	void DebugPrintGlobalStates(bool bEnable = true, FName SpecificKey = NAME_None);

private:
	// 存储所有的全局状态
	TMap<FName, int32> GlobalStates;
};