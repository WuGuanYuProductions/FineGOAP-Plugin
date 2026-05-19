// Copyright WuGuanyu Productions, All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GOAPTypes.generated.h"

UENUM(BlueprintType)
enum class EGOAPConditionOp : uint8
{
	Equal           UMETA(DisplayName = "== (等于)"),
	NotEqual        UMETA(DisplayName = "!= (不等于)"),
	Greater         UMETA(DisplayName = "> (大于)"),
	GreaterOrEqual  UMETA(DisplayName = ">= (大于等于)"),
	Less            UMETA(DisplayName = "< (小于)"),
	LessOrEqual     UMETA(DisplayName = "<= (小于等于)")
};

UENUM(BlueprintType)
enum class EGOAPEffectOp : uint8
{
	Set             UMETA(DisplayName = "= (覆盖赋值)"),
	Add             UMETA(DisplayName = "+= (增加)"),
	Subtract        UMETA(DisplayName = "-= (减少)")
};

USTRUCT(BlueprintType)
struct FGOAPCondition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GOAP")
	FName Key;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GOAP")
	EGOAPConditionOp Operator = EGOAPConditionOp::Equal;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GOAP")
	int32 Value = 0;

	bool Evaluate(int32 StateValue) const
	{
		switch (Operator)
		{
		case EGOAPConditionOp::Equal:           return StateValue == Value;
		case EGOAPConditionOp::NotEqual:        return StateValue != Value;
		case EGOAPConditionOp::Greater:         return StateValue > Value;
		case EGOAPConditionOp::GreaterOrEqual:  return StateValue >= Value;
		case EGOAPConditionOp::Less:            return StateValue < Value;
		case EGOAPConditionOp::LessOrEqual:     return StateValue <= Value;
		default:                                return false;
		}
	}
};

USTRUCT(BlueprintType)
struct FGOAPState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GOAP", meta = (ToolTip = "Dictionary of states. Add state keys (e.g., 'HasWeapon') and their integer values here."))
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

USTRUCT(BlueprintType)
struct FGOAPEffect
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GOAP")
	FName Key;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GOAP")
	EGOAPEffectOp Operator = EGOAPEffectOp::Set;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GOAP")
	int32 Value = 0;

	void Apply(FGOAPState& InOutState) const
	{
		int32 CurrentValue = InOutState.GetState(Key, 0);
		switch (Operator)
		{
		case EGOAPEffectOp::Set:      InOutState.SetState(Key, Value); break;
		case EGOAPEffectOp::Add:      InOutState.SetState(Key, CurrentValue + Value); break;
		case EGOAPEffectOp::Subtract: InOutState.SetState(Key, CurrentValue - Value); break;
		}
	}
};