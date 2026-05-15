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
	// 🔴 核心Bug修复点：接管生命周期终点，防止动作意外中断导致的令牌死锁
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(BlueprintReadWrite, Category = "GOAP State", meta = (ToolTip = "Current specific world states known by this agent."))
	FGOAPState CurrentWorldState;

	// =========================================
	// 🔴 状态缓存
	// =========================================
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "GOAP|State", meta = (ToolTip = "The current active goal the agent is trying to achieve."))
	class UGOAPGoal* CurrentGoal = nullptr;

	UPROPERTY(EditAnywhere, Instanced, BlueprintReadWrite, Category = "GOAP Setup", meta = (ToolTip = "Pool of goals this agent will evaluate and try to achieve. Click '+' to add goals."))
	TArray<UGOAPGoal*> AvailableGoals;

	// =========================================
	// 🐛 调试系统
	// =========================================
	UFUNCTION(BlueprintCallable, Category = "GOAP|Debug", meta = (WorldContext = "WorldContextObject", ToolTip = "Toggle on-screen debug printing for GOAP processes globally."))
	static void ToggleGOAPDebug(const UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category = "GOAP|Debug", meta = (WorldContext = "WorldContextObject", ToolTip = "Cycle through available GOAP agents in the world for debugging."))
	static void SwitchGOAPDebugTarget(const UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category = "GOAP|Debug", meta = (ToolTip = "Print local world states to screen for this specific agent. Pass 'None' to print all states."))
	void DebugPrintWorldState(bool bEnable = true, FName SpecificKey = NAME_None);

	// =========================================
	// 🧠 核心逻辑方法
	// =========================================
	UFUNCTION(BlueprintCallable, Category = "GOAP State", meta = (ToolTip = "Set or update an integer world state for this agent."))
	void SetWorldState(FName Key, int32 Value);

	UFUNCTION(BlueprintCallable, Category = "GOAP State", meta = (ToolTip = "Retrieve the value of a specific world state. Returns 0 if not found."))
	int32 GetWorldState(FName Key);

	UFUNCTION(BlueprintCallable, Category = "GOAP Planner", meta = (ToolTip = "Trigger the A* Pathfinding algorithm to build a new sequence of actions based on goals."))
	bool BuildPlan();

	UFUNCTION(BlueprintCallable, Category = "GOAP|Core", meta = (ToolTip = "Force cancel the currently executing action and clear the queued plan. This safely triggers OnActionEnd."))
	void AbortCurrentPlan();

	// =========================================
	// 🛠️ 辅助获取器
	// =========================================
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "GOAP Helper", meta = (ToolTip = "Get the Pawn owning this GOAP component."))
	APawn* GetAIPawn();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "GOAP Helper", meta = (ToolTip = "Get the AI Controller managing this Pawn."))
	AAIController* GetAIController();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "GOAP Helper", meta = (ToolTip = "Get the player character at index 0."))
	ACharacter* GetPlayer();

private:
	UPROPERTY()
	TArray<UGOAPAction*> CurrentPlan;

	UPROPERTY()
	UGOAPAction* CurrentAction;

	static bool bGlobalDebugEnabled;
	static TWeakObjectPtr<UGOAPComponent> CurrentDebugTarget;
};