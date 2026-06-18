#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Subject14MainMenuPlayerController.generated.h"

class USubject14MainMenuWidget;

/** Spawns the main menu widget for the local player. */
UCLASS()
class SUBJECT_14_API ASubject14MainMenuPlayerController : public APlayerController
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;
};
