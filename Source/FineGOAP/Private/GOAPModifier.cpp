#include "GOAPModifier.h"
#include "GOAPComponent.h" // 假定你工程中存在此文件

float UGOAPModifier::CalculateModifier_Implementation(UGOAPComponent* OwnerComp) const
{
	return 0.0f;
}

float UGOAPModifier::CalculateEditorPreviewValue_Implementation() const
{
	return 0.0f;
}

#if WITH_EDITOR
void UGOAPModifier::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	// [Bug修复/安全优化] 确保 Property 不为空。在 EditInlineNew 嵌套对象结构变动时，避免触发空指针导致的引擎崩溃
	if (PropertyChangedEvent.Property != nullptr)
	{
		EditorPreviewValue = CalculateEditorPreviewValue();
	}
}
#endif