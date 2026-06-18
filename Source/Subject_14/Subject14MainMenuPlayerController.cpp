#include "Subject14MainMenuPlayerController.h"
#include "Subject14MainMenuWidget.h"
#include "Blueprint/UserWidget.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(Subject14MainMenuPlayerController)

void ASubject14MainMenuPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocalPlayerController())
	{
		return;
	}

	USubject14MainMenuWidget* const Menu = CreateWidget<USubject14MainMenuWidget>(this, USubject14MainMenuWidget::StaticClass());
	if (!Menu)
	{
		return;
	}

	Menu->AddToViewport(0);

	FInputModeUIOnly InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);
	bShowMouseCursor = true;
}
