#include "Subject14NightEndPromptWidget.h"
#include "Subject14Night1Director.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Styling/CoreStyle.h"
#include "TimerManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(Subject14NightEndPromptWidget)

USubject14NightEndPromptWidget* USubject14NightEndPromptWidget::ShowPrompt(
	UObject* const WorldContextObject,
	ASubject14Night1Director* const Director,
	const FString& PromptText,
	const float PromptFadeInSeconds,
	const bool bAllowAnyKeyReturn)
{
	if (!WorldContextObject || !Director)
	{
		return nullptr;
	}

	const FString ResolvedText = PromptText.IsEmpty()
		? FString(TEXT("Press any key to return to main menu"))
		: PromptText;

	UWorld* const World = WorldContextObject->GetWorld();
	if (!World)
	{
		return nullptr;
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
		return nullptr;
	}

	USubject14NightEndPromptWidget* Widget = nullptr;
	if (UGameInstance* const GI = PC->GetGameInstance())
	{
		Widget = CreateWidget<USubject14NightEndPromptWidget>(GI, StaticClass());
	}
	if (!Widget)
	{
		Widget = CreateWidget<USubject14NightEndPromptWidget>(PC, StaticClass());
	}
	if (!Widget)
	{
		return nullptr;
	}

	Widget->SetOwningPlayer(PC);
	Widget->OwnerDirector = Director;
	Widget->PendingPromptText = ResolvedText;
	Widget->PendingFadeInSeconds = PromptFadeInSeconds;
	Widget->bPendingAllowAnyKey = bAllowAnyKeyReturn;
	Widget->bConfigurePending = true;

	Widget->AddToViewport(6000);
	if (UGameViewportClient* const Viewport = World->GetGameViewport())
	{
		FVector2D Size;
		Viewport->GetViewportSize(Size);
		Widget->SetDesiredSizeInViewport(Size);
	}

	Widget->ApplyConfigureAfterCreate();

	return Widget;
}

void USubject14NightEndPromptWidget::ApplyConfigureAfterCreate()
{
	if (!bConfigurePending)
	{
		return;
	}
	bConfigurePending = false;

	if (PromptLine)
	{
		PromptLine->SetText(FText::FromString(PendingPromptText));
		PromptLine->SetColorAndOpacity(FLinearColor(0.88f, 0.9f, 0.92f, 0.0f));
	}

	FadeAlpha = 0.0f;

	if (APlayerController* const PC = GetOwningPlayer())
	{
		PC->EnableInput(PC);

		FInputModeGameAndUI Mode;
		if (TSharedPtr<SWidget> const Cached = GetCachedWidget())
		{
			Mode.SetWidgetToFocus(Cached);
		}
		Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PC->SetInputMode(Mode);
		PC->SetShowMouseCursor(true);

		if (TSharedPtr<SWidget> const SlateWidget = GetCachedWidget())
		{
			FSlateApplication::Get().SetKeyboardFocus(SlateWidget, EFocusCause::SetDirectly);
		}
	}

	SetIsFocusable(true);

	if (UWorld* const World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(FadeTimerHandle);
		World->GetTimerManager().SetTimer(
			FadeTimerHandle,
			FTimerDelegate::CreateUObject(this, &USubject14NightEndPromptWidget::FadeInTick),
			0.032f,
			true);
		FadeInTick();
	}
}

void USubject14NightEndPromptWidget::InitializeNativeClassData()
{
	Super::InitializeNativeClassData();

	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("EndPromptWidgetTree"), RF_Transient);
	}

	UCanvasPanel* const RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("EndPromptRoot"));
	WidgetTree->RootWidget = RootCanvas;
	RootCanvas->SetVisibility(ESlateVisibility::Visible);

	PromptLine = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("EndPromptLine"));
	PromptLine->SetJustification(ETextJustify::Center);
	PromptLine->SetAutoWrapText(true);
	PromptLine->SetWrapTextAt(880.0f);
	PromptLine->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 20));
	PromptLine->SetShadowOffset(FVector2D(1.5f, 1.5f));
	PromptLine->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.75f));
	PromptLine->SetColorAndOpacity(FLinearColor(0.88f, 0.9f, 0.92f, 0.0f));

	if (UCanvasPanelSlot* const Slot = RootCanvas->AddChildToCanvas(PromptLine))
	{
		Slot->SetAnchors(FAnchors(0.08f, 0.78f, 0.92f, 0.94f));
		Slot->SetAlignment(FVector2D(0.5f, 1.0f));
		Slot->SetOffsets(FMargin(12.0f, 0.0f, 12.0f, 16.0f));
	}
}

void USubject14NightEndPromptWidget::NativeDestruct()
{
	if (UWorld* const World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(FadeTimerHandle);
	}
	Super::NativeDestruct();
}

void USubject14NightEndPromptWidget::FadeInTick()
{
	if (!PromptLine)
	{
		return;
	}

	static constexpr float TickDt = 0.032f;
	const float Target = FMath::Max(0.08f, PendingFadeInSeconds);
	FadeAlpha += TickDt;
	const float T = FMath::Clamp(FadeAlpha / Target, 0.0f, 1.0f);
	PromptLine->SetColorAndOpacity(FLinearColor(0.88f, 0.9f, 0.92f, T * 0.92f));

	if (FadeAlpha >= Target)
	{
		if (UWorld* const World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(FadeTimerHandle);
		}
	}
}

bool USubject14NightEndPromptWidget::ShouldAcceptKeyForReturn(const FKey& Key) const
{
	if (!Key.IsValid())
	{
		return false;
	}
	if (Key.IsModifierKey())
	{
		return false;
	}
	if (Key.IsAnalog())
	{
		return false;
	}

	if (bPendingAllowAnyKey)
	{
		return true;
	}

	return Key == EKeys::SpaceBar || Key == EKeys::Enter || Key == EKeys::Escape || Key == EKeys::Gamepad_FaceButton_Bottom;
}

void USubject14NightEndPromptWidget::TryFireReturn(const FKey& Key)
{
	if (!ShouldAcceptKeyForReturn(Key))
	{
		return;
	}

	if (ASubject14Night1Director* const Dir = OwnerDirector.Get())
	{
		Dir->HandleEndNightPromptCommitted();
	}
}

FReply USubject14NightEndPromptWidget::NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	if (!ShouldAcceptKeyForReturn(Key))
	{
		return FReply::Unhandled();
	}
	TryFireReturn(Key);
	return FReply::Handled();
}

FReply USubject14NightEndPromptWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (!bPendingAllowAnyKey)
	{
		return FReply::Unhandled();
	}

	const FKey Effect = InMouseEvent.GetEffectingButton();
	TryFireReturn(Effect);
	return FReply::Handled();
}
