#include "Subject14FirstPersonCharacter.h"
#include "Subject14FlashlightBatteryWidget.h"
#include "Blueprint/UserWidget.h"
#include "Camera/CameraComponent.h"
#include "Components/AudioComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SpotLightComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "Interactable.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundWave.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

ASubject14FirstPersonCharacter::ASubject14FirstPersonCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->InitCapsuleSize(42.0f, 96.0f);

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = true;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->MaxWalkSpeed = 420.0f;
	GetCharacterMovement()->JumpZVelocity = 460.0f;
	GetCharacterMovement()->AirControl = 0.35f;

	FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCamera->SetupAttachment(GetCapsuleComponent());
	FirstPersonCamera->SetRelativeLocation(FVector(0.0f, 0.0f, 64.0f));
	FirstPersonCamera->bUsePawnControlRotation = true;

	Flashlight = CreateDefaultSubobject<USpotLightComponent>(TEXT("Flashlight"));
	Flashlight->SetupAttachment(FirstPersonCamera);
	Flashlight->SetRelativeLocation(FVector(12.0f, 6.0f, 0.0f));
	Flashlight->IntensityUnits = ELightUnits::Lumens;
	Flashlight->SetIntensity(FlashlightIntensityLumens);
	Flashlight->SetInnerConeAngle(FlashlightInnerConeDegrees);
	Flashlight->SetOuterConeAngle(FlashlightOuterConeDegrees);
	Flashlight->AttenuationRadius = FlashlightAttenuationRadius;
	Flashlight->CastShadows = true;
	Flashlight->SetTemperature(6500.0f);
	Flashlight->SetVisibility(bFlashlightStartsOn);
	bFlashlightOn = bFlashlightStartsOn;

	AmbientBedAudio = CreateDefaultSubobject<UAudioComponent>(TEXT("AmbientBed"));
	AmbientBedAudio->SetupAttachment(GetCapsuleComponent());
	AmbientBedAudio->bAutoActivate = false;

	// Defaults use /Game paths after Content/Audio WAVs are imported in the editor.
	static ConstructorHelpers::FObjectFinder<USoundWave> FootstepConcrete1(
		TEXT("/Game/Audio/FootstepConcrete1.FootstepConcrete1"));
	static ConstructorHelpers::FObjectFinder<USoundWave> FootstepConcrete2(
		TEXT("/Game/Audio/FootstepConcrete2.FootstepConcrete2"));
	static ConstructorHelpers::FObjectFinder<USoundWave> FootstepConcrete3(
		TEXT("/Game/Audio/FootstepConcrete3.FootstepConcrete3"));
	if (FootstepConcrete1.Succeeded())
	{
		FootstepSounds.Add(FootstepConcrete1.Object);
	}
	if (FootstepConcrete2.Succeeded())
	{
		FootstepSounds.Add(FootstepConcrete2.Object);
	}
	if (FootstepConcrete3.Succeeded())
	{
		FootstepSounds.Add(FootstepConcrete3.Object);
	}

	static ConstructorHelpers::FObjectFinder<USoundWave> AmbientHum(
		TEXT("/Game/Audio/AmbientFacilityHum.AmbientFacilityHum"));
	if (AmbientHum.Succeeded())
	{
		AmbientBedSound = AmbientHum.Object;
	}

	static ConstructorHelpers::FObjectFinder<USoundWave> FlashOn(TEXT("/Game/Audio/FlashlightOn.FlashlightOn"));
	if (FlashOn.Succeeded())
	{
		FlashlightOnSound = FlashOn.Object;
	}
	static ConstructorHelpers::FObjectFinder<USoundWave> FlashOff(TEXT("/Game/Audio/FlashlightOff.FlashlightOff"));
	if (FlashOff.Succeeded())
	{
		FlashlightOffSound = FlashOff.Object;
	}
	static ConstructorHelpers::FObjectFinder<USoundWave> LowBatt(TEXT("/Game/Audio/LowBattery.LowBattery"));
	if (LowBatt.Succeeded())
	{
		LowBatterySound = LowBatt.Object;
	}
}

void ASubject14FirstPersonCharacter::BeginPlay()
{
	Super::BeginPlay();

	FlashlightBattery01 = FMath::Clamp(FlashlightStartingBatteryFraction, 0.0f, 1.0f);

	if (Flashlight)
	{
		Flashlight->SetIntensity(FlashlightIntensityLumens);
		Flashlight->SetInnerConeAngle(FlashlightInnerConeDegrees);
		Flashlight->SetOuterConeAngle(FlashlightOuterConeDegrees);
		Flashlight->AttenuationRadius = FlashlightAttenuationRadius;
	}

	const bool bCanStartLit =
		bFlashlightInfiniteBattery || FlashlightBattery01 > KINDA_SMALL_NUMBER;
	bFlashlightOn = bFlashlightStartsOn && bCanStartLit;
	if (Flashlight)
	{
		Flashlight->SetVisibility(bFlashlightOn);
	}

	ScheduleBatteryHudInit();
	TryStartAmbientBed();
}

void ASubject14FirstPersonCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	ScheduleBatteryHudInit();
	TryStartAmbientBed();
}

void ASubject14FirstPersonCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* const World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(FlickerTimerHandle);
		World->GetTimerManager().ClearTimer(BatteryHudRetryTimer);
	}
	StopAmbientBed();
	DestroyBatteryHud();
	Super::EndPlay(EndPlayReason);
}

void ASubject14FirstPersonCharacter::ScheduleBatteryHudInit()
{
	if (!bUseBatteryUmWidget || BatteryHudWidget)
	{
		return;
	}

	UWorld* const World = GetWorld();
	if (!World)
	{
		return;
	}

	World->GetTimerManager().ClearTimer(BatteryHudRetryTimer);
	BatteryHudRetryAttempts = 0;
	World->GetTimerManager().SetTimer(
		BatteryHudRetryTimer,
		this,
		&ASubject14FirstPersonCharacter::CreateBatteryHud,
		0.05f,
		false);
}

void ASubject14FirstPersonCharacter::CreateBatteryHud()
{
	if (!bUseBatteryUmWidget || BatteryHudWidget)
	{
		return;
	}

	APlayerController* const PC = Cast<APlayerController>(GetController());
	if (!PC || !PC->IsLocalPlayerController())
	{
		return;
	}

	if (!PC->Player)
	{
		UWorld* const World = GetWorld();
		if (World && BatteryHudRetryAttempts < 48)
		{
			++BatteryHudRetryAttempts;
			World->GetTimerManager().SetTimer(
				BatteryHudRetryTimer,
				this,
				&ASubject14FirstPersonCharacter::CreateBatteryHud,
				0.08f,
				false);
		}
#if !UE_BUILD_SHIPPING
		else if (World)
		{
			UE_LOG(
				LogTemp,
				Error,
				TEXT("Subject14: Battery HUD could not be created (PlayerController has no ULocalPlayer yet)."));
		}
#endif
		return;
	}

	BatteryHudRetryAttempts = 0;

	UClass* const WidgetClass = FlashlightHudWidgetClass
		? FlashlightHudWidgetClass.Get()
		: USubject14FlashlightBatteryWidget::StaticClass();

	if (UGameInstance* const GI = PC->GetGameInstance())
	{
		BatteryHudWidget = CreateWidget<USubject14FlashlightBatteryWidget>(GI, WidgetClass);
	}
	else
	{
		BatteryHudWidget = CreateWidget<USubject14FlashlightBatteryWidget>(PC, WidgetClass);
	}

	if (!BatteryHudWidget)
	{
		UWorld* const World = GetWorld();
		if (World && BatteryHudRetryAttempts < 48)
		{
			++BatteryHudRetryAttempts;
			World->GetTimerManager().SetTimer(
				BatteryHudRetryTimer,
				this,
				&ASubject14FirstPersonCharacter::CreateBatteryHud,
				0.1f,
				false);
		}
#if !UE_BUILD_SHIPPING
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Subject14: CreateWidget returned null for battery HUD."));
		}
#endif
		return;
	}

	BatteryHudWidget->SetOwningPlayer(PC);
	if (!BatteryHudWidget->AddToPlayerScreen(25))
	{
		BatteryHudWidget->AddToViewport(25);
	}

#if !UE_BUILD_SHIPPING
	if (!BatteryHudWidget->IsInViewport())
	{
		UE_LOG(LogTemp, Warning, TEXT("Subject14: Battery HUD was created but is not in the viewport."));
	}
#endif

	BatteryHudWidget->SetBatteryPercent(bFlashlightInfiniteBattery ? 1.0f : FlashlightBattery01);
}

void ASubject14FirstPersonCharacter::DestroyBatteryHud()
{
	if (BatteryHudWidget)
	{
		BatteryHudWidget->RemoveFromParent();
		BatteryHudWidget = nullptr;
	}
}

void ASubject14FirstPersonCharacter::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (Flashlight && !bFlashlightInfiniteBattery && bFlashlightOn && FlashlightBattery01 > 0.0f)
	{
		const float DrainSeconds = FMath::Max(FlashlightBatteryDrainSecondsForFullDeplete, 5.0f);
		FlashlightBattery01 = FMath::Clamp(
			FlashlightBattery01 - DeltaSeconds / DrainSeconds,
			0.0f,
			1.0f);

		if (FlashlightBattery01 <= KINDA_SMALL_NUMBER)
		{
			FlashlightBattery01 = 0.0f;
			bFlashlightOn = false;
			Flashlight->SetVisibility(false);
			UE_LOG(LogTemp, Log, TEXT("Subject14 Flashlight: battery empty"));
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(
					-1,
					2.5f,
					FColor::Red,
					TEXT("Flashlight battery died."));
			}
		}
	}

	if (BatteryHudWidget)
	{
		BatteryHudWidget->SetBatteryPercent(bFlashlightInfiniteBattery ? 1.0f : FlashlightBattery01);
	}

	MaybeUpdateBatteryHud(DeltaSeconds);
	MaybePlayLowBatteryWarning();
	UpdateFootsteps(DeltaSeconds);
}

void ASubject14FirstPersonCharacter::MaybeUpdateBatteryHud(const float DeltaSeconds)
{
	if (BatteryHudWidget)
	{
		return;
	}

	if (!bShowFlashlightBatteryDebugHud || !GEngine)
	{
		return;
	}

	BatteryHudAccumSec += DeltaSeconds;
	if (BatteryHudAccumSec < 0.2f)
	{
		return;
	}
	BatteryHudAccumSec = 0.0f;

	const int32 Key = 99141;
	if (bFlashlightInfiniteBattery)
	{
		GEngine->AddOnScreenDebugMessage(Key, 0.25f, FColor::Cyan, TEXT("Flashlight: infinite"));
		return;
	}

	const int32 Pct = FMath::RoundToInt(FlashlightBattery01 * 100.0f);
	FColor Color = FColor::Green;
	if (FlashlightBattery01 < 0.1f)
	{
		Color = FColor::Red;
	}
	else if (FlashlightBattery01 < 0.25f)
	{
		Color = FColor::Yellow;
	}

	GEngine->AddOnScreenDebugMessage(
		Key,
		0.25f,
		Color,
		FString::Printf(TEXT("Flashlight battery: %d%%"), Pct));
}

void ASubject14FirstPersonCharacter::AddFlashlightBattery(const float Delta01)
{
	FlashlightBattery01 = FMath::Clamp(FlashlightBattery01 + Delta01, 0.0f, 1.0f);
}

void ASubject14FirstPersonCharacter::MaybePlayLowBatteryWarning()
{
	if (!IsLocallyControlled() || !LowBatterySound || bFlashlightInfiniteBattery)
	{
		return;
	}

	if (FlashlightBattery01 > 0.22f)
	{
		bLowBatteryChirpArmed = true;
		return;
	}

	if (!bFlashlightOn || !bLowBatteryChirpArmed)
	{
		return;
	}

	if (FlashlightBattery01 > 0.14f)
	{
		return;
	}

	UGameplayStatics::PlaySound2D(this, LowBatterySound);
	bLowBatteryChirpArmed = false;
}

void ASubject14FirstPersonCharacter::UpdateFootsteps(const float DeltaSeconds)
{
	USoundBase* StepSound = nullptr;
	if (FootstepSounds.Num() > 0)
	{
		StepSound = FootstepSounds[FMath::RandRange(0, FootstepSounds.Num() - 1)];
	}
	else
	{
		StepSound = FootstepSound;
	}

	if (!bEnableFootsteps || !StepSound || !IsLocallyControlled())
	{
		return;
	}

	UCharacterMovementComponent* const Move = GetCharacterMovement();
	if (!Move || !Move->IsMovingOnGround())
	{
		FootstepAccumulator = 0.0f;
		return;
	}

	const float Speed = Move->Velocity.Size2D();
	if (Speed < 25.0f)
	{
		FootstepAccumulator = 0.0f;
		return;
	}

	const float RefSpeed = FMath::Max(Move->MaxWalkSpeed, 1.0f);
	const float SpeedNorm = FMath::Clamp(Speed / RefSpeed, 0.35f, 1.35f);
	const float Interval = FMath::Clamp(
		FootstepBaseIntervalSeconds / SpeedNorm,
		FootstepMinIntervalSeconds,
		1.15f);

	FootstepAccumulator += DeltaSeconds;
	if (FootstepAccumulator < Interval)
	{
		return;
	}

	FootstepAccumulator = 0.0f;

	const float HalfHeight = GetCapsuleComponent()
		? GetCapsuleComponent()->GetScaledCapsuleHalfHeight()
		: 88.0f;
	const FVector FootLocation = GetActorLocation() - FVector(0.0f, 0.0f, HalfHeight);

	UGameplayStatics::PlaySoundAtLocation(
		this,
		StepSound,
		FootLocation,
		FootstepVolumeMultiplier,
		FMath::FRandRange(0.92f, 1.08f));
}

void ASubject14FirstPersonCharacter::TryStartAmbientBed()
{
	if (!bEnableAmbientBed || !AmbientBedSound || !AmbientBedAudio || !IsLocallyControlled())
	{
		return;
	}

	if (AmbientBedAudio->IsPlaying())
	{
		return;
	}

	AmbientBedAudio->SetSound(AmbientBedSound);
	RefreshAmbientBedVolume();
	// Looping is defined on the asset (enable “Looping” on a Sound Wave, or use a looping Sound Cue).
	AmbientBedAudio->Play();
}

void ASubject14FirstPersonCharacter::StopAmbientBed()
{
	if (!AmbientBedAudio)
	{
		return;
	}

	if (AmbientBedAudio->IsPlaying())
	{
		AmbientBedAudio->Stop();
	}
	AmbientBedAudio->SetSound(nullptr);
}

void ASubject14FirstPersonCharacter::RefreshAmbientBedVolume()
{
	if (!AmbientBedAudio)
	{
		return;
	}

	AmbientBedAudio->SetVolumeMultiplier(AmbientBedVolume * AmbientBedVolumeScalar);
}

void ASubject14FirstPersonCharacter::SetAmbientBedVolumeScalar(const float Scalar)
{
	AmbientBedVolumeScalar = FMath::Clamp(Scalar, 0.0f, 3.0f);
	RefreshAmbientBedVolume();
}

void ASubject14FirstPersonCharacter::SetAmbientBedPaused(const bool bPaused)
{
	if (!AmbientBedAudio)
	{
		return;
	}

	AmbientBedAudio->SetPaused(bPaused);
}

void ASubject14FirstPersonCharacter::SetAmbientBedSoundAndRestart(USoundBase* const NewBedSound)
{
	if (NewBedSound)
	{
		AmbientBedSound = NewBedSound;
		if (USoundWave* const Wave = Cast<USoundWave>(NewBedSound))
		{
			Wave->bLooping = true;
		}
	}

	if (!AmbientBedAudio)
	{
		return;
	}

	const bool bShouldPlay = bEnableAmbientBed && AmbientBedSound && IsLocallyControlled();
	if (AmbientBedAudio->IsPlaying())
	{
		AmbientBedAudio->Stop();
	}
	AmbientBedAudio->SetSound(nullptr);

	if (bShouldPlay)
	{
		TryStartAmbientBed();
	}
}

void ASubject14FirstPersonCharacter::FlickerTick()
{
	if (!Flashlight || !GetWorld())
	{
		return;
	}

	if (FlickerStepsRemaining <= 0)
	{
		Flashlight->SetVisibility(bFlickerRestoreVisibility);
		GetWorld()->GetTimerManager().ClearTimer(FlickerTimerHandle);
		return;
	}

	Flashlight->SetVisibility(!Flashlight->IsVisible());
	FlickerStepsRemaining--;
}

void ASubject14FirstPersonCharacter::RunFlashlightFlickerRoutine(const int32 ToggleCount, const float StepSeconds)
{
	if (!Flashlight || !GetWorld() || ToggleCount <= 0)
	{
		return;
	}

	GetWorld()->GetTimerManager().ClearTimer(FlickerTimerHandle);

	FlickerStepsRemaining = ToggleCount;
	bFlickerRestoreVisibility = Flashlight->IsVisible();

	GetWorld()->GetTimerManager().SetTimer(
		FlickerTimerHandle,
		this,
		&ASubject14FirstPersonCharacter::FlickerTick,
		FMath::Clamp(StepSeconds, 0.015f, 0.25f),
		true);
}

void ASubject14FirstPersonCharacter::SetNightSequenceInputBlocked(const bool bBlocked)
{
	bNightSequenceInputBlocked = bBlocked;
}

void ASubject14FirstPersonCharacter::MoveForward(float Value)
{
	if (bNightSequenceInputBlocked)
	{
		return;
	}
	if (Controller && FMath::Abs(Value) > KINDA_SMALL_NUMBER)
	{
		const FRotator YawRotation(0.0f, Controller->GetControlRotation().Yaw, 0.0f);
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(Direction, Value);
	}
}

void ASubject14FirstPersonCharacter::MoveRight(float Value)
{
	if (bNightSequenceInputBlocked)
	{
		return;
	}
	if (Controller && FMath::Abs(Value) > KINDA_SMALL_NUMBER)
	{
		const FRotator YawRotation(0.0f, Controller->GetControlRotation().Yaw, 0.0f);
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		AddMovementInput(Direction, Value);
	}
}

void ASubject14FirstPersonCharacter::Turn(float Value)
{
	if (bNightSequenceInputBlocked)
	{
		return;
	}
	AddControllerYawInput(Value);
}

void ASubject14FirstPersonCharacter::LookUp(float Value)
{
	if (bNightSequenceInputBlocked)
	{
		return;
	}
	AddControllerPitchInput(Value);
}

void ASubject14FirstPersonCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis(TEXT("MoveForward"), this, &ASubject14FirstPersonCharacter::MoveForward);
	PlayerInputComponent->BindAxis(TEXT("MoveRight"), this, &ASubject14FirstPersonCharacter::MoveRight);
	PlayerInputComponent->BindAxis(TEXT("Turn"), this, &ASubject14FirstPersonCharacter::Turn);
	PlayerInputComponent->BindAxis(TEXT("LookUp"), this, &ASubject14FirstPersonCharacter::LookUp);
	PlayerInputComponent->BindAction(TEXT("Jump"), IE_Pressed, this, &ASubject14FirstPersonCharacter::JumpPressed);
	PlayerInputComponent->BindAction(TEXT("Jump"), IE_Released, this, &ASubject14FirstPersonCharacter::JumpReleased);
	PlayerInputComponent->BindAction(TEXT("Interact"), IE_Pressed, this, &ASubject14FirstPersonCharacter::TryInteract);
	PlayerInputComponent->BindAction(TEXT("Flashlight"), IE_Pressed, this, &ASubject14FirstPersonCharacter::ToggleFlashlight);
}

void ASubject14FirstPersonCharacter::JumpPressed()
{
	if (bNightSequenceInputBlocked)
	{
		return;
	}
	Jump();
}

void ASubject14FirstPersonCharacter::JumpReleased()
{
	if (bNightSequenceInputBlocked)
	{
		return;
	}
	StopJumping();
}

void ASubject14FirstPersonCharacter::ToggleFlashlight()
{
	if (bNightSequenceInputBlocked)
	{
		return;
	}

	if (!Flashlight)
	{
		return;
	}

	const bool bWasOn = bFlashlightOn;

	if (!bWasOn)
	{
		if (!bFlashlightInfiniteBattery && FlashlightBattery01 <= KINDA_SMALL_NUMBER)
		{
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(
					-1,
					1.2f,
					FColor::Orange,
					TEXT("Flashlight: dead battery"));
			}
			return;
		}

		bFlashlightOn = true;
		Flashlight->SetVisibility(true);
		if (FlashlightOnSound && IsLocallyControlled())
		{
			UGameplayStatics::PlaySound2D(this, FlashlightOnSound);
		}
		return;
	}

	bFlashlightOn = false;
	Flashlight->SetVisibility(false);
	if (FlashlightOffSound && IsLocallyControlled())
	{
		UGameplayStatics::PlaySound2D(this, FlashlightOffSound);
	}
}

void ASubject14FirstPersonCharacter::TryInteract()
{
	if (bNightSequenceInputBlocked)
	{
		return;
	}

	if (!FirstPersonCamera)
	{
		return;
	}

	const FVector Start = FirstPersonCamera->GetComponentLocation();
	const FVector End = Start + FirstPersonCamera->GetForwardVector() * InteractDistance;

	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(Subject14Interact), false, this);
	Params.bReturnPhysicalMaterial = false;

	if (!GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
	{
		UE_LOG(LogTemp, Log, TEXT("Subject14 TryInteract: trace miss (visibility channel, %.0f uu)"), InteractDistance);
		return;
	}

	AActor* const HitActor = Hit.GetActor();
	if (!HitActor || !HitActor->GetClass()->ImplementsInterface(UInteractable::StaticClass()))
	{
		UE_LOG(
			LogTemp,
			Log,
			TEXT("Subject14 TryInteract: hit '%s' but it does not implement IInteractable"),
			HitActor ? *HitActor->GetName() : TEXT("(null)"));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("Subject14 TryInteract: calling IInteractable on '%s'"), *HitActor->GetName());
#if !UE_BUILD_SHIPPING
	DrawDebugLine(GetWorld(), Start, Hit.ImpactPoint, FColor::Green, false, 1.5f, 0, 1.5f);
#endif
	IInteractable::Execute_Subject14Interact(HitActor, this);
}
