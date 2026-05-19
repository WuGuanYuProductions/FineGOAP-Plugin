// Copyright WuGuanyu Productions, All Rights Reserved.

#include "GOAPGoal.h"
#include "GOAPComponent.h" 
#include "GOAPGlobalSubsystem.h" 
#include "Engine/World.h"       

UGOAPGoal::UGOAPGoal()
{
	BasePriority = 1.0f;
}

float UGOAPGoal::CalculatePriority_Implementation(UGOAPComponent* Agent)
{
	return BasePriority;
}

bool UGOAPGoal::IsGoalAchieved_Implementation(UGOAPComponent* Agent)
{
	if (!Agent) return false;

	for (const FGOAPCondition& Condition : GoalConditions)
	{
		int32 CurrentStateValue = Agent->GetWorldState(Condition.Key);
		if (!Condition.Evaluate(CurrentStateValue))
		{
			return false;
		}
	}
	return true;
}


UWorld* UGOAPGoal::GetWorld() const
{
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		return nullptr;
	}

	if (UObject* Outer = GetOuter())
	{
		return Outer->GetWorld();
	}

	return nullptr;
}