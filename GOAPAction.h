// Copyright WuGuanyu Productions, All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "GOAPTypes.h"
#include "GOAPModifier.h"
#include "GOAPAction.generated.h"

class UGOAPComponent;

UENUM(BlueprintType)
enum class EGOAPCooldownType : uint8
{
	DirectValue UMETA(DisplayName = "Direct Value"),
	ModifierValue UMETA(DisplayName = "Use Modifier")
};

UCLASS(Blueprintable, BlueprintType, Abstract, EditInlineNew)
class FINEGOAP_API UGOAPAction : public UObject
{
	GENERATED_BODY()

public:
	UGOAPAction();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GOAP Action", meta = (ToolTip = "The unique string/name identifying this action."))
	FName ActionName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GOAP Action", meta = (ToolTip = "Base execution cost of this action. Lower cost actions are preferred by the pathfinder."))
	float BaseCost;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GOAP Action", meta = (ToolTip = "Required conditions for this action to be executed."))
	TArray<FGOAPCondition> Preconditions;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GOAP Action", meta = (ToolTip = "State effects applied after this action successfully completes."))
	TArray<FGOAPEffect> Effects;

	UPROPERTY(EditAnywhere, Instanced, BlueprintReadWrite, Category = "GOAP|Cost", meta = (ToolTip = "Dynamic cost modifiers to dynamically alter the action's cost based on real-time conditions (e.g., HP, Distance)."))
	TArray<UGOAPModifier*> CostModifiers;

	// ================= [新增] 打断控制复选框 =================
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GOAP|Interrupt", meta = (ToolTip = "If checked, this action can be interrupted by new higher priority goals."))
	bool bInterruptable = false;

	// ================= [修改] 通过 EditCondition 绑定显示隐藏 =================
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GOAP|Interrupt", meta = (EditCondition = "bInterruptable", EditConditionHides, ToolTip = "Extra priority cost required to interrupt this action. New Priority must be > (Current Priority + Interrupt Cost)."))
	float InterruptCost = 0.0f;

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GOAP|Cooldown")
	EGOAPCooldownType CooldownType = EGOAPCooldownType::DirectValue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GOAP|Cooldown", meta = (EditCondition = "CooldownType == EGOAPCooldownType::DirectValue", EditConditionHides))
	float Cooldown = 0.0f;

	UPROPERTY(EditAnywhere, Instanced, BlueprintReadWrite, Category = "GOAP|Cooldown", meta = (EditCondition = "CooldownType == EGOAPCooldownType::ModifierValue", EditConditionHides))
	UGOAPModifier* CooldownModifier;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "GOAP|Cooldown State")
	float LastExecutionTime = -10000.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "GOAP|Cooldown State")
	float CurrentCooldownDuration = 0.0f;

	UFUNCTION(BlueprintCallable, Category = "GOAP|Cooldown")
	bool IsOnCooldown() const;

	UFUNCTION(BlueprintCallable, Category = "GOAP|Cooldown")
	void StartCooldown(UGOAPComponent* Agent);

	UFUNCTION(BlueprintCallable, Category = "GOAP Action")
	bool IsActionAvailable(UGOAPComponent* Agent);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GOAP Action")
	bool bDisable = false;
};