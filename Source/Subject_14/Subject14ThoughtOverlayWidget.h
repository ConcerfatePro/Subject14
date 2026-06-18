#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Subject14ThoughtOverlayWidget.generated.h"

class UTextBlock;

/** Bottom-centered one-line “internal thought” overlay; built entirely in C++. */
UCLASS()
class SUBJECT_14_API USubject14ThoughtOverlayWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/**
	 * Shows text with a simple fade in / hold / fade out; replaces any previous instance for this PC.
	 * Fade durations clamp for stability; omit to use legacy defaults.
	 */
	UFUNCTION(BlueprintCallable, Category = "Subject14|Night", meta = (WorldContext = "WorldContextObject"))
	static void ShowThoughtLine(
		UObject* WorldContextObject,
		const FString& Line,
		float HoldSeconds = 4.5f,
		float FadeInSeconds = 0.65f,
		float FadeOutSeconds = 1.1f);

	void BeginShow(const FString& Line, float HoldSeconds, float FadeInSeconds, float FadeOutSeconds);

	/** Called before AddToViewport so NativeConstruct can run and then flush the show. */
	void QueuePendingShow(const FString& Line, float HoldSeconds, float FadeInSeconds, float FadeOutSeconds);

	/**
	 * Call after CreateWidget + QueuePendingShow. CreateWidget runs Initialize() immediately, so
	 * the tree exists but pending text was not set yet during InitializeNativeClassData.
	 */
	void ApplyPendingShowAfterQueue();

protected:
	virtual void InitializeNativeClassData() override;
	virtual void NativeDestruct() override;

private:
	void HandleFadeTick();
	void FlushPendingShowInternal();

	enum class EPhase : uint8
	{
		Idle,
		FadeIn,
		Hold,
		FadeOut
	};

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> LineText;

	EPhase Phase = EPhase::Idle;
	float PhaseTimer = 0.0f;
	float HoldDuration = 4.5f;
	float ActiveFadeInSeconds = 0.65f;
	float ActiveFadeOutSeconds = 1.1f;

	FTimerHandle FadeTickHandle;

	FString PendingShowLine;
	float PendingShowHold = 4.5f;
	float PendingFadeIn = 0.65f;
	float PendingFadeOut = 1.1f;
	bool bHasPendingShow = false;
};
