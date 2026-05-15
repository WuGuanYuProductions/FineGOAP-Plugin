#include "GOAPComponent.h"
#include "Engine/Engine.h"
#include "GameFramework/Pawn.h"
#include "AIController.h"
#include "Kismet/GameplayStatics.h" 
#include "GameFramework/Character.h"
#include "UObject/UObjectIterator.h"
#include "Algo/Reverse.h" // [性能优化] 引入算法库支持 O(N) 翻转

bool UGOAPComponent::bGlobalDebugEnabled = false;
TWeakObjectPtr<UGOAPComponent> UGOAPComponent::CurrentDebugTarget = nullptr;

struct FGOAPNode
{
	FGOAPState State;
	UGOAPAction* Action;
	TSharedPtr<FGOAPNode> Parent;
	float Cost;
	int32 Depth;

	FGOAPNode() : Action(nullptr), Parent(nullptr), Cost(0.f), Depth(0) {}
};

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
	// ======================================================================
	// 🔴【核心死锁修复】强制回收机制
	// 当NPC意外死亡被Destroy时，强行调用当前Action的 OnActionEnd。
	// 这保证了策划在蓝图中写的 GlobalState -1 (释放令牌) 逻辑绝对会被执行，避免全局死锁。
	// ======================================================================
	if (CurrentAction != nullptr)
	{
		CurrentAction->OnActionEnd(this);
		CurrentAction = nullptr;
	}

	if (CurrentDebugTarget.Get() == this)
	{
		CurrentDebugTarget = nullptr;
	}
	Super::EndPlay(EndPlayReason);
}

APawn* UGOAPComponent::GetAIPawn()
{
	if (APawn* OwnerPawn = Cast<APawn>(GetOwner())) return OwnerPawn;
	if (AAIController* OwnerController = Cast<AAIController>(GetOwner())) return OwnerController->GetPawn();
	return nullptr;
}

AAIController* UGOAPComponent::GetAIController()
{
	if (AAIController* OwnerController = Cast<AAIController>(GetOwner())) return OwnerController;
	if (APawn* OwnerPawn = Cast<APawn>(GetOwner())) return Cast<AAIController>(OwnerPawn->GetController());
	return nullptr;
}

ACharacter* UGOAPComponent::GetPlayer()
{
	return UGameplayStatics::GetPlayerCharacter(this, 0);
}

void UGOAPComponent::SetWorldState(FName Key, int32 Value)
{
	CurrentWorldState.SetState(Key, Value);
}

int32 UGOAPComponent::GetWorldState(FName Key)
{
	return CurrentWorldState.GetState(Key, 0);
}

bool UGOAPComponent::BuildPlan()
{
	// [性能优化] 使用 Reset() 替代 Empty()，保留底层内存容量，避免高频规划时的 GC 与内存碎片分配开销
	CurrentPlan.Reset();
	CurrentGoal = nullptr;

	UGOAPGoal* BestGoal = nullptr;
	float MaxPriority = -1.f;

	if (bGlobalDebugEnabled && GEngine) {
		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Yellow, FString::Printf(TEXT("🔍 [GOAP 追踪]: 正在评估 %d 个 Goals..."), AvailableGoals.Num()));
	}

	for (UGOAPGoal* Goal : AvailableGoals)
	{
		if (!Goal)
		{
			if (bGlobalDebugEnabled && GEngine) GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Red, TEXT("❌ 警告: AvailableGoals 中存在空指针(Null)!"));
			continue;
		}
		if (Goal->bDisable) continue;

		if (!Goal->IsGoalAchieved(this))
		{
			bool bAlreadyMet = true;
			// [规范优化] TPair 代替 auto，消除隐式转换的性能损耗
			for (const TPair<FName, int32>& DesiredState : Goal->DesiredState)
			{
				if (CurrentWorldState.GetState(DesiredState.Key) != DesiredState.Value)
				{
					bAlreadyMet = false;
					break;
				}
			}

			if (bAlreadyMet)
			{
				if (bGlobalDebugEnabled && GEngine) GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Orange, FString::Printf(TEXT("⏭️ 略过: %s 的期望状态已在世界中满足。"), *Goal->GetName()));
				continue;
			}

			float Prio = Goal->CalculatePriority(this);
			if (bGlobalDebugEnabled && GEngine) GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Cyan, FString::Printf(TEXT("📊 评估 Goal: %s | 算得优先级: %f"), *Goal->GetName(), Prio));

			if (Prio > MaxPriority)
			{
				MaxPriority = Prio;
				BestGoal = Goal;
			}
		}
		else
		{
			if (bGlobalDebugEnabled && GEngine) GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Orange, FString::Printf(TEXT("⏭️ 略过: %s 自带的 IsGoalAchieved 返回了 True!"), *Goal->GetName()));
		}
	}

	if (!BestGoal)
	{
		if (bGlobalDebugEnabled && GEngine) GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Red, TEXT("❌ [GOAP 追踪]: 失败！没有找到任何可用的优先级大于 0 的 Goal！"));
		return false;
	}

	CurrentGoal = BestGoal;

	if (bGlobalDebugEnabled && GEngine) {
		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green, FString::Printf(TEXT("✅ 选中最佳 Goal: %s | 开始规划路径! (动作池有 %d 个)"), *BestGoal->GetName(), BestGoal->GoalActions.Num()));
	}

	TArray<TSharedPtr<FGOAPNode>> OpenList;
	TSharedPtr<FGOAPNode> StartNode = MakeShared<FGOAPNode>();
	StartNode->State = CurrentWorldState;
	OpenList.Add(StartNode);

	TSharedPtr<FGOAPNode> GoalNode = nullptr;

	const int32 MaxDepth = 10;
	const int32 MaxIterations = 1000;
	int32 IterationCount = 0;

	while (OpenList.Num() > 0 && IterationCount < MaxIterations)
	{
		IterationCount++;

		int32 BestNodeIndex = 0;
		for (int32 i = 1; i < OpenList.Num(); ++i)
		{
			if (OpenList[i]->Cost < OpenList[BestNodeIndex]->Cost)
			{
				BestNodeIndex = i;
			}
		}

		TSharedPtr<FGOAPNode> CurrentNode = OpenList[BestNodeIndex];
		OpenList.RemoveAt(BestNodeIndex);

		bool bGoalMet = true;
		for (const TPair<FName, int32>& DesiredState : BestGoal->DesiredState)
		{
			if (CurrentNode->State.GetState(DesiredState.Key) != DesiredState.Value)
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

		for (UGOAPAction* Action : BestGoal->GoalActions)
		{
			if (!Action)
			{
				if (bGlobalDebugEnabled && GEngine) GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Red, TEXT("❌ 警告: Action 池中存在空指针(Null)!"));
				continue;
			}
			if (Action->bDisable) continue;

			bool bPreconditionsMet = true;
			for (const TPair<FName, int32>& Precond : Action->Preconditions)
			{
				if (CurrentNode->State.GetState(Precond.Key) != Precond.Value)
				{
					bPreconditionsMet = false;
					break;
				}
			}

			if (!bPreconditionsMet)
			{
				if (bGlobalDebugEnabled && GEngine) GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Orange, FString::Printf(TEXT("🚫 拒绝 Action [%s]: 前置条件不满足"), *Action->GetName()));
			}

			if (bPreconditionsMet)
			{
				if (!Action->IsActionAvailable(this))
				{
					if (bGlobalDebugEnabled && GEngine) GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Red, FString::Printf(TEXT("🚫 拒绝 Action [%s]: IsActionAvailable 返回了 False!"), *Action->GetName()));
				}
				else
				{
					if (bGlobalDebugEnabled && GEngine) GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Cyan, FString::Printf(TEXT("✅ 采纳 Action [%s] 尝试推进路线!"), *Action->GetName()));

					TSharedPtr<FGOAPNode> NewNode = MakeShared<FGOAPNode>();
					NewNode->Parent = CurrentNode;
					NewNode->Action = Action;
					NewNode->Depth = CurrentNode->Depth + 1;
					NewNode->Cost = CurrentNode->Cost + Action->CalculateCost(this);
					NewNode->State = CurrentNode->State;

					for (const TPair<FName, int32>& Effect : Action->Effects)
					{
						NewNode->State.SetState(Effect.Key, Effect.Value);
					}

					OpenList.Add(NewNode);
				}
			}
		}
	}

	if (GoalNode != nullptr)
	{
		TSharedPtr<FGOAPNode> TraceNode = GoalNode;

		// [性能优化] 废弃极为耗时的 O(N²) Insert(0) 做法
		// 改为顺序 Add (O(1)) ，并在循环结束后翻转数组 (O(N))，在大规模规划时性能显著提升
		while (TraceNode != nullptr && TraceNode->Action != nullptr)
		{
			CurrentPlan.Add(TraceNode->Action);
			TraceNode = TraceNode->Parent;
		}

		if (CurrentPlan.Num() > 0)
		{
			Algo::Reverse(CurrentPlan);

			if (bGlobalDebugEnabled && GEngine) GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green, TEXT("🎉 [GOAP 追踪]: 寻路成功！找到了有效计划！"));
			return true;
		}
	}

	if (bGlobalDebugEnabled && GEngine) GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Red, TEXT("❌ [GOAP 追踪]: 寻路彻底失败！由于没找到连通的 Action，重置 Goal 为 None！"));
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
			for (const TPair<FName, int32>& Precond : CurrentAction->Preconditions)
			{
				if (CurrentWorldState.GetState(Precond.Key) != Precond.Value)
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
				if (GEngine && bGlobalDebugEnabled && CurrentDebugTarget.Get() == this) {
					GEngine->AddOnScreenDebugMessage(6677, 3.0f, FColor::Red, TEXT("❌ [GOAP] 计划中止: 下一个 Action 条件已不满足!"));
				}
				AbortCurrentPlan();
				return;
			}
		}
	}

	if (CurrentAction != nullptr)
	{
		// 处理动作打断 (Interrupt) 检查
		if (CurrentAction->InterruptCost >= 0.0f && CurrentGoal != nullptr)
		{
			for (UGOAPAction* CheckAction : CurrentGoal->GoalActions)
			{
				if (CheckAction && CheckAction != CurrentAction && !CheckAction->bDisable)
				{
					bool bPreconditionsMet = true;
					for (const TPair<FName, int32>& Precond : CheckAction->Preconditions)
					{
						if (CurrentWorldState.GetState(Precond.Key) != Precond.Value)
						{
							bPreconditionsMet = false;
							break;
						}
					}

					if (bPreconditionsMet && CheckAction->IsActionAvailable(this))
					{
						float DynamicCost = CheckAction->CalculateCost(this);

						if (DynamicCost < CurrentAction->InterruptCost)
						{
							if (GEngine && bGlobalDebugEnabled && CurrentDebugTarget.Get() == this) {
								GEngine->AddOnScreenDebugMessage(6678, 3.0f, FColor::Yellow, FString::Printf(TEXT("⚔️ [%s] 打断触发: [%s] 取代了 [%s]"),
									*GetOwner()->GetName(), *CheckAction->GetName(), *CurrentAction->GetName()));
							}
							AbortCurrentPlan();
							break; // 触发打断，跳出循环
						}
					}
				}
			}
		}

		if (CurrentAction == nullptr) return;

		bool bIsFinished = CurrentAction->OnActionTick(this, DeltaTime);

		if (bIsFinished && CurrentAction != nullptr)
		{
			CurrentAction->StartCooldown(this);
			CurrentAction->OnActionEnd(this);

			for (const TPair<FName, int32>& Effect : CurrentAction->Effects)
			{
				SetWorldState(Effect.Key, Effect.Value);
			}
			CurrentAction = nullptr;
		}
	}

	// 打印当前调试目标状态
	if (bGlobalDebugEnabled && CurrentDebugTarget.Get() == this && GEngine)
	{
		FString OwnerName = GetOwner() ? GetOwner()->GetName() : TEXT("Unknown AI");
		FString GoalStr = CurrentGoal ? CurrentGoal->GetClass()->GetName() : TEXT("None");
		FString ActionStr = CurrentAction ? CurrentAction->ActionName.ToString() : TEXT("None(Plan Failed/Waiting)");

		GoalStr = GoalStr.Replace(TEXT("_C"), TEXT(""));

		FString DebugStr = FString::Printf(TEXT("🤖 [GOAP Target]: %s\n🎯 [Current Goal]: %s\n🎬 [Current Action]: %s"),
			*OwnerName, *GoalStr, *ActionStr);

		GEngine->AddOnScreenDebugMessage((uint64)GetUniqueID(), 0.0f, FColor::Cyan, DebugStr, true, FVector2D(1.5f, 1.5f));
	}
}

void UGOAPComponent::AbortCurrentPlan()
{
	if (CurrentAction)
	{
		CurrentAction->StartCooldown(this);
		CurrentAction->OnActionEnd(this); // 确保被打断时令牌能安全释放
		CurrentAction = nullptr;
	}
	CurrentPlan.Reset();
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
		FString Msg = bGlobalDebugEnabled ? TEXT("GOAP Debug: ON") : TEXT("GOAP Debug: OFF");
		GEngine->AddOnScreenDebugMessage(8888, 3.0f, bGlobalDebugEnabled ? FColor::Green : FColor::Red, Msg);
	}
}

void UGOAPComponent::SwitchGOAPDebugTarget(const UObject* WorldContextObject)
{
	static uint64 LastSwitchFrame = 0;
	if (GFrameCounter == LastSwitchFrame) return;
	LastSwitchFrame = GFrameCounter;

	if (!WorldContextObject || !WorldContextObject->GetWorld()) return;

	bool bFoundCurrent = false;
	UGOAPComponent* FirstComp = nullptr;
	UGOAPComponent* NextComp = nullptr;

	for (TObjectIterator<UGOAPComponent> It; It; ++It)
	{
		if (It->GetWorld() != WorldContextObject->GetWorld()) continue;
		if (!FirstComp) FirstComp = *It;

		if (bFoundCurrent)
		{
			NextComp = *It;
			break;
		}
		if (CurrentDebugTarget.Get() == *It)
		{
			bFoundCurrent = true;
		}
	}

	CurrentDebugTarget = NextComp ? NextComp : FirstComp;

	if (GEngine && CurrentDebugTarget.IsValid())
	{
		FString TargetName = CurrentDebugTarget->GetOwner() ? CurrentDebugTarget->GetOwner()->GetName() : TEXT("Unknown AI");
		GEngine->AddOnScreenDebugMessage(8889, 3.0f, FColor::Yellow, FString::Printf(TEXT("🔄 [GOAP] 已切换观察目标: %s"), *TargetName));
	}
}

void UGOAPComponent::DebugPrintWorldState(bool bPrintToScreen, FName KeyToPrint)
{
	if (KeyToPrint.IsNone())
	{
		int32 PrintIndex = 0;
		for (const TPair<FName, int32>& StatePair : CurrentWorldState.States)
		{
			FString Msg = FString::Printf(TEXT("[%s]: %d"), *StatePair.Key.ToString(), StatePair.Value);

			if (bPrintToScreen && GEngine)
			{
				uint64 MsgKey = (uint64)GetUniqueID() + 1000 + PrintIndex;
				GEngine->AddOnScreenDebugMessage(MsgKey, 2.0f, FColor::Cyan, Msg);
			}
			PrintIndex++;
		}
	}
	else
	{
		int32 Val = GetWorldState(KeyToPrint);
		FString Msg = FString::Printf(TEXT("[%s]: %d"), *KeyToPrint.ToString(), Val);
		if (bPrintToScreen && GEngine)
		{
			uint64 MsgKey = (uint64)GetUniqueID() + 2000 + GetTypeHash(KeyToPrint);
			GEngine->AddOnScreenDebugMessage(MsgKey, 2.0f, FColor::Cyan, Msg);
		}
	}
}