#include "Subject14MainMenuGameMode.h"
#include "Subject14MainMenuPlayerController.h"
#include "GameFramework/SpectatorPawn.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(Subject14MainMenuGameMode)

ASubject14MainMenuGameMode::ASubject14MainMenuGameMode()
{
	PlayerControllerClass = ASubject14MainMenuPlayerController::StaticClass();
	DefaultPawnClass = ASpectatorPawn::StaticClass();
	HUDClass = nullptr;
}
