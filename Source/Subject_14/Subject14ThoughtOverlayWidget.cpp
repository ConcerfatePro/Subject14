#include "Subject14ThoughtOverlayWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Styling/CoreStyle.h"
#include "TimerManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(Subject14ThoughtOverlayWidget)

namespace Subject14ThoughtWidgetPrivate
{
static TWeakObjectPtr<USubject14ThoughtOverlayWidget> GActiveThought;
}

void USubject14ThoughtOverlayWidget::ShowThoughtLine(
	UObject* const WorldContextObject,
	const FString& Line,
	const float HoldSeconds,
	const float FadeInSeconds,
	const float FadeOutSeconds)
{
	if (!WorldContextObject || Line.IsEmpty())
	{
		return;
	}

	UWorld* const World = WorldContextObject->GetWorld();
	if (!World)
	{
		return;
	}

	APlayerController* PC = nullptr;
	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* const Candidate = It->Get();
		if (Candidate && Candidate->IsLocalPlayerController())
		{
			PC = Candidate;
			break;
		}
	}
	if (!PC)
	{
		PC = UGameplayStatics::GetPlayerController(World, 0);
	}
	if (!PC || !PC->IsLocalPlayerController())
	{
		return;
	}

	if (Subject14ThoughtWidgetPrivate::GActiveThought.IsValid())
	{
		UWorld* const OldWorld = Subject14ThoughtWidgetPrivate::GActiveThought->GetWorld();
		if (OldWorld)
		{
			OldWorld->GetTimerManager().ClearAllTimersForObject(
				Subject14ThoughtWidgetPrivate::GActiveThought.Get());
		}
		Subject14ThoughtWidgetPrivate::GActiveThought->RemoveFromParent();
		Subject14ThoughtWidgetPrivate::GActiveThought.Reset();
	}

	USubject14ThoughtOverlayWidget* Widget = nullptr;
	if (UGameInstance* const GI = PC->GetGameInstance())
	{
		Widget = CreateWidget<USubject14ThoughtOverlayWidget>(GI, USubject14ThoughtOverlayWidget::StaticClass());
	}
	if (!Widget)
	{
		Widget = CreateWidget<USubject14ThoughtOverlayWidget>(PC, USubject14ThoughtOverlayWidget::StaticClass());
	}
	if (!Widget)
	{
#if !UE_BUILD_SHIPPING
		UE_LOG(LogTemp, Warning, TEXT("Subject14 ThoughtOverlay: CreateWidget failed."));
#endif
		return;
	}

	Widget->SetOwningPlayer(PC);
	Widget->QueuePendingShow(Line, HoldSeconds, FadeInSeconds, FadeOutSeconds);
	Widget->ApplyPendingShowAfterQueue();

	// Full-viewport overlay: AddToViewport is most reliable for a single local player.
	Widget->AddToViewport(5000);
	if (UGameViewportClient* const Viewport = World->GetGameViewport())
	{
		FVector2D Size;
		Viewport->GetViewportSize(Size);
		Widget->SetDesiredSizeInViewport(Size);
	}

	Subject14ThoughtWidgetPrivate::GActiveThought = Widget;

#if !UE_BUILD_SHIPPING
	UE_LOG(
		LogTemp,
		Log,
		TEXT("Subject14 ThoughtOverlay: showing line (len=%d, inViewport=%d)"),
		Line.Len(),
		Widget->IsInViewport() ? 1 : 0);
#endif
}

void USubject14ThoughtOverlayWidget::QueuePendingShow(
	const FString& Line,
	const float HoldSeconds,
	const float FadeInSeconds,
	const float FadeOutSeconds)
{
	PendingShowLine = Line;
	PendingShowHold = HoldSeconds;
	PendingFadeIn = FadeInSeconds;
	PendingFadeOut = FadeOutSeconds;
	bHasPendingShow = true;
}

void USubject14ThoughtOverlayWidget::ApplyPendingShowAfterQueue()
{
	FlushPendingShowInternal();
}

void USubject14ThoughtOverlayWidget::FlushPendingShowInternal()
{
	if (!bHasPendingShow || PendingShowLine.IsEmpty())
	{
		return;
	}

	if (!LineText)
	{
#if !UE_BUILD_SHIPPING
		UE_LOG(LogTemp, Warning, TEXT("Subject14 ThoughtOverlay: pending show but LineText is null"));
#endif
		return;
	}

	const FString LineCopy = PendingShowLine;
	const float HoldCopy = PendingShowHold;
	const float FadeInCopy = PendingFadeIn;
	const float FadeOutCopy = PendingFadeOut;
	PendingShowLine.Empty();
	bHasPendingShow = false;
	BeginShow(LineCopy, HoldCopy, FadeInCopy, FadeOutCopy);
}

void USubject14ThoughtOverlayWidget::InitializeNativeClassData()
{
	Super::InitializeNativeClassData();

	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"), RF_Transient);
	}

	UCanvasPanel* const RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("ThoughtRoot"));
	WidgetTree->RootWidget = RootCanvas;
	RootCanvas->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	LineText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ThoughtLine"));
	LineText->SetJustification(ETextJustify::Center);
	LineText->SetAutoWrapText(true);
	LineText->SetWrapTextAt(920.0f);
	LineText->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 22));
	LineText->SetShadowOffset(FVector2D(1.5f, 1.5f));
	LineText->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.62f));
	LineText->SetColorAndOpacity(FLinearColor(0.92f, 0.94f, 0.96f, 0.0f));

	if (UCanvasPanelSlot* const Slot = RootCanvas->AddChildToCanvas(LineText))
	{
		// Must span a non-zero vertical band: MinY == MaxY == 1 gives zero height (invisible text).
		Slot->SetAnchors(FAnchors(0.06f, 0.72f, 0.94f, 0.96f));
		Slot->SetAlignment(FVector2D(0.5f, 1.0f));
		Slot->SetOffsets(FMargin(8.0f, 0.0f, 8.0f, 12.0f));
	}
}

void USubject14ThoughtOverlayWidget::NativeDestruct()
{
	if (UWorld* const World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(FadeTickHandle);
	}
	Super::NativeDestruct();
}

void USubject14ThoughtOverlayWidget::HandleFadeTick()
{
	auto ResolveTimerWorld = [this]() -> UWorld*
	{
		if (const APlayerController* const OwningPC = GetOwningPlayer())
		{
			if (UWorld* const W = OwningPC->GetWorld())
			{
				return W;
			}
		}
		if (UWorld* const W = GetWorld())
		{
			return W;
		}
		if (const UGameInstance* const GI = GetGameInstance())
		{
			return GI->GetWorld();
		}
		return nullptr;
	};

	if (!LineText)
	{
		if (UWorld* const World = ResolveTimerWorld())
		{
			World->GetTimerManager().ClearTimer(FadeTickHandle);
		}
		return;
	}

	static constexpr float TickDt = 0.032f;

	switch (Phase)
	{
	case EPhase::Idle:
		if (UWorld* const World = ResolveTimerWorld())
		{
			World->GetTimerManager().ClearTimer(FadeTickHandle);
		}
		return;
	case EPhase::FadeIn:
		PhaseTimer += TickDt;
		{
			const float Fi = FMath::Max(0.05f, ActiveFadeInSeconds);
			const float Alpha = FMath::Clamp(PhaseTimer / Fi, 0.0f, 1.0f);
			LineText->SetColorAndOpacity(FLinearColor(0.92f, 0.94f, 0.96f, Alpha * 0.9f));
			if (PhaseTimer >= Fi)
			{
				Phase = EPhase::Hold;
				PhaseTimer = 0.0f;
			}
		}
		break;
	case EPhase::Hold:
		PhaseTimer += TickDt;
		if (PhaseTimer >= HoldDuration)
		{
			Phase = EPhase::FadeOut;
			PhaseTimer = 0.0f;
		}
		break;
	case EPhase::FadeOut:
		PhaseTimer += TickDt;
		{
			constexpr float StartA = 0.9f;
			const float Fo = FMath::Max(0.05f, ActiveFadeOutSeconds);
			const float T = FMath::Clamp(PhaseTimer / Fo, 0.0f, 1.0f);
			const float A = FMath::Lerp(StartA, 0.0f, T);
			LineText->SetColorAndOpacity(FLinearColor(0.92f, 0.94f, 0.96f, A));
			if (PhaseTimer >= Fo)
			{
				Phase = EPhase::Idle;
				if (UWorld* const World = ResolveTimerWorld())
				{
					World->GetTimerManager().ClearTimer(FadeTickHandle);
				}
				RemoveFromParent();
				if (Subject14ThoughtWidgetPrivate::GActiveThought.Get() == this)
				{
					Subject14ThoughtWidgetPrivate::GActiveThought.Reset();
				}
			}
		}
		break;
	default:
		break;
	}
}

void USubject14ThoughtOverlayWidget::BeginShow(
	const FString& Line,
	const float HoldSeconds,
	const float FadeInSeconds,
	const float FadeOutSeconds)
{
	if (!LineText)
	{
#if !UE_BUILD_SHIPPING
		UE_LOG(LogTemp, Warning, TEXT("Subject14 ThoughtOverlay: BeginShow skipped (LineText null)"));
#endif
		return;
	}

	LineText->SetText(FText::FromString(Line));
	HoldDuration = FMath::Clamp(HoldSeconds, 1.0f, 30.0f);
	ActiveFadeInSeconds = FMath::Clamp(FadeInSeconds, 0.05f, 10.0f);
	ActiveFadeOutSeconds = FMath::Clamp(FadeOutSeconds, 0.05f, 15.0f);
	Phase = EPhase::FadeIn;
	PhaseTimer = 0.0f;
	LineText->SetColorAndOpacity(FLinearColor(0.92f, 0.94f, 0.96f, 0.0f));

	UWorld* TimerWorld = nullptr;
	if (const APlayerController* const OwningPC = GetOwningPlayer())
	{
		TimerWorld = OwningPC->GetWorld();
	}
	if (!TimerWorld)
	{
		TimerWorld = GetWorld();
	}
	if (!TimerWorld && GetGameInstance())
	{
		TimerWorld = GetGameInstance()->GetWorld();
	}

	if (TimerWorld)
	{
		TimerWorld->GetTimerManager().ClearTimer(FadeTickHandle);
		TimerWorld->GetTimerManager().SetTimer(
			FadeTickHandle,
			FTimerDelegate::CreateUObject(this, &USubject14ThoughtOverlayWidget::HandleFadeTick),
			0.032f,
			true);
		HandleFadeTick();
	}
#if !UE_BUILD_SHIPPING
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Subject14 ThoughtOverlay: BeginShow has no UWorld for fade timer."));
	}
#endif
}
