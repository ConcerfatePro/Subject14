#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Subject14MainMenuGameMode.generated.h"

/** Boots to a simple title screen; opens the night test map from the menu widget. */
UCLASS()
class SUBJECT_14_API ASubject14MainMenuGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ASubject14MainMenuGameMode();

	/** Short map name passed to OpenLevel (e.g. Lvl_Dev). */
	UPROPERTY(EditDefaultsOnly, Category = "Subject14|Menu")
	FName PlayNightTestMapName = FName(TEXT("Lvl_Dev"));
};
