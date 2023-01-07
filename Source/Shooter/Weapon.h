// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Item.h"
#include "AmmoType.h"
#include "WeaponType.h"
#include "Weapon.generated.h"

USTRUCT(BlueprintType)
struct FWeaponDataTable : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EAmmoType AmmoType;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 WeaponAmmo;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 MagazineCapacity;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    class USoundCue* PickupSound{ nullptr };

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    class USoundCue* EquipSound{ nullptr };

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    USkeletalMesh* ItemMesh{ nullptr };

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString ItemName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    UTexture2D* InventoryIcon{ nullptr };

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    UTexture2D* AmmoIcon{ nullptr };

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    UMaterialInstance* MaterialInstance{ nullptr };

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 MaterialIndex;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName ClipBoneName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName ReloadMontageSection;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TSubclassOf<UAnimInstance> AnimBP;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    UTexture2D* CrosshairsMiddle;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    UTexture2D* CrosshairsLeft;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    UTexture2D* CrosshairsRight;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    UTexture2D* CrosshairsBottom;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    UTexture2D* CrosshairsTop;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float AutoFireRate;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    UParticleSystem* MuzzleFlash;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    USoundCue* FireSound;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName BoneToHide;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bAutomatic;
};

/**
 * 
 */
UCLASS()
class SHOOTER_API AWeapon : public AItem
{
	GENERATED_BODY()

private:
	const float DEFAULT_THROW_WEAPON_TIME = 0.7f;

public:
	AWeapon();
	
	virtual void Tick(float DeltaTime);

	/** Tosses weapon with impulse */
	void ThrowWeapon();

	bool TryGetBarrelSocketTransform(FTransform& OutBarrelSocketTransform) const;

	bool IsClipFull() const;

protected:
	void StopFalling();

    virtual void OnConstruction(const FTransform& Transform) override;

    virtual void BeginPlay() override;

    void FinishMovingSlide();

    void UpdateSlideDisplacement();

private:
	FTimerHandle ThrowWeaponTimer;
	float ThrowWeaponTime;
	bool bFalling;

	void EnsureWeaponIsUpright();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Properties", meta = (AllowPrivateAccess = "true"))
	int32  MagazineCapacity;

	/** Ammo count for this weapon */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Properties", meta = (AllowPrivateAccess = "true"))
	int32 Ammo;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Properties", meta = (AllowPrivateAccess = "true"))
	EWeaponType WeaponType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Properties", meta = (AllowPrivateAccess = "true"))
	EAmmoType AmmoType;

	/** Animation montage section name for this weapon */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Properties", meta = (AllowPrivateAccess = "true"))
	FName ReloadMontageSectionName;

	/** Indicates whether the clip is moving while reloading */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon Properties", meta = (AllowPrivateAccess = "true"))
	bool bMovingClip;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Properties", meta = (AllowPrivateAccess = "true"))
	FName ClipBoneName;

    /** Data table for weapon properties */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = DataTable, meta = (AllowPrivateAccess = "true"))
    UDataTable* WeaponDataTable{ nullptr };

    int32 PreviousMaterialIndex;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = DataTable, meta = (AllowPrivateAccess = "true"))
    UTexture2D* CrosshairsMiddle;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = DataTable, meta = (AllowPrivateAccess = "true"))
    UTexture2D* CrosshairsLeft;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = DataTable, meta = (AllowPrivateAccess = "true"))
    UTexture2D* CrosshairsRight;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = DataTable, meta = (AllowPrivateAccess = "true"))
    UTexture2D* CrosshairsBottom;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = DataTable, meta = (AllowPrivateAccess = "true"))
    UTexture2D* CrosshairsTop;

    /** The speed at which automatic fire occurs. 
     * Fire rate must be higher than crosshair interpolation speed */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = DataTable, meta = (AllowPrivateAccess = "true"))
    float AutoFireRate;

    /** Particle system spawned at the BarrelSocket */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = DataTable, meta = (AllowPrivateAccess = "true"))
    UParticleSystem* MuzzleFlash;

    /** Sound played when the weapon is fired */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = DataTable, meta = (AllowPrivateAccess = "true"))
    USoundCue* FireSound;

    /** Name of the bone to hide on the weapon mesh */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = DataTable, meta = (AllowPrivateAccess = "true"))
    FName BoneToHide;

    /** Amount that pistol slide is pushed back during pistol fire */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Pistol, meta = (AllowPrivateAccess = "true"))
    float SlideDisplacement;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Pistol, meta = (AllowPrivateAccess = "true"))
    UCurveFloat* SlideDisplacementCurve;

    /** Timer handle for updating slide displacement */
    FTimerHandle SlideTimer;

    /** Time for displacing the slide during pistol fire */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Pistol, meta = (AllowPrivateAccess = "true"))
    float SlideDisplacementTime;

    /** True when moving pistol slide */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Pistol, meta = (AllowPrivateAccess = "true"))
    bool bMovingSlide;

    /** Max distance for the slide on the pistol */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Pistol, meta = (AllowPrivateAccess = "true"))
    float MaxSlideDisplacement;

    /** The amount that pistol will rotate during pistol fire */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Pistol, meta = (AllowPrivateAccess = "true"))
    float RecoilRotation;

    /** Max rotation for pistol rotation */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Pistol, meta = (AllowPrivateAccess = "true"))
    float MaxRecoilRotation;

    /** Specifies whether automatic gunfire is enabled */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Properties", meta = (AllowPrivateAccess = "true"))
    bool bAutomatic;

public:
	FORCEINLINE bool IsFalling() const
	{
		return GetItemState() == EItemState::EIS_Falling && bFalling;
	}

	FORCEINLINE int32 GetAmmo() const { return Ammo; }

	FORCEINLINE int32 GetMagazineCapacity() const { return MagazineCapacity; }

	FORCEINLINE EWeaponType GetWeaponType() { return WeaponType; }

	void DecrementAmmo();

	FORCEINLINE EWeaponType GetWeaponType() const { return WeaponType; }

	FORCEINLINE EAmmoType GetAmmoType() const { return AmmoType; }

	FORCEINLINE FName GetReloadMontageSectionName() const { return ReloadMontageSectionName; }

    FORCEINLINE void SetReloadMontageSectionName(FName Value) { ReloadMontageSectionName = Value; }

	void ReloadAmmo(int32 Amount);

	FORCEINLINE FName GetClipBoneName() const { return ClipBoneName; }

    FORCEINLINE void SetClipBoneName(FName Value) { ClipBoneName = Value; }

	FORCEINLINE void SetMovingClip(bool bValue) { bMovingClip = bValue; }

    FORCEINLINE float GetAutoFireRate() const { return AutoFireRate; }

    FORCEINLINE UParticleSystem* GetMuzzleFlash() const { return MuzzleFlash; }

    FORCEINLINE USoundCue* GetFireSound() const { return FireSound; }

    FORCEINLINE bool GetAutomatic() const { return bAutomatic; }

    void StartSlideTimer();
};
