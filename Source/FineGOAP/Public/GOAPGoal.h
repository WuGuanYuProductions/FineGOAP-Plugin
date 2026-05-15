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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GOAP Goal")
	FName GoalName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GOAP Goal")
	float BasePriority;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GOAP Goal")
	TMap<FName, int32> DesiredState;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "GOAP Goal")
	float CalculatePriority(UGOAPComponent* Agent);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "GOAP Goal")
	bool IsGoalAchieved(UGOAPComponent* Agent);

	// 🔴 新增：策划专用的禁用开关
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GOAP Goal")
	bool bDisable = false;

	// 🔴 新增：层级化结构，专属当前 Goal 的动作库！
	// (Instanced 允许策划直接在 Goal 的下拉菜单里点加号创建 Action 实例)
	UPROPERTY(EditAnywhere, Instanced, BlueprintReadWrite, Category = "GOAP Goal")
	TArray<class UGOAPAction*> GoalActions;
};