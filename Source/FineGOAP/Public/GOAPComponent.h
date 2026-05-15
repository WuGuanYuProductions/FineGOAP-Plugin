#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GOAPTypes.h"
#include "GOAPAction.h"
#include "GOAPGoal.h"
#include "GOAPComponent.generated.h"

class UGOAPAction;
class UGOAPGoal;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class FINEGOAP_API UGOAPComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGOAPComponent();

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(BlueprintReadWrite, Category = "GOAP State")
	FGOAPState CurrentWorldState;

	// 组件面板配置
	// =========================================
	// 🔴 新增：状态缓存
	// =========================================
	// 缓存当前正在执行的 Goal
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "GOAP|State")
	class UGOAPGoal* CurrentGoal = nullptr;

	// =========================================
	// 🐛 调试系统 (BlueprintCallable 允许策划用快捷键调用)
	// =========================================

	// 按键 1: 开/关 屏幕 Debug 显示
	UFUNCTION(BlueprintCallable, Category = "GOAP|Debug", meta = (WorldContext = "WorldContextObject"))
	static void ToggleGOAPDebug(const UObject* WorldContextObject);

	// 按键 2: 切换观察的 AI 目标
	UFUNCTION(BlueprintCallable, Category = "GOAP|Debug", meta = (WorldContext = "WorldContextObject"))
	static void SwitchGOAPDebugTarget(const UObject* WorldContextObject);

	UPROPERTY(EditAnywhere, Instanced, BlueprintReadWrite, Category = "GOAP Setup")
	TArray<UGOAPGoal*> AvailableGoals;

	UFUNCTION(BlueprintCallable, Category = "GOAP State")
	void SetWorldState(FName Key, int32 Value);

	UFUNCTION(BlueprintCallable, Category = "GOAP State")
	int32 GetWorldState(FName Key);

	UFUNCTION(BlueprintCallable, Category = "GOAP Planner")
	bool BuildPlan();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "GOAP Helper")
	APawn* GetAIPawn();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "GOAP Helper")
	AAIController* GetAIController();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "GOAP Helper")
	ACharacter* GetPlayer();

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// ==========================================
	// 🐛 Debug 专用函数
	// ==========================================
	UFUNCTION(BlueprintCallable, Category = "GOAP|Debug")
	void DebugPrintWorldState(bool bEnable = true, FName SpecificKey = NAME_None);

	// ==========================================
	// ⚡ 计划打断机制
	// ==========================================
	UFUNCTION(BlueprintCallable, Category = "GOAP|Core")
	void AbortCurrentPlan();

private:
	UPROPERTY()
	TArray<UGOAPAction*> CurrentPlan;

	UPROPERTY()
	UGOAPAction* CurrentAction;

	// 静态变量，用于管理全局的调试状态
	static bool bGlobalDebugEnabled;
	static TWeakObjectPtr<UGOAPComponent> CurrentDebugTarget;
};