#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "GOAPTypes.h"
#include "GOAPModifier.h"
#include "GOAPAction.generated.h"

class UGOAPComponent;

// 🟢 策划专用的冷却配置选项
UENUM(BlueprintType)
enum class EGOAPCooldownType : uint8
{
	DirectValue UMETA(DisplayName = "直接填值 (Direct Value)"),
	ModifierValue UMETA(DisplayName = "使用修饰器 (Use Modifier)")
};

UCLASS(Blueprintable, BlueprintType, Abstract, EditInlineNew)
class FINEGOAP_API UGOAPAction : public UObject
{
	GENERATED_BODY()

public:
	UGOAPAction();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GOAP Action", meta = (ToolTip = "The unique string/name identifying this action."))
	FName ActionName;

	// 基础 Cost
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GOAP Action", meta = (ToolTip = "Base execution cost of this action. Lower cost actions are preferred by the pathfinder."))
	float BaseCost;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GOAP Action", meta = (ToolTip = "Required world states for this action to be executed."))
	TMap<FName, int32> Preconditions;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GOAP Action", meta = (ToolTip = "World states that will be modified after this action successfully completes."))
	TMap<FName, int32> Effects;

	// ==========================================
	// 💰 消耗与动态修饰器
	// ==========================================
	UPROPERTY(EditAnywhere, Instanced, BlueprintReadWrite, Category = "GOAP|Cost", meta = (ToolTip = "Dynamic cost modifiers to dynamically alter the action's cost based on real-time conditions (e.g., HP, Distance)."))
	TArray<UGOAPModifier*> CostModifiers;

	// ==========================================
	// ⚡ 打断机制
	// ==========================================
	// 设为 -1 表示不允许通过 Cost 机制被打断。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GOAP|Interrupt", meta = (ToolTip = "Threshold cost for interruption. If another valid action's dynamic cost is lower than this value, this action will be aborted. Set to -1 to disable interruption."))
	float InterruptCost = -1.0f;

	// ------------------------------------------
	// 核心逻辑函数
	// ------------------------------------------

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "GOAP Action", meta = (ToolTip = "Calculate the final dynamic cost of this action."))
	float CalculateCost(UGOAPComponent* Agent);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "GOAP Action", meta = (ToolTip = "Procedural validation for conditions that cannot be easily represented by integer world states."))
	bool CheckProceduralPrecondition(UGOAPComponent* Agent);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "GOAP Action", meta = (ToolTip = "Called once when the action begins execution."))
	void OnActionStart(UGOAPComponent* Agent);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "GOAP Action", meta = (ToolTip = "Called every frame while the action is running. Return true to indicate the action is finished."))
	bool OnActionTick(UGOAPComponent* Agent, float DeltaTime);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "GOAP Action", meta = (ToolTip = "Called once when the action is finished or aborted."))
	void OnActionEnd(UGOAPComponent* Agent);

	virtual UWorld* GetWorld() const override;

	UFUNCTION(BlueprintPure, Category = "GOAP|Global State", meta = (ToolTip = "Convenience function to fetch the Global GOAP Subsystem."))
	class UGOAPGlobalSubsystem* GetGlobalGOAPSubsystem() const;

	// ==========================================
	// ⏳ 冷却机制 (Cooldown)
	// ==========================================

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GOAP|Cooldown", meta = (ToolTip = "Choose how the cooldown duration is determined: fixed value or calculated via a modifier."))
	EGOAPCooldownType CooldownType = EGOAPCooldownType::DirectValue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GOAP|Cooldown", meta = (EditCondition = "CooldownType == EGOAPCooldownType::DirectValue", EditConditionHides, ToolTip = "Fixed cooldown duration in seconds."))
	float Cooldown = 0.0f;

	UPROPERTY(EditAnywhere, Instanced, BlueprintReadWrite, Category = "GOAP|Cooldown", meta = (EditCondition = "CooldownType == EGOAPCooldownType::ModifierValue", EditConditionHides, ToolTip = "Modifier used to dynamically calculate cooldown duration."))
	UGOAPModifier* CooldownModifier;

	// --- 内部状态缓存 ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "GOAP|Cooldown State", meta = (ToolTip = "The exact time this action was last executed."))
	float LastExecutionTime = -10000.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "GOAP|Cooldown State", meta = (ToolTip = "The active cooldown duration applied to the current execution."))
	float CurrentCooldownDuration = 0.0f;

	UFUNCTION(BlueprintCallable, Category = "GOAP|Cooldown", meta = (ToolTip = "Check if this action is currently on cooldown."))
	bool IsOnCooldown() const;

	UFUNCTION(BlueprintCallable, Category = "GOAP|Cooldown", meta = (ToolTip = "Trigger the cooldown calculation and start the timer."))
	void StartCooldown(UGOAPComponent* Agent);

	UFUNCTION(BlueprintCallable, Category = "GOAP Action", meta = (ToolTip = "Comprehensive check returning whether this action is available (combines Cooldown and Procedural Preconditions)."))
	bool IsActionAvailable(UGOAPComponent* Agent);

	// 🔴 策划专用的禁用开关
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GOAP Action", meta = (ToolTip = "If true, this action will be completely ignored by the GOAP solver."))
	bool bDisable = false;
};