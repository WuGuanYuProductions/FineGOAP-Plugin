#pragma once

#include "CoreMinimal.h"
#include "GOAPTypes.generated.h"

// 封装状态字典，方便在蓝图和C++中使用
USTRUCT(BlueprintType)
struct FGOAPState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GOAP")
	TMap<FName, int32> States;

	void SetState(FName Key, int32 Value)
	{
		States.Add(Key, Value);
	}

	int32 GetState(FName Key, int32 DefaultValue = 0) const
	{
		if (const int32* Found = States.Find(Key))
		{
			return *Found;
		}
		return DefaultValue;
	}

	bool HasState(FName Key) const
	{
		return States.Contains(Key);
	}
};