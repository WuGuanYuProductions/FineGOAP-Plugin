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
	// [Bug修复/安全优化] 避免类默认对象(CDO)在蓝图编译期间获取World引发的引擎警告或Crash
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		return nullptr;
	}

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
	if (CurrentCooldownDuration <= 0.0f)
	{
		return false;
	}

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
		LastExecutionTime = World->GetTimeSeconds();

		if (CooldownType == EGOAPCooldownType::DirectValue)
		{
			CurrentCooldownDuration = Cooldown;
		}
		else if (CooldownType == EGOAPCooldownType::ModifierValue && CooldownModifier != nullptr)
		{
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
	if (IsOnCooldown())
	{
		return false;
	}

	return CheckProceduralPrecondition(Agent);
}