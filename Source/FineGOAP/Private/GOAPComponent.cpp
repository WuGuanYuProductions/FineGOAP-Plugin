// Copyright WuGuanyu Productions, All Rights Reserved.

#include "GOAPComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "AIController.h"
#include "Kismet/GameplayStatics.h" 
#include "GameFramework/Character.h"
#include "UObject/UObjectIterator.h"
#include "Algo/Reverse.h" 

bool UGOAPComponent::bGlobalDebugEnabled = false;
TWeakObjectPtr<UGOAPComponent> UGOAPComponent::CurrentDebugTarget = nullptr;

struct FGOAPNode
{
	FGOAPState State;
	UGOAPAction* Action;
	TSharedPtr<FGOAPNode> Parent;
	float G;
	float H;
	int32 Depth;

	FGOAPNode() : Action(nullptr), Parent(nullptr), G(0.f), H(0.f), Depth(0) {}
	float F() const { return G + H; }
};

static bool AreStatesEqual(const FGOAPState& A, const FGOAPState& B)
{
	if (A.States.Num() != B.States.Num()) return false;
	for (const TPair<FName, int32>& Pair : A.States)
	{
		const int32* BVal = B.States.Find(Pair.Key);
		if (!BVal || *BVal != Pair.Value) return false;
	}
	return true;
}

static float CalculateHeuristic(const FGOAPState& CurrentState, const TArray<FGOAPCondition>& GoalConditions)
{
	float H = 0.0f;
	for (const FGOAPCondition& Cond : GoalConditions)
	{
		if (!Cond.Evaluate(CurrentState.GetState(Cond.Key)))
		{
			H += 1.0f;
		}
	}
	return H;
}

UGOAPComponent::UGOAPComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	CurrentAction = nullptr;
	CurrentGoal = nullptr;
}

void UGOAPComponent::BeginPlay()
{
	Super::BeginPlay();
	if (!CurrentDebugTarget.IsValid())
	{
		CurrentDebugTarget = this;
	}
}

void UGOAPComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (CurrentAction != nullptr)
	{
		CurrentAction->OnActionEnd(this);
		CurrentAction = nullptr;
	}
	if (CurrentDebugTarget.Get() == this) { CurrentDebugTarget = nullptr; }
	Super::EndPlay(EndPlayReason);
}

APawn* UGOAPComponent::GetAIPawn()
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor) return nullptr;

	if (APawn* OwnerAsPawn = Cast<APawn>(OwnerActor))
	{
		return OwnerAsPawn;
	}

	if (AAIController* OwnerAsController = Cast<AAIController>(OwnerActor))
	{
		return OwnerAsController->GetPawn();
	}

	return nullptr;
}

AAIController* UGOAPComponent::GetAIController()
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor) return nullptr;

	if (AAIController* OwnerAsController = Cast<AAIController>(OwnerActor))
	{
		return OwnerAsController;
	}

	if (APawn* OwnerAsPawn = Cast<APawn>(OwnerActor))
	{
		return Cast<AAIController>(OwnerAsPawn->GetController());
	}

	return nullptr;
}


ACharacter* UGOAPComponent::GetPlayer()
{
	if (UWorld* World = GetWorld())
	{
		return UGameplayStatics::GetPlayerCharacter(World, 0);
	}
	return nullptr;
}


void UGOAPComponent::SetWorldState(FName Key, int32 Value) { CurrentWorldState.SetState(Key, Value); }
int32 UGOAPComponent::GetWorldState(FName Key) { return CurrentWorldState.GetState(Key, 0); }

bool UGOAPComponent::BuildPlan()
{
	CurrentPlan.Reset();
	CurrentGoal = nullptr;
	UGOAPGoal* BestGoal = nullptr;
	float MaxPriority = -1.f;

	for (UGOAPGoal* Goal : AvailableGoals)
	{
		if (!Goal || Goal->bDisable) continue;
		if (!Goal->IsGoalAchieved(this))
		{
			bool bAlreadyMet = true;
			for (const FGOAPCondition& Cond : Goal->GoalConditions)
			{
				if (!Cond.Evaluate(CurrentWorldState.GetState(Cond.Key)))
				{
					bAlreadyMet = false;
					break;
				}
			}
			if (bAlreadyMet) continue;

			float Prio = Goal->CalculatePriority(this);
			if (Prio > MaxPriority)
			{
				MaxPriority = Prio;
				BestGoal = Goal;
			}
		}
	}

	if (!BestGoal) return false;
	CurrentGoal = BestGoal;

	TArray<TSharedPtr<FGOAPNode>> OpenList;
	TArray<FGOAPState> ClosedList;

	TSharedPtr<FGOAPNode> StartNode = MakeShared<FGOAPNode>();
	StartNode->State = CurrentWorldState;
	StartNode->G = 0;
	StartNode->H = CalculateHeuristic(StartNode->State, BestGoal->GoalConditions);
	OpenList.Add(StartNode);

	TSharedPtr<FGOAPNode> GoalNode = nullptr;
	const int32 MaxDepth = 15;
	const int32 MaxIterations = 1000;
	int32 IterationCount = 0;

	while (OpenList.Num() > 0 && IterationCount < MaxIterations)
	{
		IterationCount++;
		int32 BestNodeIndex = 0;
		for (int32 i = 1; i < OpenList.Num(); ++i)
		{
			if (OpenList[i]->F() < OpenList[BestNodeIndex]->F())
			{
				BestNodeIndex = i;
			}
		}

		TSharedPtr<FGOAPNode> CurrentNode = OpenList[BestNodeIndex];
		OpenList.RemoveAt(BestNodeIndex);
		ClosedList.Add(CurrentNode->State);

		bool bGoalMet = true;
		for (const FGOAPCondition& Cond : BestGoal->GoalConditions)
		{
			if (!Cond.Evaluate(CurrentNode->State.GetState(Cond.Key)))
			{
				bGoalMet = false;
				break;
			}
		}

		if (bGoalMet)
		{
			GoalNode = CurrentNode;
			break;
		}

		if (CurrentNode->Depth >= MaxDepth) continue;

		for (UGOAPAction* Action : AvailableActions)
		{
			if (!Action || Action->bDisable) continue;

			bool bPreconditionsMet = true;
			for (const FGOAPCondition& Precond : Action->Preconditions)
			{
				if (!Precond.Evaluate(CurrentNode->State.GetState(Precond.Key)))
				{
					bPreconditionsMet = false;
					break;
				}
			}

			if (bPreconditionsMet && !Action->bDisable && !Action->IsOnCooldown())
			{
				FGOAPState NewState = CurrentNode->State;
				for (const FGOAPEffect& Effect : Action->Effects)
				{
					Effect.Apply(NewState);
				}

				bool bInClosedList = false;
				for (const FGOAPState& VisitedState : ClosedList)
				{
					if (AreStatesEqual(VisitedState, NewState))
					{
						bInClosedList = true;
						break;
					}
				}
				if (bInClosedList) continue;

				TSharedPtr<FGOAPNode> NewNode = MakeShared<FGOAPNode>();
				NewNode->Parent = CurrentNode;
				NewNode->Action = Action;
				NewNode->Depth = CurrentNode->Depth + 1;
				NewNode->G = CurrentNode->G + Action->CalculateCost(this);
				NewNode->H = CalculateHeuristic(NewState, BestGoal->GoalConditions);
				NewNode->State = NewState;

				OpenList.Add(NewNode);
			}
		}
	}

	if (GoalNode != nullptr)
	{
		TSharedPtr<FGOAPNode> TraceNode = GoalNode;
		while (TraceNode != nullptr && TraceNode->Action != nullptr)
		{
			CurrentPlan.Add(TraceNode->Action);
			TraceNode = TraceNode->Parent;
		}

		if (CurrentPlan.Num() > 0)
		{
			Algo::Reverse(CurrentPlan);
			return true;
		}
	}

	CurrentGoal = nullptr;
	return false;
}

void UGOAPComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (CurrentAction == nullptr && CurrentPlan.IsEmpty())
	{
		BuildPlan();
	}

	if (CurrentAction == nullptr && !CurrentPlan.IsEmpty())
	{
		CurrentAction = CurrentPlan[0];
		CurrentPlan.RemoveAt(0);

		if (CurrentAction != nullptr)
		{
			bool bPreconditionsMet = true;
			for (const FGOAPCondition& Precond : CurrentAction->Preconditions)
			{
				if (!Precond.Evaluate(CurrentWorldState.GetState(Precond.Key)))
				{
					bPreconditionsMet = false;
					break;
				}
			}

			if (bPreconditionsMet && CurrentAction->IsActionAvailable(this))
			{
				CurrentAction->OnActionStart(this);
			}
			else
			{
				AbortCurrentPlan();
				return;
			}
		}
	}

	if (CurrentAction != nullptr)
	{
		bool bIsFinished = CurrentAction->OnActionTick(this, DeltaTime);
		if (bIsFinished)
		{
			CurrentAction->StartCooldown(this);
			for (const FGOAPEffect& Effect : CurrentAction->Effects)
			{
				Effect.Apply(CurrentWorldState);
			}
			CurrentAction->OnActionEnd(this);
			CurrentAction = nullptr;
		}
	}

	if (bGlobalDebugEnabled && CurrentDebugTarget.Get() == this && GEngine && GetOwner())
	{
		FString DebugMsg = FString::Printf(TEXT("=== GOAP DEBUG [%s] ==="), *GetOwner()->GetName());

		FString GoalStr = CurrentGoal ? CurrentGoal->GoalName.ToString() : TEXT("None");
		DebugMsg += FString::Printf(TEXT("\n[Current Goal]: %s"), *GoalStr);

		FString ActionStr = CurrentAction ? CurrentAction->ActionName.ToString() : TEXT("None");
		DebugMsg += FString::Printf(TEXT("\n[Current Action]: %s"), *ActionStr);

		FString PlanStr = TEXT("");
		for (UGOAPAction* PlanAction : CurrentPlan)
		{
			if (PlanAction)
			{
				PlanStr += PlanAction->ActionName.ToString() + TEXT(" -> ");
			}
		}
		DebugMsg += FString::Printf(TEXT("\n[Plan Queue]: %s"), PlanStr.IsEmpty() ? TEXT("Empty") : *PlanStr);

		DebugMsg += TEXT("\n[World States]:");
		if (CurrentWorldState.States.Num() == 0)
		{
			DebugMsg += TEXT(" Empty");
		}
		else
		{
			for (const TPair<FName, int32>& Pair : CurrentWorldState.States)
			{
				DebugMsg += FString::Printf(TEXT("\n - %s : %d"), *Pair.Key.ToString(), Pair.Value);
			}
		}

		GEngine->AddOnScreenDebugMessage((uint64)GetOwner()->GetUniqueID(), 0.0f, FColor::Cyan, DebugMsg);
	}
}


void UGOAPComponent::AbortCurrentPlan()
{
	if (CurrentAction != nullptr)
	{
		CurrentAction->OnActionEnd(this);
		CurrentAction = nullptr;
	}
	CurrentPlan.Empty();
	CurrentGoal = nullptr;
}

void UGOAPComponent::ToggleGOAPDebug(const UObject* WorldContextObject)
{
	static uint64 LastToggleFrame = 0;
	if (GFrameCounter == LastToggleFrame) return;
	LastToggleFrame = GFrameCounter;

	bGlobalDebugEnabled = !bGlobalDebugEnabled;
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.f, bGlobalDebugEnabled ? FColor::Green : FColor::Red, FString::Printf(TEXT("GOAP Debug: %s"), bGlobalDebugEnabled ? TEXT("ON") : TEXT("OFF")));
	}
}

void UGOAPComponent::SwitchGOAPDebugTarget(const UObject* WorldContextObject)
{
	static uint64 LastSwitchFrame = 0;
	if (GFrameCounter == LastSwitchFrame) return;
	LastSwitchFrame = GFrameCounter;

	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (!World) return;

	bool bFoundCurrent = false;
	UGOAPComponent* FirstComponent = nullptr;
	UGOAPComponent* NextComponent = nullptr;

	for (TObjectIterator<UGOAPComponent> It; It; ++It)
	{
		if (It->GetWorld() != World) continue;
		if (!FirstComponent) FirstComponent = *It;
		if (bFoundCurrent)
		{
			NextComponent = *It;
			break;
		}
		if (*It == CurrentDebugTarget.Get())
		{
			bFoundCurrent = true;
		}
	}
	CurrentDebugTarget = NextComponent ? NextComponent : FirstComponent;
}

void UGOAPComponent::DebugPrintWorldState(bool bPrintToScreen, FName SpecificKey)
{
	if (!GEngine || !GetOwner()) return;

	FString Msg = FString::Printf(TEXT("[%s] World States:"), *GetOwner()->GetName());
	if (SpecificKey != NAME_None)
	{
		Msg += FString::Printf(TEXT("\n%s = %d"), *SpecificKey.ToString(), CurrentWorldState.GetState(SpecificKey));
	}
	else
	{
		for (const TPair<FName, int32>& Pair : CurrentWorldState.States)
		{
			Msg += FString::Printf(TEXT("\n%s = %d"), *Pair.Key.ToString(), Pair.Value);
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("%s"), *Msg);
	if (bPrintToScreen)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Yellow, Msg);
	}
}

void UGOAPComponent::CheckForInterruption()
{
	if (!CurrentGoal || !CurrentAction) return;

	if (!CurrentAction->bInterruptable) return;

	UGOAPGoal* BestNewGoal = nullptr;
	float MaxPriority = -1.f;

	bool bShouldDebug = bGlobalDebugEnabled && (CurrentDebugTarget.Get() == this) && GEngine && GetOwner();

	FString DebugStr;
	if (bShouldDebug)
	{
		DebugStr = FString::Printf(TEXT("[GOAP 打断检测雷达 - %s]\n当前正在执行: %s (优先级: %.1f)"),
			*GetOwner()->GetName(),
			*CurrentGoal->GoalName.ToString(), CurrentGoal->CalculatePriority(this));
	}

	for (UGOAPGoal* Goal : AvailableGoals)
	{
		if (!Goal || Goal->bDisable) continue;

		if (!Goal->IsGoalAchieved(this))
		{
			bool bAlreadyMet = true;
			for (const FGOAPCondition& Cond : Goal->GoalConditions)
			{
				if (!Cond.Evaluate(CurrentWorldState.GetState(Cond.Key)))
				{
					bAlreadyMet = false;
					break;
				}
			}
			if (bAlreadyMet) continue;

			float Prio = Goal->CalculatePriority(this);

			if (bShouldDebug)
			{
				DebugStr += FString::Printf(TEXT("\n竞选目标 -> %s : %.1f"), *Goal->GoalName.ToString(), Prio);
			}

			if (Prio > MaxPriority)
			{
				MaxPriority = Prio;
				BestNewGoal = Goal;
			}
		}
	}

	if (bShouldDebug)
	{
		uint64 DebugKey = (uint64)GetOwner()->GetUniqueID() + 1000;
		GEngine->AddOnScreenDebugMessage(DebugKey, 0.0f, FColor::Orange, DebugStr);
	}

	if (BestNewGoal && BestNewGoal != CurrentGoal)
	{
		float CurrentPriority = CurrentGoal->CalculatePriority(this);
		float NewPriority = MaxPriority;

		if (NewPriority > (CurrentPriority + CurrentAction->InterruptCost))
		{
			UE_LOG(LogTemp, Warning, TEXT("GOAP Interrupted! Stopping [%s] to pursue [%s]"),
				*CurrentAction->ActionName.ToString(),
				*BestNewGoal->GoalName.ToString());

			AbortCurrentPlan();
		}
	}
}