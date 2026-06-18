#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Subject14NightEndPromptWidget.generated.h"

class UTextBlock;
class ASubject14Night1Director;

/** Minimal “press any key” overlay for Night 1 completion; built in C++ like the thought widget. */
UCLASS()
class SUBJECT_14_API USubject14NightEndPromptWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	static USubject14NightEndPromptWidget* ShowPrompt(
		UObject* WorldContextObject,
		ASubject14Night1Director* Director,
		const FString& PromptText,
		float PromptFadeInSeconds,
		bool bAllowAnyKeyReturn);

	void ApplyConfigureAfterCreate();

protected:
	virtual void InitializeNativeClassData() override;
	virtual void NativeDestruct() override;
	virtual FReply NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

private:
	void FadeInTick();
	bool ShouldAcceptKeyForReturn(const FKey& Key) const;
	void TryFireReturn(const FKey& Key);

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> PromptLine;

	TWeakObjectPtr<ASubject14Night1Director> OwnerDirector;

	FString PendingPromptText;
	float PendingFadeInSeconds = 0.55f;
	bool bPendingAllowAnyKey = true;
	bool bConfigurePending = false;

	float FadeAlpha = 0.0f;
	FTimerHandle FadeTimerHandle;
};
