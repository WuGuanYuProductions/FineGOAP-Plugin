#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "GOAPTypes.h"
#include "GOAPGoal.generated.h"

class UGOAPComponent;

UCLASS(Blueprintable, BlueprintType, Abstract, EditInlineNew)
class FINEGOAP_API UGOAPGoal : public UObject
{
	GENERATED_BODY()

public:
	UGOAPGoal();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GOAP Goal", meta = (ToolTip = "The unique identifier name for this goal."))
	FName GoalName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GOAP Goal", meta = (ToolTip = "Base priority of this goal. Higher value means higher priority to be executed."))
	float BasePriority;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GOAP Goal", meta = (ToolTip = "The target world states required to consider this goal successfully achieved."))
	TMap<FName, int32> DesiredState;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "GOAP Goal", meta = (ToolTip = "Override this in Blueprint to dynamically calculate the priority of this goal based on current conditions."))
	float CalculatePriority(UGOAPComponent* Agent);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "GOAP Goal", meta = (ToolTip = "Override this to define custom logic for whether the goal is achieved. By default, it checks the DesiredState."))
	bool IsGoalAchieved(UGOAPComponent* Agent);

	// 🔴 策划专用的禁用开关
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GOAP Goal", meta = (ToolTip = "If true, this goal will be completely ignored by the GOAP solver."))
	bool bDisable = false;

	// 🔴 层级化结构，专属当前 Goal 的动作库！
	UPROPERTY(EditAnywhere, Instanced, BlueprintReadWrite, Category = "GOAP Goal", meta = (ToolTip = "Exclusive actions available only when attempting to achieve this specific goal. Click '+' to add action instances."))
	TArray<class UGOAPAction*> GoalActions;
};