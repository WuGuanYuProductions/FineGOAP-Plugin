#include "GOAPGoal.h"
#include "GOAPComponent.h" // 假定你工程中存在此文件

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

	// [规范优化] 使用 TPair 明确类型，符合UE最佳实践，避免 auto 潜在的迭代器推导性能损耗
	for (const TPair<FName, int32>& State : DesiredState)
	{
		if (Agent->GetWorldState(State.Key) != State.Value)
		{
			return false;
		}
	}
	return true;
}