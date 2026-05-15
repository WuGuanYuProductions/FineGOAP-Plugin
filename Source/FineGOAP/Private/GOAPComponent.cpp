#include "GOAPComponent.h"
#include "Engine/Engine.h"
#include "GameFramework/Pawn.h"
#include "AIController.h"
#include "Kismet/GameplayStatics.h" 
#include "GameFramework/Character.h"
#include "UObject/UObjectIterator.h"

// ==========================================
// 静态 Debug 变量初始化
// ==========================================
bool UGOAPComponent::bGlobalDebugEnabled = false;
TWeakObjectPtr<UGOAPComponent> UGOAPComponent::CurrentDebugTarget = nullptr;

// ==========================================
// 🧠 GOAP A* 算法专用的节点数据结构
// ==========================================
struct FGOAPNode
{
	FGOAPState State;
	UGOAPAction* Action;
	TSharedPtr<FGOAPNode> Parent;
	float Cost;
	int32 Depth;

	FGOAPNode() : Action(nullptr), Parent(nullptr), Cost(0.f), Depth(0) {}
};

// ==========================================
// 组件基础配置
// ==========================================
UGOAPComponent::UGOAPComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	CurrentAction = nullptr;
	CurrentGoal = nullptr;
}

void UGOAPComponent::BeginPlay()
{
	Super::BeginPlay();

	// 如果刚开始没有 Debug 目标，默认把自己设为目标
	if (!CurrentDebugTarget.IsValid())
	{
		CurrentDebugTarget = this;
	}
}

void UGOAPComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 清理静态调试指针，防止卸载关卡时出现野指针崩溃
	if (CurrentDebugTarget.Get() == this)
	{
		CurrentDebugTarget = nullptr;
	}
	Super::EndPlay(EndPlayReason);
}

// ==========================================
// 🛠️ 辅助函数 Helper Functions (支持双向兼容挂载)
// ==========================================
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

// ==========================================
// 🌍 状态管理
// ==========================================
void UGOAPComponent::SetWorldState(FName Key, int32 Value)
{
	CurrentWorldState.SetState(Key, Value);
}

int32 UGOAPComponent::GetWorldState(FName Key)
{
	return CurrentWorldState.GetState(Key, 0);
}

// ==========================================
// 🎯 核心大脑：正向 A* 规划器 (防死循环版)
// ==========================================
bool UGOAPComponent::BuildPlan()
{
	CurrentPlan.Empty();
	CurrentGoal = nullptr;

	UGOAPGoal* BestGoal = nullptr;
	float MaxPriority = -1.f;

	// --- [Debug 追踪 1]：检查目标数组 ---
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
			for (const auto& DesiredState : Goal->DesiredState)
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

	// --- [Debug 追踪 2]：A* 寻路追踪 ---
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
		for (const auto& DesiredState : BestGoal->DesiredState)
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
			for (const auto& Precond : Action->Preconditions)
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

					for (const auto& Effect : Action->Effects)
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

		while (TraceNode != nullptr && TraceNode->Action != nullptr)
		{
			CurrentPlan.Insert(TraceNode->Action, 0);
			TraceNode = TraceNode->Parent;
		}

		if (CurrentPlan.Num() > 0)
		{
			if (bGlobalDebugEnabled && GEngine) GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green, TEXT("🎉 [GOAP 追踪]: 寻路成功！找到了有效计划！"));
			return true;
		}
	}

	if (bGlobalDebugEnabled && GEngine) GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Red, TEXT("❌ [GOAP 追踪]: 寻路彻底失败！由于没找到连通的 Action，重置 Goal 为 None！"));
	CurrentGoal = nullptr;
	return false;
}

// ==========================================
// ⚙️ 组件心跳：管理动作的生老病死
// ==========================================
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
			for (const auto& Precond : CurrentAction->Preconditions)
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
		if (CurrentAction->InterruptCost >= 0.0f && CurrentGoal != nullptr)
		{
			for (UGOAPAction* CheckAction : CurrentGoal->GoalActions)
			{
				if (CheckAction && CheckAction != CurrentAction && !CheckAction->bDisable)
				{
					bool bPreconditionsMet = true;
					for (const auto& Precond : CheckAction->Preconditions)
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
							break;
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

			for (const auto& Effect : CurrentAction->Effects)
			{
				SetWorldState(Effect.Key, Effect.Value);
			}
			CurrentAction = nullptr;
		}
	}

	// 🎨 Debug UI (状态面板) - 已修复固定Key使其常驻不刷屏
	if (bGlobalDebugEnabled && CurrentDebugTarget.Get() == this && GEngine)
	{
		FString OwnerName = GetOwner() ? GetOwner()->GetName() : TEXT("Unknown AI");
		FString GoalStr = CurrentGoal ? CurrentGoal->GetClass()->GetName() : TEXT("None");

		// 【修复点 2】：把 TEXT("Idle") 改为更明确的提示，如果它为 None，说明没找到计划或正在等待条件满足
		FString ActionStr = CurrentAction ? CurrentAction->ActionName.ToString() : TEXT("None(Plan Failed/Waiting)");

		GoalStr = GoalStr.Replace(TEXT("_C"), TEXT(""));

		FString DebugStr = FString::Printf(TEXT("🤖 [GOAP Target]: %s\n🎯 [Current Goal]: %s\n🎬 [Current Action]: %s"),
			*OwnerName, *GoalStr, *ActionStr);

		// Key 使用 Component 的 UniqueID，防止不同 AI 互相覆盖
		GEngine->AddOnScreenDebugMessage((uint64)GetUniqueID(), 0.0f, FColor::Cyan, DebugStr, true, FVector2D(1.5f, 1.5f));
	}
}

void UGOAPComponent::AbortCurrentPlan()
{
	if (CurrentAction)
	{
		CurrentAction->StartCooldown(this);
		CurrentAction->OnActionEnd(this);
		CurrentAction = nullptr;
	}
	CurrentPlan.Empty();
	CurrentGoal = nullptr;
}

// ==========================================
// 🐛 调试函数实现 (已完全修复刷屏 & 双重触发Bug)
// ==========================================
void UGOAPComponent::ToggleGOAPDebug(const UObject* WorldContextObject)
{
	// 【修复Bug 1】：加入“帧计数器”锁。
	// 防止如果遍历了多个AI组件，或者按键事件一帧内触发了多次（如Pressed和Released连击），导致状态反复翻转抵消。
	static uint64 LastToggleFrame = 0;
	if (GFrameCounter == LastToggleFrame) return;
	LastToggleFrame = GFrameCounter;

	bGlobalDebugEnabled = !bGlobalDebugEnabled;
	if (GEngine)
	{
		FString Msg = bGlobalDebugEnabled ? TEXT("GOAP Debug: ON") : TEXT("GOAP Debug: OFF");
		// 使用固定 Key = 8888 强制在屏幕同一行刷新
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
		for (const auto& StatePair : CurrentWorldState.States)
		{
			FString Msg = FString::Printf(TEXT("[%s]: %d"), *StatePair.Key.ToString(), StatePair.Value);

			if (bPrintToScreen && GEngine)
			{
				// 【修复Bug 2】：给每一个 State 分配一个唯一的固定 Key (ComponentID + 序号偏移)。
				// 时间设置为 0.0f 或 2.0f。如果有相同 Key，它会在原地刷新这一行，彻底解决刷屏问题。
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