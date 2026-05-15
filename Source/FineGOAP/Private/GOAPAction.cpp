#include "GOAPAction.h"
#include "GOAPComponent.h" 
#include "GOAPGlobalSubsystem.h" 

UGOAPAction::UGOAPAction()
{
	BaseCost = 1.0f;
}

float UGOAPAction::CalculateCost_Implementation(UGOAPComponent* Agent)
{
	float FinalCost = BaseCost;

	for (const UGOAPModifier* Modifier : CostModifiers)
	{
		if (Modifier)
		{
			FinalCost += Modifier->CalculateModifier(Agent);
		}
	}

	return FMath::Max(0.0f, FinalCost);
}

bool UGOAPAction::CheckProceduralPrecondition_Implementation(UGOAPComponent* Agent)
{
	return true;
}

void UGOAPAction::OnActionStart_Implementation(UGOAPComponent* Agent)
{
}

bool UGOAPAction::OnActionTick_Implementation(UGOAPComponent* Agent, float DeltaTime)
{
	return true;
}

void UGOAPAction::OnActionEnd_Implementation(UGOAPComponent* Agent)
{
}

UWorld* UGOAPAction::GetWorld() const
{
	if (UObject* MyOuter = GetOuter())
	{
		return MyOuter->GetWorld();
	}
	return nullptr;
}

UGOAPGlobalSubsystem* UGOAPAction::GetGlobalGOAPSubsystem() const
{
	if (UWorld* World = GetWorld())
	{
		return World->GetSubsystem<UGOAPGlobalSubsystem>();
	}
	return nullptr;
}

bool UGOAPAction::IsOnCooldown() const
{
	// 如果本次缓存的冷却时间 <= 0，说明没有冷却
	if (CurrentCooldownDuration <= 0.0f)
	{
		return false;
	}

	// 判断当前时间与上次执行时间的差值
	if (UWorld* World = GetWorld())
	{
		return (World->GetTimeSeconds() - LastExecutionTime) < CurrentCooldownDuration;
	}

	return false;
}

void UGOAPAction::StartCooldown(UGOAPComponent* Agent)
{
	if (UWorld* World = GetWorld())
	{
		// 记录冷却起始时间
		LastExecutionTime = World->GetTimeSeconds();

		// 根据策划的配置方式，决定本次冷却时长的来源
		if (CooldownType == EGOAPCooldownType::DirectValue)
		{
			CurrentCooldownDuration = Cooldown;
		}
		else if (CooldownType == EGOAPCooldownType::ModifierValue && CooldownModifier != nullptr)
		{
			// 传入 Agent，调用修饰器动态计算
			CurrentCooldownDuration = CooldownModifier->CalculateModifier(Agent);
		}
		else
		{
			CurrentCooldownDuration = 0.0f;
		}
	}
}

bool UGOAPAction::IsActionAvailable(UGOAPComponent* Agent)
{
	// 1. 核心阻断：如果该技能还在冷却中，直接返回不可用
	if (IsOnCooldown())
	{
		return false;
	}

	// 2. 如果不在冷却中，继续执行原本的蓝图条件判断
	return CheckProceduralPrecondition(Agent);
}