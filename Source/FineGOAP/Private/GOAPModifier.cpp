// Copyright WuGuanyu Productions, All Rights Reserved.

#include "GOAPModifier.h"
#include "GOAPComponent.h" 

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

	if (PropertyChangedEvent.Property != nullptr)
	{
		EditorPreviewValue = CalculateEditorPreviewValue();
	}
}
#endif