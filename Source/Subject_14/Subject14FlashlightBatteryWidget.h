#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Subject14FlashlightBatteryWidget.generated.h"

class UProgressBar;

/** Minimal screen-space battery bar (built in C++; optional BP subclass for styling). */
UCLASS()
class SUBJECT_14_API USubject14FlashlightBatteryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetBatteryPercent(float Fraction01);

protected:
	/** Pure C++ widget: build the tree here so RebuildWidget sees RootWidget (NativeConstruct is too late). */
	virtual void InitializeNativeClassData() override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UProgressBar> BatteryBar;
};
