#include "GOAPGlobalSubsystem.h"

void UGOAPGlobalSubsystem::ModifyGlobalState(FName Key, int32 Delta)
{
	// 如果 Key 不存在会自动创建并初始化为0，然后加上 Delta
	int32& Value = GlobalStates.FindOrAdd(Key);
	Value += Delta;
}

int32 UGOAPGlobalSubsystem::GetGlobalState(FName Key) const
{
	if (const int32* Ptr = GlobalStates.Find(Key))
	{
		return *Ptr;
	}
	return 0; // 如果没人注册过这个状态，默认返回 0
}

void UGOAPGlobalSubsystem::SetGlobalState(FName Key, int32 Value)
{
	GlobalStates.Add(Key, Value);
}

void UGOAPGlobalSubsystem::DebugPrintGlobalStates(bool bEnable, FName SpecificKey)
{
	// 如果策划没有勾选 Enable，或者引擎不存在，直接返回
	if (!bEnable || !GEngine) return;

	// 用于收集所有文字的大字符串
	FString DebugMsg = TEXT("=== 🌍 GOAP Global States ===\n");

	if (SpecificKey != NAME_None)
	{
		// 1. 如果策划指定了具体要看的某个 State
		if (const int32* Val = GlobalStates.Find(SpecificKey))
		{
			DebugMsg += FString::Printf(TEXT("%s : %d\n"), *SpecificKey.ToString(), *Val);
		}
		else
		{
			DebugMsg += FString::Printf(TEXT("%s : [无数据/未注册]\n"), *SpecificKey.ToString());
		}
	}
	else
	{
		// 2. 如果策划没有指定 (SpecificKey 为 None)，则遍历打印全部
		if (GlobalStates.IsEmpty())
		{
			DebugMsg += TEXT("  (当前没有任何全局状态数据)\n");
		}
		else
		{
			for (const auto& Pair : GlobalStates)
			{
				DebugMsg += FString::Printf(TEXT("%s : %d\n"), *Pair.Key.ToString(), Pair.Value);
			}
		}
	}

	// 核心技巧：第一个参数(Key)使用固定的数字(如1001)，这样每帧更新时会覆盖上一帧的文字，而不是在屏幕上无限往下刷！
	GEngine->AddOnScreenDebugMessage(1001, 0.0f, FColor::Cyan, DebugMsg);
}