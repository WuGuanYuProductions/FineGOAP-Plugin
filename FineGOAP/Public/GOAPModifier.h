// Copyright WuGuanyu Productions, All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "GOAPModifier.generated.h"

class UGOAPComponent;

UCLASS(Blueprintable, Abstract, EditInlineNew, DefaultToInstanced)
class FINEGOAP_API UGOAPModifier : public UObject
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintNativeEvent, Category = "GOAP|Modifier", meta = (ToolTip = "Override this in Blueprint to dynamically calculate a value (e.g., based on distance or HP). This will affect the Action's Cost or Cooldown."))
	float CalculateModifier(UGOAPComponent* OwnerComp) const;

	UPROPERTY(VisibleAnywhere, Transient, Category = "GOAP|Preview", meta = (ToolTip = "Real-time preview of the calculated modifier value based on your current Blueprint settings."))
	float EditorPreviewValue;

	UFUNCTION(BlueprintNativeEvent, Category = "GOAP|Preview", meta = (ToolTip = "Provide a mock calculation logic here so designers can preview the result in the editor without running the game."))
	float CalculateEditorPreviewValue() const;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};