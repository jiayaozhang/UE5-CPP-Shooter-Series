// Fill out your copyright notice in the Description page of Project Settings.


#include "ShooterCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"
#include "Engine/SkeletalMeshSocket.h"
#include "DrawDebugHelpers.h"
#include "Particles/ParticleSystemComponent.h"
#include "Components/WidgetComponent.h"
#include "Item.h"
#include "Weapon.h"
#include "Components/CapsuleComponent.h"
#include "Ammo.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "BulletHitInterface.h"
#include "Shooter.h"

// Sets default values
AShooterCharacter::AShooterCharacter() : 
	BaseTurnRate(DEFAULT_BASE_TURN_RATE),
	BaseLookUpRate(DEFAULT_BASE_LOOK_UP_RATE),
	
	// Turn rates when aiming or not aiming
	HipTurnRate(90.f),
	HipLookUpRate(90.f),
	AimingTurnRate(20.f),
	AimingLookUpRate(20.f),
	
	// Mouse sensitivity scale factors
	MouseHipTurnRate(1.f),
	MouseHipLookUpRate(1.f),
	MouseAimingTurnRate(0.6f),
	MouseAimingLookUpRate(0.6f),

	bAiming(false),
	
	// Camera field of view values
	CameraDefaultFieldOfView(0.f),
	CameraZoomedFieldOfView(AIMING_ZOOM_FOV),
	CameraCurrentFieldOfView(0.f),
	ZoomInterpolationSpeed(CAMERA_ZOOM_INTERPOLATION_SPEED),

	// Crosshair spread factors
	CrosshairSpreadMultiplier(0.f),
	CrosshairVelocityFactor(0.f),
	CrosshairInAirFactor(0.f),
	CrosshairAimFactor(0.f),
	CrosshairShootingFactor(0.f),

	// Automatic gunfire configuration
	bFireButtonPressed(false),

	// Item tracing
	bShouldTraceForOverlappingItems(false),

	// Combat
	CombatState(ECombatState::ECS_Ready),

	// Camera interp location
	CameraInterpDistance(250.f),
	CameraInterpElevation(65.f),

	// Ammo
	Starting9mmAmmoAmount(85),
	StartingARAmmoAmount(120),

	bCrouching(false),

	BaseMovementSpeed(650.f),
	CrouchMovementSpeed(300.f),

	StandingCapsuleHalfHeight(88.f),
	CrouchingCapsuleHalfHeight(44.f),

	BaseGroundFriction(2.f),
	CrouchingGroundFriction(100.f),

	bAimingButtonPressed(false),

    bShouldPlayPickupSound(true),
    bShouldPlayEquipSound(true),
    PickupSoundResetTime(0.2f),
    EquipSoundResetTime(0.2),

    HighlightedSlot(-1)
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Create camera spring arm, which pulls in towards character if there is a collision 
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("Camera Boom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = CAMERA_BOOM_ARM_LENGTH;	// Follow character at this distance
	CameraBoom->bUsePawnControlRotation = true;	// Rotate the spring arm based on the character controller
	CameraBoom->SocketOffset = FVector(0.f, 50.f, 70.f);

	// Create follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("Follow Camera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);	// Attach camera to end of boom
	FollowCamera->bUsePawnControlRotation = false;	// Camera does not rotate relative to spring arm

	// Don't rotate when the controller rotates
	// because Controller should only affect camera
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;
	// we should rotate along with the controller
	bUseControllerRotationYaw = true;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = false;	// true => character moves in the direction of input
	GetCharacterMovement()->RotationRate = FRotator(0.f, 540.f, 0.f);	// ... at the rotation rate here
	GetCharacterMovement()->JumpZVelocity = 600.f;
	GetCharacterMovement()->AirControl = 0.2f;

	// Create Hand Scene Component. Don't attach since it will be handled during reloading
	HandSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Hand Scene Component"));

    WeaponInterpComp = CreateDefaultSubobject<USceneComponent>(TEXT("WeaponInterpolationComponent"));
    WeaponInterpComp->SetupAttachment(GetFollowCamera());

    InterpComp1 = CreateDefaultSubobject<USceneComponent>(TEXT("InterpolationComponent1"));
    InterpComp1->SetupAttachment(GetFollowCamera());
    
    InterpComp2 = CreateDefaultSubobject<USceneComponent>(TEXT("InterpolationComponent2"));
    InterpComp2->SetupAttachment(GetFollowCamera());
    
    InterpComp3 = CreateDefaultSubobject<USceneComponent>(TEXT("InterpolationComponent3"));
    InterpComp3->SetupAttachment(GetFollowCamera());
    
    InterpComp4 = CreateDefaultSubobject<USceneComponent>(TEXT("InterpolationComponent4"));
    InterpComp4->SetupAttachment(GetFollowCamera());
    
    InterpComp5 = CreateDefaultSubobject<USceneComponent>(TEXT("InterpolationComponent5"));
    InterpComp5->SetupAttachment(GetFollowCamera());
    
    InterpComp6 = CreateDefaultSubobject<USceneComponent>(TEXT("InterpolationComponent6"));
    InterpComp6->SetupAttachment(GetFollowCamera());
}

// Called when the game starts or when spawned
void AShooterCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	if (FollowCamera)
	{
		CameraDefaultFieldOfView = GetFollowCamera()->FieldOfView;
		CameraCurrentFieldOfView = CameraDefaultFieldOfView;
	}

	EquipWeapon(SpawnDefaultWeapon());
    EquippedWeapon->DisableCustomDepth();
    EquippedWeapon->DisableGlowMaterial();
    EquippedWeapon->SetSlotIndex(0);
    EquippedWeapon->SetCharacter(this);
    Inventory.Add(EquippedWeapon);

	InitializeAmmoMap();

	GetCharacterMovement()->MaxWalkSpeed = BaseMovementSpeed;

    InitializeInterpLocations();
}

// Called every frame
void AShooterCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	InterpolateAimCameraZoom(DeltaTime);

	RefreshAimingOrHipLookRates();

	CalculateCrosshairSpread(DeltaTime);

	TraceForOverlappingItems();

	InterpCapsuleHalfHeight(DeltaTime);
}


void AShooterCharacter::InterpolateAimCameraZoom(float DeltaTime)
{
	if (IsAiming())
	{
		CameraCurrentFieldOfView = FMath::FInterpTo(
			CameraCurrentFieldOfView, 
			CameraZoomedFieldOfView, 
			DeltaTime, 
			ZoomInterpolationSpeed);
	}
	else
	{
		CameraCurrentFieldOfView = FMath::FInterpTo(
			CameraCurrentFieldOfView, 
			CameraDefaultFieldOfView, 
			DeltaTime, 
			ZoomInterpolationSpeed);
	}

	GetFollowCamera()->SetFieldOfView(CameraCurrentFieldOfView);
}

void AShooterCharacter::RefreshAimingOrHipLookRates()
{
	if (IsAiming())
	{
		BaseTurnRate = AimingTurnRate;
		BaseLookUpRate = AimingLookUpRate;
	}
	else
	{
		BaseTurnRate = HipTurnRate;
		BaseLookUpRate = HipLookUpRate;
	}
}

void AShooterCharacter::MoveForward(float Value)
{
	if (Controller == nullptr || Value == 0)
	{
		return;
	}

	// TODO: This is too complicated, as it can simply be AddMovementInput(GetActorForwardVector(), Value);

	// Determine forward direction
	const FRotator Rotation{ Controller->GetControlRotation() };
	const FRotator YawRotation{0, Rotation.Yaw, 0};
	const FVector Direction{ FRotationMatrix{YawRotation}.GetUnitAxis(EAxis::X) };

	AddMovementInput(Direction, Value);
}

void AShooterCharacter::MoveRight(float Value)
{
	if (Controller == nullptr || Value == 0)
	{
		return;
	}

	// TODO: This is too complicated, as it can simply be AddMovementInput(GetActorRightVector(), Value);

	// Determine right-side direction
	const FRotator Rotation{ Controller->GetControlRotation() };
	const FRotator YawRotation{0, Rotation.Yaw, 0};
	const FVector Direction{ FRotationMatrix{YawRotation}.GetUnitAxis(EAxis::Y) };

	AddMovementInput(Direction, Value);
}

// Called to bind functionality to input
void AShooterCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// Assertion: Use "check()" macro to halt execution if PlayerInputComponent is invalid
	check(PlayerInputComponent);

	PlayerInputComponent->BindAxis("MoveForward", this, &AShooterCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AShooterCharacter::MoveRight);
	PlayerInputComponent->BindAxis("TurnRate", this, &AShooterCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUpRate", this, &AShooterCharacter::LookUpAtRate);

	PlayerInputComponent->BindAxis("Turn", this, &AShooterCharacter::TurnWithMouse);
	PlayerInputComponent->BindAxis("LookUp", this, &AShooterCharacter::LookUpWithMouse);

	PlayerInputComponent->BindAction("Jump", EInputEvent::IE_Pressed, this, &AShooterCharacter::Jump);
	PlayerInputComponent->BindAction("Jump", EInputEvent::IE_Released, this, &ACharacter::StopJumping);

	PlayerInputComponent->BindAction("FireButton", EInputEvent::IE_Pressed, this, &AShooterCharacter::FireButtonPressed);
	PlayerInputComponent->BindAction("FireButton", EInputEvent::IE_Released, this, &AShooterCharacter::FireButtonReleased);

	PlayerInputComponent->BindAction("AimingButton", EInputEvent::IE_Pressed, this, &AShooterCharacter::AimingButtonPressed);
	PlayerInputComponent->BindAction("AimingButton", EInputEvent::IE_Released, this, &AShooterCharacter::AimingButtonReleased);

	PlayerInputComponent->BindAction("SelectButton", EInputEvent::IE_Pressed, this, &AShooterCharacter::SelectButtonPressed);
	PlayerInputComponent->BindAction("SelectButton", EInputEvent::IE_Released, this, &AShooterCharacter::SelectButtonReleased);

	PlayerInputComponent->BindAction("ReloadButton", EInputEvent::IE_Pressed, this, &AShooterCharacter::ReloadButtonPressed);

	PlayerInputComponent->BindAction("Crouch", EInputEvent::IE_Pressed, this, &AShooterCharacter::CrouchButtonPressed);

	PlayerInputComponent->BindAction("FKey", EInputEvent::IE_Pressed, this, &AShooterCharacter::FKeyPressed);
	PlayerInputComponent->BindAction("1Key", EInputEvent::IE_Pressed, this, &AShooterCharacter::OneKeyPressed);
	PlayerInputComponent->BindAction("2Key", EInputEvent::IE_Pressed, this, &AShooterCharacter::TwoKeyPressed);
	PlayerInputComponent->BindAction("3Key", EInputEvent::IE_Pressed, this, &AShooterCharacter::ThreeKeyPressed);
	PlayerInputComponent->BindAction("4Key", EInputEvent::IE_Pressed, this, &AShooterCharacter::FourKeyPressed);
	PlayerInputComponent->BindAction("5Key", EInputEvent::IE_Pressed, this, &AShooterCharacter::FiveKeyPressed);
}

void AShooterCharacter::TurnWithMouse(float Value)
{
	const float MouseTurnSensitivity = IsAiming() ? MouseAimingTurnRate : MouseHipTurnRate;

	AddControllerYawInput(Value * MouseTurnSensitivity);
}

void AShooterCharacter::LookUpWithMouse(float Value)
{
	const float MouseLookUpSensitivity = IsAiming() ? MouseAimingLookUpRate : MouseHipLookUpRate;
	
	AddControllerPitchInput(Value * MouseLookUpSensitivity);
}

void AShooterCharacter::TurnAtRate(float Rate)
{
	// base turn rate (deg/sec)  * delta time (sec/frame)  => degrees per frame
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void AShooterCharacter::LookUpAtRate(float Rate)
{
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

void AShooterCharacter::Jump()
{
	if (bCrouching)
	{
		bCrouching = false;
		GetCharacterMovement()->MaxWalkSpeed = BaseMovementSpeed;
	}
	else
	{
		Super::Jump();
	}
}

void AShooterCharacter::FireWeapon()
{
	if (!EquippedWeapon)
	{
		return;
	}

	if (CombatState != ECombatState::ECS_Ready)
	{
		return;
	}

	if (WeaponHasAmmo())
	{
		PlayFireSound();

		SendBullet();

		PlayGunFireMontage();

		EquippedWeapon->DecrementAmmo();

		StartFireTimer();

        if (EquippedWeapon->GetWeaponType() == EWeaponType::EWT_Pistol)
        {
            EquippedWeapon->StartSlideTimer();
        }
	}
}

void AShooterCharacter::PlayFireSound()
{
	if (EquippedWeapon->GetFireSound())
	{
		UGameplayStatics::PlaySound2D(this, EquippedWeapon->GetFireSound());
	}
}

void AShooterCharacter::SendBullet()
{
	FTransform BarrelSocketTransform;
	if (!EquippedWeapon->TryGetBarrelSocketTransform(BarrelSocketTransform))
	{
		return;
	}

	if (EquippedWeapon->GetMuzzleFlash())
	{
		UGameplayStatics::SpawnEmitterAtLocation(
            GetWorld(), 
            EquippedWeapon->GetMuzzleFlash(), 
            BarrelSocketTransform);
	}

    FHitResult BeamHitResult;
	if (GetBeamEndLocation(BarrelSocketTransform.GetLocation(), BeamHitResult))
	{
        // Does hit actor implement BulletHitInterface?
        if (BeamHitResult.GetActor())
        {
            IBulletHitInterface* BulletHitInterface = Cast<IBulletHitInterface>(BeamHitResult.GetActor());
            if (BulletHitInterface)
            {
                BulletHitInterface->BulletHit_Implementation(BeamHitResult);
            }
        }
        else
        {
            // Spawn default impact particles 
            if (ImpactParticles)
            {
                UGameplayStatics::SpawnEmitterAtLocation(
                    GetWorld(), 
                    ImpactParticles, 
                    BeamHitResult.Location);
            }
        }
	}

	if (BeamParticles)
	{
		UParticleSystemComponent* Beam = UGameplayStatics::SpawnEmitterAtLocation(
			GetWorld(), 
			BeamParticles, 
			BarrelSocketTransform);

		if (Beam)
		{
			Beam->SetVectorParameter(FName("Target"), BeamHitResult.Location);
		}
	}
}

void AShooterCharacter::PlayGunFireMontage()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && HipFireMontage)
	{
		AnimInstance->Montage_Play(HipFireMontage);
		AnimInstance->Montage_JumpToSection(FName("StartFire"));
	}
}

void AShooterCharacter::StartFireTimer()
{
    if (!EquippedWeapon)
    {
        return;
    }

	CombatState = ECombatState::ECS_FiringInProgress;

	GetWorldTimerManager().SetTimer(
		AutoFireTimer,
		this,
		&AShooterCharacter::ResetAutoFire,
		EquippedWeapon->GetAutoFireRate()
    );
}

void AShooterCharacter::ResetAutoFire()
{
	CombatState = ECombatState::ECS_Ready;

    if (!EquippedWeapon)
    {
        return;
    }

	if (WeaponHasAmmo())
	{
		if (bFireButtonPressed && EquippedWeapon->GetAutomatic())
		{
			FireWeapon();
		}
	}
	else
	{
		ReloadWeapon();
	}
}

void AShooterCharacter::SelectButtonPressed()
{
    if (CombatState != ECombatState::ECS_Ready)
    {
        return;
    }

	if (TraceHitItem)
	{
		TraceHitItem->StartItemCurve(this, true);
        // Prevent spamming of item selection while it's interping
        TraceHitItem = nullptr;
	}
}

void AShooterCharacter::SelectButtonReleased()
{

}

void AShooterCharacter::ReloadButtonPressed()
{
	ReloadWeapon();
}

void AShooterCharacter::ReloadWeapon()
{
	if (CombatState != ECombatState::ECS_Ready || EquippedWeapon == nullptr)
	{
		return;
	}

	if (IsCarryingAmmo() && !EquippedWeapon->IsClipFull())
	{
		if (bAiming)
		{
			StopAiming();
		}

		CombatState = ECombatState::ECS_Reloading;

		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
		if (AnimInstance && ReloadAnimMontage)
		{
			AnimInstance->Montage_Play(ReloadAnimMontage);
			AnimInstance->Montage_JumpToSection(EquippedWeapon->GetReloadMontageSectionName());
		}
	}
}


bool AShooterCharacter::GetBeamEndLocation(const FVector& MuzzleSocketLocation, FHitResult& OutHitResult)
{
    FVector OutBeamEndLocation;
	FHitResult CrosshairHitResult;

	if (TryGetTraceUnderCrosshairs(CrosshairHitResult, OutBeamEndLocation))
	{
		OutBeamEndLocation = CrosshairHitResult.Location;
	}

	// Peform a second trace from gun barrel in case there are 
	// objects between gun barrel and impact point
	const FVector WeaponTraceStart{ MuzzleSocketLocation };
	// Extend second line trace farther by 25%, 
	// taking into account last beam end location and start of muzzle socket
	const FVector StartToEnd{ OutBeamEndLocation - MuzzleSocketLocation };
	const FVector WeaponTraceEnd{ MuzzleSocketLocation + StartToEnd * 1.25f };	

	GetWorld()->LineTraceSingleByChannel(
		OutHitResult, 
		WeaponTraceStart, 
		WeaponTraceEnd, 
		ECollisionChannel::ECC_Visibility);
	if (!OutHitResult.bBlockingHit)
	{
		OutHitResult.Location = OutBeamEndLocation;
        return false;
	}

	return true;
}

void AShooterCharacter::AimingButtonPressed()
{
	bAimingButtonPressed = true;

	if (CombatState != ECombatState::ECS_Reloading && CombatState != ECombatState::ECS_Equipping)
	{
		Aim();
	}
}

void AShooterCharacter::AimingButtonReleased()
{
	bAimingButtonPressed = false;

	StopAiming();
}


void AShooterCharacter::CalculateCrosshairSpread(float DeltaTime)
{
	FVector2D WalkSpeedRange{0.f, 600.f};
	FVector2D VelocityMultiplierRange{ 0.f, 1.f };
	FVector Velocity{ GetVelocity() };
	Velocity.Z = 0;

	CrosshairVelocityFactor = FMath::GetMappedRangeValueClamped(
		WalkSpeedRange, 
		VelocityMultiplierRange, 
		Velocity.Size());

	if (GetCharacterMovement()->IsFalling())
	{
		// Spread crosshairs slowly while in air
		CrosshairInAirFactor = FMath::FInterpTo(
			CrosshairInAirFactor, 
			2.5f, 
			DeltaTime, 
			2.25f);
	}
	else
	{
		// Shrink crosshairs rapidly once on ground
		CrosshairInAirFactor = FMath::FInterpTo(
			CrosshairInAirFactor, 
			0.f, 
			DeltaTime, 
			30.f);
	}

	if (IsAiming())
	{
		// Crosshairs should shrink in quickly by a small amount
		CrosshairAimFactor = FMath::FInterpTo(
			CrosshairAimFactor,
			0.6f,
			DeltaTime,
			30.f);
	}
	else
	{
		// Crosshairs should quickly return to normal when we're not aiming
		CrosshairAimFactor = FMath::FInterpTo(
			CrosshairAimFactor,
			0.f,
			DeltaTime,
			30.f);
	}

	if (CombatState == ECombatState::ECS_FiringInProgress)
	{
		CrosshairShootingFactor = FMath::FInterpTo(
			CrosshairShootingFactor,
			0.3f,
			DeltaTime,
			60.f);
	}
	else
	{
		CrosshairShootingFactor = FMath::FInterpTo(
			CrosshairShootingFactor,
			0.f,
			DeltaTime,
			60.f);
	}

	CrosshairSpreadMultiplier = 0.5f 
		+ CrosshairVelocityFactor 
		+ CrosshairInAirFactor
		+ CrosshairShootingFactor
		- CrosshairAimFactor;
}

float AShooterCharacter::GetCrosshairSpreadMultiplier() const
{
	return CrosshairSpreadMultiplier;
}

void AShooterCharacter::FireButtonPressed()
{
	bFireButtonPressed = true;
	
	FireWeapon();
}

void AShooterCharacter::FireButtonReleased()
{
	bFireButtonPressed = false;
}

bool AShooterCharacter::TryGetTraceUnderCrosshairs(FHitResult& OutHitResult, FVector& OutTraceEndOrHitLocation)
{
	FVector2D ViewportSize;
	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->GetViewportSize(ViewportSize);
	}

	FVector2D CrosshairLocation(ViewportSize.X / 2.f, ViewportSize.Y / 2.f);

	// Get world position and direction of crosshair
	FVector CrosshairWorldPosition;
	FVector CrosshairWorldDirection;

	bool bScreenToWorldSuccess = UGameplayStatics::DeprojectScreenToWorld(
		UGameplayStatics::GetPlayerController(this, 0),
		CrosshairLocation,
		CrosshairWorldPosition,
		CrosshairWorldDirection);

	if (bScreenToWorldSuccess)
	{
		// trace crosshair world location outward
		const FVector Start{ CrosshairWorldPosition };
		const FVector End{ Start + CrosshairWorldDirection * MAX_LINE_TRACE_DISTANCE };

		OutTraceEndOrHitLocation = End;
		
		GetWorld()->LineTraceSingleByChannel(
			OutHitResult, 
			Start, 
			End, 
			ECollisionChannel::ECC_Visibility);

		if (OutHitResult.bBlockingHit)
		{
			OutTraceEndOrHitLocation = OutHitResult.Location;

			return true;
		}
	}

	return false;
}

void AShooterCharacter::AddOverlappedItemCount(int32 Amount)
{
	// TODO: probably should extract bShouldTraceForItems from here but copied as is from course
	if (OverlappedItemCount + Amount <= 0)
	{
		OverlappedItemCount = 0;
		bShouldTraceForOverlappingItems = false;
	}
	else
	{
		OverlappedItemCount += Amount;
		bShouldTraceForOverlappingItems = true;
	}
}

void AShooterCharacter::TraceForOverlappingItems()
{
	if (!bShouldTraceForOverlappingItems)
	{
		if (LastTracedPickupItem)
		{
			LastTracedPickupItem->GetPickupWidget()->SetVisibility(false);
            LastTracedPickupItem->DisableCustomDepth();
		}

		return;
	}

	FHitResult ItemTraceResult;
	FVector IgnoredTraceEndOrHitLocation;
	if (!TryGetTraceUnderCrosshairs(ItemTraceResult, IgnoredTraceEndOrHitLocation))
	{
		return;
	}

	TraceHitItem = Cast<AItem>(ItemTraceResult.GetActor());

    const auto TraceHitWeapon = Cast<AWeapon>(TraceHitItem);
    if (TraceHitWeapon)
    {
        if (HighlightedSlot == -1)
        {
            HighlightInventorySlot();
        }
    }
    else
    {
        if (HighlightedSlot != -1)
        {
            UnhighlightInventorySlot();
        }
    }

    if (TraceHitItem && TraceHitItem->GetItemState() == EItemState::EIS_EquipInterpolating)
    {
        // No need to track hit item if interping so we can prevent spamming
        TraceHitItem = nullptr;
    }

	if (TraceHitItem && TraceHitItem->GetPickupWidget())
	{
		TraceHitItem->GetPickupWidget()->SetVisibility(true);
        TraceHitItem->EnableCustomDepth();

        TraceHitItem->SetCharacterInventoryFull(Inventory.Num() >= INVENTORY_CAPACITY);
	}

	if (LastTracedPickupItem && LastTracedPickupItem != TraceHitItem)
	{
		LastTracedPickupItem->GetPickupWidget()->SetVisibility(false);
        LastTracedPickupItem->DisableCustomDepth();
	}

	LastTracedPickupItem = TraceHitItem;
}

void AShooterCharacter::InterpCapsuleHalfHeight(float DeltaTime)
{
	const float TargetCapsuleHalfHeight = (bCrouching) 
		? CrouchingCapsuleHalfHeight 
		: StandingCapsuleHalfHeight;

	const float InterpHalfHeight{ 
		FMath::FInterpTo(
			GetCapsuleComponent()->GetScaledCapsuleHalfHeight(), 
			TargetCapsuleHalfHeight, 
			DeltaTime, 
			20.f
		) 
	};

	// Delta Capsule Half Height: Negative value if crouching; positive value if standing
	const float DeltaCapsuleHalfHeight{ InterpHalfHeight - GetCapsuleComponent()->GetScaledCapsuleHalfHeight() };
	const FVector MeshOffset{ 0.f, 0.f, -DeltaCapsuleHalfHeight };
	GetMesh()->AddLocalOffset(MeshOffset);

	GetCapsuleComponent()->SetCapsuleHalfHeight(InterpHalfHeight);
}


AWeapon* AShooterCharacter::SpawnDefaultWeapon()
{
	if (DefaultWeaponClass)
	{
		return GetWorld()->SpawnActor<AWeapon>(DefaultWeaponClass);
	}

	return nullptr;
}

void AShooterCharacter::EquipWeapon(AWeapon* WeaponToEquip, bool bSwapping)
{
	if (!WeaponToEquip)
	{
		return;
	}

	const USkeletalMeshSocket* HandWeaponSocket = GetMesh()->GetSocketByName(FName("RightHandSocket"));
	if (HandWeaponSocket)
	{
		HandWeaponSocket->AttachActor(WeaponToEquip, GetMesh());
	}

    if (EquippedWeapon == nullptr)
    {
        // -1 == no equipped weapon yet. No need to reverse icon animation
        EquipItemDelegate.Broadcast(-1, WeaponToEquip->GetSlotIndex());
    }
    else if (!bSwapping)
    {
        EquipItemDelegate.Broadcast(EquippedWeapon->GetSlotIndex(), WeaponToEquip->GetSlotIndex());
    }

    // Set equipped weapon to newly spawned weapon
	EquippedWeapon = WeaponToEquip;
	EquippedWeapon->SetItemState(EItemState::EIS_Equipped);
}

void AShooterCharacter::DropWeapon()
{
	if (EquippedWeapon)
	{
		FDetachmentTransformRules DetachmentTransformRules(EDetachmentRule::KeepWorld, true);
		EquippedWeapon->GetItemMesh()->DetachFromComponent(DetachmentTransformRules);

		EquippedWeapon->SetItemState(EItemState::EIS_Falling);
		EquippedWeapon->ThrowWeapon();
	}
}

void AShooterCharacter::SwapWeapon(AWeapon* WeaponToSwap)
{
    if (Inventory.Num() - 1 >= EquippedWeapon->GetSlotIndex())
    {
        Inventory[EquippedWeapon->GetSlotIndex()] = WeaponToSwap;
        WeaponToSwap->SetSlotIndex(EquippedWeapon->GetSlotIndex());
    }

	DropWeapon();
	EquipWeapon(WeaponToSwap, true);
    TraceHitItem = nullptr;
	LastTracedPickupItem = nullptr;
}

// TODO: no longer needed. AItem already has GetInterpLocation()
// FVector AShooterCharacter::GetCameraInterpLocation()
// {
// 	const FVector CameraWorldLocation{ FollowCamera->GetComponentLocation() };
// 	const FVector CameraForward{ FollowCamera->GetForwardVector() };

// 	// Desired = cameraWorldLocation + forward * A + Up * B;
// 	return CameraWorldLocation 
// 		+ CameraForward * CameraInterpDistance
// 		+ FVector(0.f, 0.f, CameraInterpElevation);
// }

void AShooterCharacter::LoadPickupItem(AItem* Item)
{
	if (!Item)
	{
		return;
	}

	Item->PlayEquipSound();

	auto Weapon = Cast<AWeapon>(Item);
	if (Weapon)
	{
        if (Inventory.Num() < INVENTORY_CAPACITY)
        {
            Weapon->SetSlotIndex(Inventory.Num());
            Inventory.Add(Weapon);
            Weapon->SetItemState(EItemState::EIS_PickedUp);
        }
        else
        {
            // Inventory is full. Swap with EquippedWeapon
		    SwapWeapon(Weapon);
        }
	}

	auto Ammo = Cast<AAmmo>(Item);
	if (Ammo)
	{
		PickUpAmmo(Ammo);
	}
}

void AShooterCharacter::PickUpAmmo(AAmmo* Ammo)
{
	if (!(Ammo && EquippedWeapon))
	{
		return;
	}

	if (AmmoMap.Find(Ammo->GetAmmoType()))
	{
		int32 AmmoCount{ AmmoMap[Ammo->GetAmmoType()] };
		AmmoCount += Ammo->GetItemCount();
		AmmoMap[Ammo->GetAmmoType()] = AmmoCount;
	}

	if (EquippedWeapon->GetAmmoType() == Ammo->GetAmmoType() 
			&& EquippedWeapon->GetAmmo() == 0)
	{
		ReloadWeapon();
	}

	Ammo->Destroy();
}

void AShooterCharacter::InitializeAmmoMap()
{
	AmmoMap.Add(EAmmoType::EAT_9mm, Starting9mmAmmoAmount);
	AmmoMap.Add(EAmmoType::EAT_AR, StartingARAmmoAmount);
}

bool AShooterCharacter::WeaponHasAmmo() const
{
	return EquippedWeapon ? EquippedWeapon->GetAmmo() > 0 : 0;
}

void AShooterCharacter::FinishReloading()
{
	CombatState = ECombatState::ECS_Ready;

	if (bAimingButtonPressed)
	{
		Aim();
	}

	if (EquippedWeapon == nullptr)
	{
		return;
	}

	const auto AmmoType = EquippedWeapon->GetAmmoType();
	if (AmmoMap.Contains(AmmoType))
	{
		int32 CarriedAmmo = AmmoMap[AmmoType];

		// Space left in the magazine of EquippedWeapon
		const auto MagazineEmptySpace = EquippedWeapon->GetMagazineCapacity() - EquippedWeapon->GetAmmo();
		if (MagazineEmptySpace > CarriedAmmo)
		{
			EquippedWeapon->ReloadAmmo(CarriedAmmo);
			CarriedAmmo = 0;
		}
		else
		{
			// Fill the magazine
			EquippedWeapon->ReloadAmmo(MagazineEmptySpace);
			CarriedAmmo -= MagazineEmptySpace;
		}

		AmmoMap.Add(AmmoType, CarriedAmmo);
	}
}

bool AShooterCharacter::IsCarryingAmmo()
{
	if (!EquippedWeapon)
	{
		return false;
	}

	auto AmmoType = EquippedWeapon->GetAmmoType();

	return AmmoMap.Contains(AmmoType) && AmmoMap[AmmoType] > 0;
}

void AShooterCharacter::GrabClip()
{
	if (!EquippedWeapon)
	{
		return;
	}

	if (!HandSceneComponent)
	{
		return;
	}

	int32 ClipBoneIndex = EquippedWeapon->GetItemMesh()->GetBoneIndex(EquippedWeapon->GetClipBoneName());
	ClipTransform = EquippedWeapon->GetItemMesh()->GetBoneTransform(ClipBoneIndex);

	FAttachmentTransformRules AttachmentRules(EAttachmentRule::KeepRelative, true);
	HandSceneComponent->AttachToComponent(GetMesh(), AttachmentRules, FName(TEXT("hand_l")));
	HandSceneComponent->SetWorldTransform(ClipTransform);

	EquippedWeapon->SetMovingClip(true);
}

void AShooterCharacter::ReleaseClip()
{
	if (EquippedWeapon)
	{
		EquippedWeapon->SetMovingClip(false);
	}
}

void AShooterCharacter::CrouchButtonPressed()
{
	if (!GetCharacterMovement()->IsFalling())
	{
		bCrouching = !bCrouching;
	}

	if (bCrouching)
	{
		GetCharacterMovement()->MaxWalkSpeed = CrouchMovementSpeed;
		GetCharacterMovement()->GroundFriction = CrouchingGroundFriction;
	}
	else
	{
		GetCharacterMovement()->MaxWalkSpeed = BaseMovementSpeed;
		GetCharacterMovement()->GroundFriction = BaseGroundFriction;
	}
}

void AShooterCharacter::Aim()
{
	bAiming = true;

	GetCharacterMovement()->MaxWalkSpeed = CrouchMovementSpeed;
}

void AShooterCharacter::StopAiming()
{
	bAiming = false;

	if (!bCrouching)
	{
		GetCharacterMovement()->MaxWalkSpeed = BaseMovementSpeed;
	}
}

FInterpLocation AShooterCharacter::GetInterpLocation(int32 Index)
{
    if (Index <= InterpLocations.Num())
    {
        return InterpLocations[Index];
    }

    return FInterpLocation();
}

void AShooterCharacter::InitializeInterpLocations()
{
    FInterpLocation WeaponLocation{ WeaponInterpComp, 0 };
    InterpLocations.Add(WeaponLocation);

    FInterpLocation InterpLoc1{ InterpComp1, 0};
    InterpLocations.Add(InterpLoc1);

    FInterpLocation InterpLoc2{ InterpComp2, 0};
    InterpLocations.Add(InterpLoc2);

    FInterpLocation InterpLoc3{ InterpComp3, 0};
    InterpLocations.Add(InterpLoc3);

    FInterpLocation InterpLoc4{ InterpComp4, 0};
    InterpLocations.Add(InterpLoc4);

    FInterpLocation InterpLoc5{ InterpComp5, 0};
    InterpLocations.Add(InterpLoc5);

    FInterpLocation InterpLoc6{ InterpComp6, 0};
    InterpLocations.Add(InterpLoc6);    
}

int32 AShooterCharacter::GetInterpLocationIndex()
{
    // Note: Zeroth element is the weapon interp location
    // so we handle only elements 1 through InterpLoc"N"

    int32 LowestIndex = 1;
    int32 LowestCount = INT_MAX;

    // Find the interp location that has the least number of items in it
    for (int32 i = 1; i < InterpLocations.Num(); i++)
    {
        if (InterpLocations[i].ItemCount < LowestCount)
        {
            LowestIndex = i;
            LowestCount = InterpLocations[i].ItemCount;
        }
    }

    return LowestIndex;
}

void AShooterCharacter::IncrementInterpLocationItemCount(int32 Index, int32 Amount)
{
    if (Amount < -1 || Amount > 1)
    {
        return;
    }

    if (InterpLocations.Num() >= Index)
    {
        InterpLocations[Index].ItemCount += Amount;
    }
}

 void AShooterCharacter::ResetPickupSoundTimer()
 {
    bShouldPlayPickupSound = true;
 }

void AShooterCharacter::ResetEquipSoundTimer()
{
    bShouldPlayEquipSound = true;
}

void AShooterCharacter::StartPickupSoundTimer()
{
    bShouldPlayPickupSound = false;
    GetWorldTimerManager().SetTimer(
        PickupSoundTimer, 
        this, 
        &AShooterCharacter::ResetPickupSoundTimer, 
        PickupSoundResetTime
    );
}

void AShooterCharacter::StartEquipSoundTimer()
{
    bShouldPlayEquipSound = false;
    GetWorldTimerManager().SetTimer(
        EquipSoundTimer,
        this,
        &AShooterCharacter::ResetEquipSoundTimer,
        EquipSoundResetTime
    );
}

void AShooterCharacter::FKeyPressed()
{
    if (EquippedWeapon->GetSlotIndex() == 0)
    {
        return;
    }

    ExchangeInventoryItems(EquippedWeapon->GetSlotIndex(), 0);
}

void AShooterCharacter::OneKeyPressed()
{
    if (EquippedWeapon->GetSlotIndex() == 1)
    {
        return;
    }

    ExchangeInventoryItems(EquippedWeapon->GetSlotIndex(), 1);
}

void AShooterCharacter::TwoKeyPressed()
{
    if (EquippedWeapon->GetSlotIndex() == 2)
    {
        return;
    }

    ExchangeInventoryItems(EquippedWeapon->GetSlotIndex(), 2);
}

void AShooterCharacter::ThreeKeyPressed()
{
    if (EquippedWeapon->GetSlotIndex() == 3)
    {
        return;
    }

    ExchangeInventoryItems(EquippedWeapon->GetSlotIndex(), 3);
}

void AShooterCharacter::FourKeyPressed()
{
    if (EquippedWeapon->GetSlotIndex() == 4)
    {
        return;
    }

    ExchangeInventoryItems(EquippedWeapon->GetSlotIndex(), 4);
}

void AShooterCharacter::FiveKeyPressed()
{
    if (EquippedWeapon->GetSlotIndex() == 5)
    {
        return;
    }

    ExchangeInventoryItems(EquippedWeapon->GetSlotIndex(), 5);
}

void AShooterCharacter::ExchangeInventoryItems(int32 CurrentItemIndex, int32 NewItemIndex)
{
    const bool bCanExchangeItems = CurrentItemIndex != NewItemIndex 
        && NewItemIndex < Inventory.Num() 
        && (CombatState == ECombatState::ECS_Ready || CombatState == ECombatState::ECS_Equipping);

    if (bCanExchangeItems)
    {
        if (bAiming)
        {
            StopAiming();
        }

        auto OldEquippedWeapon = EquippedWeapon;
        auto NewWeapon = Cast<AWeapon>(Inventory[NewItemIndex]);
        EquipWeapon(NewWeapon);

        OldEquippedWeapon->SetItemState(EItemState::EIS_PickedUp);
        NewWeapon->SetItemState(EItemState::EIS_Equipped);

        CombatState = ECombatState::ECS_Equipping;
        
        UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
        if (AnimInstance && EquipAnimMontage)
        {
            AnimInstance->Montage_Play(EquipAnimMontage, 1.f);
            AnimInstance->Montage_JumpToSection(FName("Equip"));
        }

        NewWeapon->PlayEquipSound(true);
    }
}

void AShooterCharacter::FinishEquipping()
{
    CombatState = ECombatState::ECS_Ready;

    if (bAimingButtonPressed)
    {
        Aim();
    }
}

int32 AShooterCharacter::GetEmptyInventorySlot()
{
    for (int32 i = 0; i < Inventory.Num(); i++)
    {
        if (Inventory[i] == nullptr)
        {
            return i;
        }
    }

    if (Inventory.Num() < INVENTORY_CAPACITY)
    {
        return Inventory.Num();
    }

    return -1;
}

void AShooterCharacter::HighlightInventorySlot()
{
    const int32 EmptySlotIndex{ GetEmptyInventorySlot() };
    HighlightIconDelegate.Broadcast(EmptySlotIndex, true);
    HighlightedSlot = EmptySlotIndex;
}

void AShooterCharacter::UnhighlightInventorySlot()
{
    HighlightIconDelegate.Broadcast(HighlightedSlot, false);
    HighlightedSlot = -1;
}

EPhysicalSurface AShooterCharacter::GetSurfaceType() const
{
    const FVector Start{ GetActorLocation() };
    const FVector End{ Start + FVector(0.f, 0.f, -400.f) };
    
    FCollisionQueryParams QueryParams;
    QueryParams.bReturnPhysicalMaterial = true;

    FHitResult HitResult;
    GetWorld()->LineTraceSingleByChannel(
        HitResult, 
        Start, 
        End, 
        ECollisionChannel::ECC_Visibility, 
        QueryParams
    );

    return UPhysicalMaterial::DetermineSurfaceType(HitResult.PhysMaterial.Get());
}
