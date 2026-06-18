#include "Subject14FlashlightBatteryWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Blueprint/UserWidget.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ProgressBar.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(Subject14FlashlightBatteryWidget)

void USubject14FlashlightBatteryWidget::InitializeNativeClassData()
{
	Super::InitializeNativeClassData();

	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"), RF_Transient);
	}

	UCanvasPanel* const RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
	WidgetTree->RootWidget = RootCanvas;

	BatteryBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("BatteryBar"));
	BatteryBar->SetPercent(1.0f);
	BatteryBar->SetFillColorAndOpacity(FLinearColor(0.15f, 0.85f, 0.25f, 0.92f));

	if (UCanvasPanelSlot* const Slot = RootCanvas->AddChildToCanvas(BatteryBar))
	{
		// Top-left of the viewport (safe margin from corner).
		Slot->SetAnchors(FAnchors(0.0f, 0.0f, 0.0f, 0.0f));
		Slot->SetAlignment(FVector2D(0.0f, 0.0f));
		Slot->SetPosition(FVector2D(22.0f, 18.0f));
		Slot->SetSize(FVector2D(280.0f, 12.0f));
	}
}

void USubject14FlashlightBatteryWidget::SetBatteryPercent(const float Fraction01)
{
	if (!BatteryBar)
	{
		return;
	}

	const float P = FMath::Clamp(Fraction01, 0.0f, 1.0f);
	BatteryBar->SetPercent(P);

	FLinearColor Fill;
	if (P < 0.1f)
	{
		Fill = FLinearColor(0.95f, 0.18f, 0.14f, 0.94f);
	}
	else if (P < 0.25f)
	{
		Fill = FLinearColor(0.95f, 0.72f, 0.12f, 0.92f);
	}
	else
	{
		Fill = FLinearColor(0.18f, 0.82f, 0.28f, 0.92f);
	}
	BatteryBar->SetFillColorAndOpacity(Fill);
}
