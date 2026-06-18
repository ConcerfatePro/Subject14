#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Subject14MainMenuWidget.generated.h"

class UButton;
class UTextBlock;
class UVerticalBox;

/** Minimal title + play / quit for a graybox main menu (C++ only). */
UCLASS()
class SUBJECT_14_API USubject14MainMenuWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void InitializeNativeClassData() override;
	virtual void NativeConstruct() override;

private:
	UFUNCTION()
	void OnPlayClicked();

	UFUNCTION()
	void OnQuitClicked();

	UPROPERTY(Transient)
	TObjectPtr<UButton> PlayButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> QuitButton;
};
