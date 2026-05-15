#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "GOAPTypes.h"
#include "GOAPModifier.h"
#include "GOAPAction.generated.h"

class UGOAPComponent;

// 🟢 新增：策划专用的冷却配置选项
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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GOAP Action")
	FName ActionName;

	// 基础 Cost
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GOAP Action")
	float BaseCost;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GOAP Action")
	TMap<FName, int32> Preconditions;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GOAP Action")
	TMap<FName, int32> Effects;

	// ==========================================
	// 💰 消耗与动态修饰器
	// ==========================================
	// 动态 Cost 修饰器数组（策划可以在面板里直接添加和编辑）
	UPROPERTY(EditAnywhere, Instanced, BlueprintReadWrite, Category = "GOAP|Cost")
	TArray<UGOAPModifier*> CostModifiers;

	// ==========================================
	// ⚡ 打断机制
	// ==========================================
	// 打断阈值：如果当前正在执行本动作，且有其他合法动作的“动态 Cost”低于这个阈值，则本动作会被打断。
	// 设为 -1 表示不允许通过 Cost 机制被打断。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GOAP|Interrupt")
	float InterruptCost = -1.0f;

	// ------------------------------------------
	// 核心逻辑函数
	// ------------------------------------------

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "GOAP Action")
	float CalculateCost(UGOAPComponent* Agent);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "GOAP Action")
	bool CheckProceduralPrecondition(UGOAPComponent* Agent);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "GOAP Action")
	void OnActionStart(UGOAPComponent* Agent);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "GOAP Action")
	bool OnActionTick(UGOAPComponent* Agent, float DeltaTime);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "GOAP Action")
	void OnActionEnd(UGOAPComponent* Agent);

	virtual UWorld* GetWorld() const override;

	UFUNCTION(BlueprintPure, Category = "GOAP|Global State")
	class UGOAPGlobalSubsystem* GetGlobalGOAPSubsystem() const;

	// ==========================================
	// ⏳ 冷却机制 (Cooldown)
	// ==========================================

	// 选择冷却时间的配置方式
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GOAP|Cooldown")
	EGOAPCooldownType CooldownType = EGOAPCooldownType::DirectValue;

	// 方式A：直接填值（仅当选择了 DirectValue 时显示）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GOAP|Cooldown", meta = (EditCondition = "CooldownType == EGOAPCooldownType::DirectValue", EditConditionHides))
	float Cooldown = 0.0f;

	// 方式B：使用修饰器动态计算（仅当选择了 ModifierValue 时显示，Instanced 允许直接在面板内实例化编辑）
	UPROPERTY(EditAnywhere, Instanced, BlueprintReadWrite, Category = "GOAP|Cooldown", meta = (EditCondition = "CooldownType == EGOAPCooldownType::ModifierValue", EditConditionHides))
	UGOAPModifier* CooldownModifier;

	// --- 内部状态缓存 ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "GOAP|Cooldown State")
	float LastExecutionTime = -10000.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "GOAP|Cooldown State")
	float CurrentCooldownDuration = 0.0f;

	// 检查动作是否在冷却中
	UFUNCTION(BlueprintCallable, Category = "GOAP|Cooldown")
	bool IsOnCooldown() const;

	// 开始计算冷却（由 GOAPComponent 在动作结束或开始时调用，需要 Agent 参与修饰器计算）
	UFUNCTION(BlueprintCallable, Category = "GOAP|Cooldown")
	void StartCooldown(UGOAPComponent* Agent);

	// 综合检查：是否满足冷却条件 且 满足前提条件
	UFUNCTION(BlueprintCallable, Category = "GOAP Action")
	bool IsActionAvailable(UGOAPComponent* Agent);

	// 🔴 新增：策划专用的禁用开关
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GOAP Action")
	bool bDisable = false;
};