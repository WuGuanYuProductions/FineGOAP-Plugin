#include "GOAPModifier.h"

float UGOAPModifier::CalculateModifier_Implementation(UGOAPComponent* OwnerComp) const
{
	return 0.0f;
}

float UGOAPModifier::CalculateEditorPreviewValue_Implementation() const
{
	// 默认返回0，建议策划在蓝图子类中重写这个函数返回假定值用于预览
	return 0.0f;
}

#if WITH_EDITOR
void UGOAPModifier::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	// 只要策划在面板修改了任何数值，就重新计算并刷新预览值！
	EditorPreviewValue = CalculateEditorPreviewValue();
}
#endif