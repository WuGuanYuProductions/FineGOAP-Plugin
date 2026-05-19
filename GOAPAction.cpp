// Copyright WuGuanyu Productions, All Rights Reserved.

#include "GOAPAction.h"
#include "GOAPComponent.h"
#include "GOAPModifier.h"
#include "GOAPGlobalSubsystem.h" 
#include "Engine/World.h"

UGOAPAction::UGOAPAction()
{
	BaseCost = 1.0f;
}

float UGOAPAction::CalculateCost_Implementation(UGOAPComponent* Agent)
{
	float FinalCost = BaseCost;

	for (UGOAPModifier* Modifier : CostModifiers)
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

bool UGOAPAction::IsOnCooldown() const
{
	if (CooldownType == EGOAPCooldownType::DirectValue)
	{
		if (Cooldown <= 0.0f) return false;
	}

	if (!GetWorld()) return false;

	return (GetWorld()->GetTimeSeconds() - LastExecutionTime) < CurrentCooldownDuration;
}

void UGOAPAction::StartCooldown(UGOAPComponent* Agent)
{
	if (CooldownType == EGOAPCooldownType::DirectValue)
	{
		CurrentCooldownDuration = Cooldown;
	}
	else if (CooldownModifier)
	{
		CurrentCooldownDuration = CooldownModifier->CalculateModifier(Agent);
	}
	else
	{
		CurrentCooldownDuration = 0.0f;
	}

	if (GetWorld())
	{
		LastExecutionTime = GetWorld()->GetTimeSeconds();
	}
}

bool UGOAPAction::IsActionAvailable(UGOAPComponent* Agent)
{
	if (bDisable) return false;
	if (IsOnCooldown()) return false;

	return CheckProceduralPrecondition(Agent);
}