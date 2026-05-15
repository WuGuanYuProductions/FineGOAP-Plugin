#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "GOAPModifier.generated.h"

class UGOAPComponent;

/**
 * GOAP 动作消耗修饰器
 * 策划可以创建此类的蓝图子类，在里面根据玩家距离、血量等动态计算一个值，影响 Action 的最终 Cost 或 Cooldown
 */
UCLASS(Blueprintable, Abstract, EditInlineNew, DefaultToInstanced)
class FINEGOAP_API UGOAPModifier : public UObject
{
	GENERATED_BODY()

public:
	/*
	 * 核心计算函数：策划在蓝图中重写这个函数。
	 * 返回的值将与 Action 的 BaseCost 相加，或者作为 Cooldown 使用。
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "GOAP|Modifier")
	float CalculateModifier(UGOAPComponent* OwnerComp) const;

	// =========================================
	// 👁️ 编辑器实时预览机制
	// =========================================

	// 策划在面板里修改参数时，这里会实时显示最终的计算结果
	UPROPERTY(VisibleAnywhere, Transient, Category = "GOAP|Preview")
	float EditorPreviewValue;

	// 提供给蓝图的预览专用计算函数（因为在编辑器态是没有 Agent 实例的）
	UFUNCTION(BlueprintNativeEvent, Category = "GOAP|Preview")
	float CalculateEditorPreviewValue() const;

#if WITH_EDITOR
	// 当策划在细节面板修改任意数值时，自动触发此函数
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};