#include "GOAPGoal.h"
#include "GOAPComponent.h"

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

	for (const auto& State : DesiredState)
	{
		if (Agent->GetWorldState(State.Key) != State.Value)
		{
			return false;
		}
	}
	return true;
}