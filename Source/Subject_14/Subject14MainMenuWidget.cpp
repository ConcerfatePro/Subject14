#include "Subject14MainMenuWidget.h"
#include "Subject14MainMenuGameMode.h"
#include "Blueprint/WidgetTree.h"
#include "Misc/PackageName.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/World.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Styling/CoreStyle.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(Subject14MainMenuWidget)

namespace
{
static bool S14MapExists(const FString& ShortName)
{
	if (ShortName.IsEmpty())
	{
		return false;
	}
	const FString A = FString::Printf(TEXT("/Game/Maps/%s"), *ShortName);
	const FString B = FString::Printf(TEXT("/Game/%s"), *ShortName);
	return FPackageName::DoesPackageExist(A) || FPackageName::DoesPackageExist(B);
}
} // namespace

void USubject14MainMenuWidget::InitializeNativeClassData()
{
	Super::InitializeNativeClassData();

	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("MainMenuWidgetTree"), RF_Transient);
	}

	UCanvasPanel* const Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("MenuCanvas"));
	WidgetTree->RootWidget = Canvas;
	Canvas->SetVisibility(ESlateVisibility::Visible);

	UVerticalBox* const Root = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MenuRoot"));
	if (UCanvasPanelSlot* const CanvasSlot = Canvas->AddChildToCanvas(Root))
	{
		CanvasSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		CanvasSlot->SetOffsets(FMargin(0.0f));
		CanvasSlot->SetAlignment(FVector2D(0.5f, 0.42f));
	}

	auto AddText = [Root, this](const TCHAR* Txt, const int32 Size, const float PadBottom)
	{
		UTextBlock* const T = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		T->SetText(FText::FromString(Txt));
		T->SetJustification(ETextJustify::Center);
		T->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", Size));
		T->SetColorAndOpacity(FLinearColor(0.92f, 0.94f, 0.96f, 1.0f));
		if (UVerticalBoxSlot* const S = Root->AddChildToVerticalBox(T))
		{
			S->SetPadding(FMargin(24.0f, 0.0f, 24.0f, PadBottom));
			S->SetHorizontalAlignment(HAlign_Center);
		}
	};

	AddText(TEXT("Subject 14"), 36, 8.0f);
	AddText(TEXT("Prototype build"), 18, 32.0f);

	PlayButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("PlayButton"));
	PlayButton->SetIsEnabled(true);
	if (UTextBlock* const Bt = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("PlayLabel")))
	{
		Bt->SetText(FText::FromString(TEXT("Play Night 1 test")));
		Bt->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 18));
		Bt->SetJustification(ETextJustify::Center);
		PlayButton->AddChild(Bt);
	}
	if (UVerticalBoxSlot* const S = Root->AddChildToVerticalBox(PlayButton))
	{
		S->SetPadding(FMargin(48.0f, 0.0f, 48.0f, 16.0f));
		S->SetHorizontalAlignment(HAlign_Center);
	}

	QuitButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("QuitButton"));
	if (UTextBlock* const Qt = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("QuitLabel")))
	{
		Qt->SetText(FText::FromString(TEXT("Quit")));
		Qt->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 16));
		Qt->SetJustification(ETextJustify::Center);
		QuitButton->AddChild(Qt);
	}
	if (UVerticalBoxSlot* const S = Root->AddChildToVerticalBox(QuitButton))
	{
		S->SetPadding(FMargin(48.0f, 8.0f, 48.0f, 0.0f));
		S->SetHorizontalAlignment(HAlign_Center);
	}
}

void USubject14MainMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (PlayButton)
	{
		PlayButton->OnClicked.AddDynamic(this, &USubject14MainMenuWidget::OnPlayClicked);
	}
	if (QuitButton)
	{
		QuitButton->OnClicked.AddDynamic(this, &USubject14MainMenuWidget::OnQuitClicked);
	}
}

void USubject14MainMenuWidget::OnPlayClicked()
{
	UWorld* const World = GetWorld();
	if (!World)
	{
		return;
	}

	FName MapName = FName(TEXT("Lvl_Dev"));
	if (const ASubject14MainMenuGameMode* const MenuGM = Cast<ASubject14MainMenuGameMode>(World->GetAuthGameMode()))
	{
		MapName = MenuGM->PlayNightTestMapName;
	}

	const FString MapStr = MapName.ToString();
	if (!S14MapExists(MapStr))
	{
#if !UE_BUILD_SHIPPING
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("Subject14 MainMenu: map '%s' not found under /Game/Maps or /Game — opening current level name instead."),
			*MapStr);
#endif
		MapName = FName(*UGameplayStatics::GetCurrentLevelName(World, true));
	}

	UGameplayStatics::OpenLevel(World, MapName);
}

void USubject14MainMenuWidget::OnQuitClicked()
{
	if (APlayerController* const PC = GetOwningPlayer())
	{
		UKismetSystemLibrary::QuitGame(this, PC, EQuitPreference::Quit, false);
	}
}
