// Copyright WuGuanyu Productions, All Rights Reserved.

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
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(BlueprintReadWrite, Category = "GOAP State", meta = (ToolTip = "Current specific world states known by this agent."))
	FGOAPState CurrentWorldState;

	UPROPERTY(EditAnywhere, Instanced, BlueprintReadWrite, Category = "GOAP Setup", meta = (ToolTip = "Global pool of actions this agent can perform. The planner will emergent-ly chain them together."))
	TArray<UGOAPAction*> AvailableActions;

	UPROPERTY(EditAnywhere, Instanced, BlueprintReadWrite, Category = "GOAP Setup", meta = (ToolTip = "Pool of goals this agent will evaluate and try to achieve. Click '+' to add goals."))
	TArray<UGOAPGoal*> AvailableGoals;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "GOAP|State", meta = (ToolTip = "The current active goal the agent is trying to achieve."))
	class UGOAPGoal* CurrentGoal = nullptr;

	UFUNCTION(BlueprintCallable, Category = "GOAP|Debug", meta = (WorldContext = "WorldContextObject"))
	static void ToggleGOAPDebug(const UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category = "GOAP|Debug", meta = (WorldContext = "WorldContextObject"))
	static void SwitchGOAPDebugTarget(const UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category = "GOAP|Debug")
	void DebugPrintWorldState(bool bEnable = true, FName SpecificKey = NAME_None);

	UFUNCTION(BlueprintCallable, Category = "GOAP State")
	void SetWorldState(FName Key, int32 Value);

	UFUNCTION(BlueprintCallable, Category = "GOAP State")
	int32 GetWorldState(FName Key);

	UFUNCTION(BlueprintCallable, Category = "GOAP Planner")
	bool BuildPlan();

	UFUNCTION(BlueprintCallable, Category = "GOAP|Core")
	void AbortCurrentPlan();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "GOAP Helper")
	APawn* GetAIPawn();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "GOAP Helper")
	AAIController* GetAIController();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "GOAP Helper")
	ACharacter* GetPlayer();

	UFUNCTION(BlueprintCallable, Category = "GOAP|Interrupt")
	void CheckForInterruption();

private:
	UPROPERTY()
	TArray<UGOAPAction*> CurrentPlan;

	UPROPERTY()
	UGOAPAction* CurrentAction;

	static bool bGlobalDebugEnabled;
	static TWeakObjectPtr<UGOAPComponent> CurrentDebugTarget;
};