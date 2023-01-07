// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AmmoType.h"
#include "ShooterCharacter.generated.h"

class AWeapon;

UENUM(BlueprintType)
enum class ECombatState : uint8
{
	ECS_Ready	 UMETA(DisplayName = "Ready"),
	ECS_FiringInProgress UMETA(DisplayName = "FiringInProgress"),
	ECS_Reloading UMETA(DisplayName = "Reloading"),
    ECS_Equipping UMETA(DisplayName = "Equipping"),

	ECS_MAX UMETA(DisplayName = "DefaultMAX")
};

USTRUCT(BlueprintType)
struct FInterpLocation
{
    GENERATED_BODY()

    /** Scene component to use for its location for interping */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    USceneComponent* SceneComponent;

    /** Number of items interping to/at this scene component location */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    int32 ItemCount;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FEquipItemDelegate, int32, CurrentSlotIndex, int32, NewSlotIndex);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FHighlightIconDelegate, int32, SlotIndex, bool, bStartAnimation);

UCLASS()
class SHOOTER_API AShooterCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AShooterCharacter();

private:
	const float DEFAULT_BASE_TURN_RATE = 45.f;
	const float DEFAULT_BASE_LOOK_UP_RATE = 45.f;
	const float AIMING_ZOOM_FOV = 25.f;
	const float CAMERA_BOOM_ARM_LENGTH = 180.f;
	const float CAMERA_ZOOM_INTERPOLATION_SPEED = 20.f;
	const float MAX_LINE_TRACE_DISTANCE = 50000.f;
    const int32 INVENTORY_CAPACITY{ 6 };

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual void Jump() override;

	/** Moves character forwards or backwards */
	void MoveForward(float Value);

	/** Moves character to the right or left */
	void MoveRight(float Value);

	/** 
	 * Called via input to turn character at a given rate.
	 * @param Rate	Normalized rate (i.e. 1.0 means 100% of desired turn rate).
	*/
	void TurnAtRate(float Rate);

	/**
	 * Called via input to look up or down at a given rate.
	 * @param Rate	Normalized rate (i.e. 1.0 means 100% of desired rate).
	 */
	void LookUpAtRate(float Rate);

	void TurnWithMouse(float Value);

	void LookUpWithMouse(float Value);


	/**
	 * Called when Fire Button is pressed.
	 */
	void FireWeapon();

	bool GetBeamEndLocation(const FVector& MuzzleSocketLocation, FHitResult& OutHitResult);

	void AimingButtonPressed();

	void AimingButtonReleased();

	void InterpolateAimCameraZoom(float DeltaTime);

	void RefreshAimingOrHipLookRates();

	void CalculateCrosshairSpread(float DeltaTime);

	void FireButtonPressed();

	void FireButtonReleased();

	void StartFireTimer();

	UFUNCTION()
	void ResetAutoFire();

	bool TryGetTraceUnderCrosshairs(FHitResult& OutHitResult, FVector& OutTraceEndOrHitLocation);

	void TraceForOverlappingItems();

	AWeapon* SpawnDefaultWeapon();

	void EquipWeapon(class AWeapon* WeaponToEquip, bool bSwapping = false);

	void DropWeapon();

	void SelectButtonPressed();

	void SelectButtonReleased();

	void SwapWeapon(AWeapon* WeaponToSwap);

	void InitializeAmmoMap();

	bool WeaponHasAmmo() const;

	void PlayFireSound();

	void SendBullet();

	void PlayGunFireMontage();

	void ReloadButtonPressed();

	void ReloadWeapon();

	/** Checks whether the character has ammo of the equipped weapon's ammo type */
	bool IsCarryingAmmo();

	UFUNCTION(BlueprintCallable)
	void GrabClip();

	UFUNCTION(BlueprintCallable)
	void ReleaseClip();

	void CrouchButtonPressed();

	/** Interps capsule half-height when crouching or standing */
	void InterpCapsuleHalfHeight(float DeltaTime);

	void Aim();

	void StopAiming();

	void PickUpAmmo(class AAmmo* Ammo);

    void InitializeInterpLocations();

    void FKeyPressed();
    void OneKeyPressed();
    void TwoKeyPressed();
    void ThreeKeyPressed();
    void FourKeyPressed();
    void FiveKeyPressed();
    void ExchangeInventoryItems(int32 CurrentItemIndex, int32 NewItemIndex);

    /** Gets the slot index of the first inventory empty spot; -1 if none found */
    int32 GetEmptyInventorySlot();

    void HighlightInventorySlot();

    UFUNCTION(BlueprintCallable)
    EPhysicalSurface GetSurfaceType() const;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

     /** Gets the index of the interp location that has the least amount of 
     * items interping to it at any given point in time */
    int32 GetInterpLocationIndex();

    void IncrementInterpLocationItemCount(int32 Index, int32 Amount);

    void UnhighlightInventorySlot();

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* CameraBoom{ nullptr };

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* FollowCamera{ nullptr };

	/** Base turn rate in degrees per second. Other scaling may affect final turn rate */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	float BaseTurnRate;

	/** Base look-up/down rate in degrees per second. Other scaling may affect final turn rate */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	float BaseLookUpRate;

	/** Turn rate while not aiming */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	float HipTurnRate;

	/** Look up rate when not aiming */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	float HipLookUpRate;

	/** Turn rate when aiming */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	float AimingTurnRate;

	/** Look up rate when aiming */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	float AimingLookUpRate;

	/** Mouse sensitivity turn rate when not aiming */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"), meta = (ClampMin = "0.1", ClampMax = "1.0",  UIMin = "0.1", UIMax = "1.0"))
	float MouseHipTurnRate;

	/** Mouse sensitivity look up rate when not aiming */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"), meta = (ClampMin = "0.1", ClampMax = "1.0",  UIMin = "0.1", UIMax = "1.0"))
	float MouseHipLookUpRate;

	/** Mouse sensitivity turn rate while aiming */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"), meta = (ClampMin = "0.1", ClampMax = "1.0",  UIMin = "0.1", UIMax = "1.0"))
	float MouseAimingTurnRate;

	/** Mouse sensitivity look up rate while aiming */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"), meta = (ClampMin = "0.1", ClampMax = "1.0",  UIMin = "0.1", UIMax = "1.0"))
	float MouseAimingLookUpRate;

	/** Particles spawned upon bullet impact */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	UParticleSystem* ImpactParticles{ nullptr };

	/** Smoke trail for bullets */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	UParticleSystem* BeamParticles{ nullptr };

	/** Montage for firing the weapon */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	class UAnimMontage* HipFireMontage{ nullptr };

	UPROPERTY(VisibleAnywhere, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	bool bAiming;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	float CameraDefaultFieldOfView;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	float CameraZoomedFieldOfView;

	float CameraCurrentFieldOfView;

	UPROPERTY(EditAnywhere, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	float ZoomInterpolationSpeed;

	/** Determines the spread of the crosshairs */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Crosshairs, meta = (AllowPrivateAccess = "true"))
	float CrosshairSpreadMultiplier;

	/** Velocity component for crosshair's spread */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Crosshairs, meta = (AllowPrivateAccess = "true"))
	float CrosshairVelocityFactor;

	/** In-air component for crosshair's spread */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Crosshairs, meta = (AllowPrivateAccess = "true"))
	float CrosshairInAirFactor;

	/** Aim component for crosshair spread */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Crosshairs, meta = (AllowPrivateAccess = "true"))
	float CrosshairAimFactor;

	/** Shooting component for crosshair spread */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Crosshairs, meta = (AllowPrivateAccess = "true"))
	float CrosshairShootingFactor;

	/** Left mouse button or gamepad right-trigger pressed */
	bool bFireButtonPressed;

	/** Timer between gunshots */
	FTimerHandle AutoFireTimer;

	/** Allows tracing for items every frame when set to true */
	bool bShouldTraceForOverlappingItems;

	int32 OverlappedItemCount;	// Can be int8

	/** The last AItem traced in most recent frame */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Items, meta = (AllowPrivateAccess = "true"))
	class AItem* LastTracedPickupItem{ nullptr };

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Combat, meta = (AllowPrivateAccess = "true"))
	AWeapon* EquippedWeapon{ nullptr };

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Combat, meta = (AllowPrivateAccess = "true"))
	TSubclassOf<AWeapon> DefaultWeaponClass;

	/** Current item hit by trace or null if no item. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Combat, meta = (AllowPrivateAccess = "true"))
	AItem* TraceHitItem;

	/** Combat state can only occur when not occupied */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Combat, meta = (AllowPrivateAccess = "true"))
	ECombatState CombatState;

	/** Montage for reload animation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Combat, meta = (AllowPrivateAccess = "true"))
	UAnimMontage* ReloadAnimMontage{ nullptr };

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Combat, meta = (AllowPrivateAccess = "true"))
	UAnimMontage* EquipAnimMontage{ nullptr };

	UFUNCTION(BlueprintCallable)
	void FinishReloading();

    UFUNCTION(BlueprintCallable)
	void FinishEquipping();

	/** Distance outward from the camera for the interp destination */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Items, meta = (AllowPrivateAccess = "true"))
	float CameraInterpDistance;

	/** Distance upward from the camera for the interp destination */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Items, meta = (AllowPrivateAccess = "true"))
	float CameraInterpElevation;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Items, meta = (AllowPrivateAccess = "true"))
	TMap<EAmmoType, int32> AmmoMap;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Items, meta = (AllowPrivateAccess = "true"))
	int32 Starting9mmAmmoAmount;

	/** Starting assault rifle ammo amount */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Items, meta = (AllowPrivateAccess = "true"))
	int32 StartingARAmmoAmount;

	/** Transform of the gun's magazine clip when it's grabbed during reloading */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Combat, meta = (AllowPrivateAccess = "true"))
	FTransform ClipTransform;

	/** Scene component to attach to the character's hand during reloading */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Combat, meta = (AllowPrivateAccess = "true"))
	USceneComponent* HandSceneComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Movement, meta = (AllowPrivateAccess = "true"))
	bool bCrouching;

	/** Regular movement speed */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Movement, meta = (AllowPrivateAccess = "true"))
	float BaseMovementSpeed;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Movement, meta = (AllowPrivateAccess = "true"))
	float CrouchMovementSpeed;

	float CurrentCapsuleHalfHeight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Movement, meta = (AllowPrivateAccess = "true"))
	float StandingCapsuleHalfHeight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Movement, meta = (AllowPrivateAccess = "true"))
	float CrouchingCapsuleHalfHeight;

	/** Ground friction while standing (not crouching) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Movement, meta = (AllowPrivateAccess = "true"))
	float BaseGroundFriction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Movement, meta = (AllowPrivateAccess = "true"))
	float CrouchingGroundFriction;

	bool bAimingButtonPressed;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
    USceneComponent* WeaponInterpComp;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
    USceneComponent* InterpComp1;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
    USceneComponent* InterpComp2;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
    USceneComponent* InterpComp3;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
    USceneComponent* InterpComp4;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
    USceneComponent* InterpComp5;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
    USceneComponent* InterpComp6;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
    TArray<FInterpLocation> InterpLocations;

    FTimerHandle PickupSoundTimer;
    FTimerHandle EquipSoundTimer;

    bool bShouldPlayPickupSound;
    bool bShouldPlayEquipSound;

    /** Time to wait before we can play another pickup sound */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Items, meta = (AllowPrivateAccess = "true"))
    float PickupSoundResetTime;

    /** Time to wait before we can play another equip sound */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Items, meta = (AllowPrivateAccess = "true"))
    float EquipSoundResetTime;

    void ResetPickupSoundTimer();
    void ResetEquipSoundTimer();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Inventory, meta = (AllowPrivateAccess = "true"))
    TArray<AItem*> Inventory;

    /** Delegate for sending slot information to inventory bar when equipping */
    UPROPERTY(BlueprintAssignable, Category = Delegates, meta = (AllowPrivateAccess = "true"))
    FEquipItemDelegate EquipItemDelegate;

    /**Delegate for sending slot information for playing the icon animation */
    UPROPERTY(BlueprintAssignable, Category = Delegates, meta = (AllowPrivateAccess = "true"))
    FHighlightIconDelegate HighlightIconDelegate;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Inventory, meta = (AllowPrivateAccess = "true"))
    int32 HighlightedSlot;

public:
	FORCEINLINE USpringArmComponent* GetCameraBoom() const 
	{ 
		return CameraBoom; 
	}

	FORCEINLINE UCameraComponent* GetFollowCamera() const 
	{ 
		return FollowCamera; 
	}

	FORCEINLINE bool IsAiming() const
	{
		return bAiming;
	}

	UFUNCTION(BlueprintCallable)
	float GetCrosshairSpreadMultiplier() const;

	FORCEINLINE int32 GetOverlappedItemCount() const
	{
		return OverlappedItemCount;
	}

	FORCEINLINE ECombatState GetCombatState() const
	{
		return CombatState;
	}

	FORCEINLINE bool IsCrouching() const
	{ 
		return bCrouching; 
	}

    FORCEINLINE AWeapon* GetEquippedWeapon() const
    {
        return EquippedWeapon;
    }

    FInterpLocation GetInterpLocation(int32 Index);

	/** Adds given amount to current number of overlapped items */
	void AddOverlappedItemCount(int32 Amount);

    // TODO: No longer needed. AItem already has GetInterpLocation
	// FVector GetCameraInterpLocation();

	void LoadPickupItem(AItem* Item);

    FORCEINLINE bool ShouldPlayPickupSound() const { return bShouldPlayPickupSound; }

    FORCEINLINE bool ShouldPlayEquipSound() const { return bShouldPlayEquipSound; }

    void StartPickupSoundTimer();
    void StartEquipSoundTimer();
};
