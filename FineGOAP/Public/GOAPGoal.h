// Copyright WuGuanyu Productions, All Rights Reserved.

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
	TArray<FGOAPCondition> GoalConditions;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "GOAP Goal", meta = (ToolTip = "Override this in Blueprint to dynamically calculate the priority of this goal based on current conditions."))
	float CalculatePriority(UGOAPComponent* Agent);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "GOAP Goal", meta = (ToolTip = "Override this to define custom logic for whether the goal is achieved. By default, it checks the GoalConditions."))
	bool IsGoalAchieved(UGOAPComponent* Agent);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GOAP Goal", meta = (ToolTip = "If true, this goal will be completely ignored by the GOAP solver."))
	bool bDisable = false;

	virtual UWorld* GetWorld() const override;
};