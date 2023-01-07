// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/DataTable.h"
#include "Item.generated.h"

UENUM(BlueprintType)
enum class EItemRarity : uint8
{
	EIR_Damaged UMETA(DisplayName = "Damaged"),
	EIR_Common UMETA(DisplayName = "Common"),
	EIR_Uncommon UMETA(DisplayName = "Uncommon"),
	EIR_Rare UMETA(DisplayName = "Rare"),
	EIR_Legendary UMETA(DisplayName = "Legendary"),

	EIR_MAX UMETA(DisplayName = "DefaultMAX")
};

UENUM(BlueprintType)
enum class EItemState : uint8
{
	EIS_Pickup UMETA(DisplayName = "Pickup"),
	EIS_EquipInterpolating UMETA(DisplayName = "EquipInterpolating"),
	EIS_PickedUp UMETA(DisplayName = "PickedUp"),
	EIS_Equipped UMETA(DisplayName = "Equipped"),
	EIS_Falling UMETA(DisplayName = "Falling"),

	EIS_MAX UMETA(DisplayName = "DefaultMAX")
};

UENUM(BlueprintType)
enum class EItemType : uint8
{
	EIT_Ammo UMETA(DisplayName = "Ammo"),
	EIT_Weapon UMETA(DisplayName = "Weapon"),

	EIT_MAX UMETA(DisplayName = "DefaultMAX")
};

USTRUCT(BlueprintType)
struct FItemRarityTable : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FLinearColor GlowColor;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FLinearColor LightColor;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FLinearColor DarkColor;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 NumberOfStars;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    UTexture2D* IconBackground{ nullptr };
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 CustomDepthStencil;
};

UCLASS()
class SHOOTER_API AItem : public AActor
{
	GENERATED_BODY()

public:	
	// Sets default values for this actor's properties
	AItem();

private:
	const int32 MAX_RARITY_SCORE = 5;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnSphereOverlap(
		UPrimitiveComponent* OverlappedComponent, 
		AActor* OtherActor, 
		UPrimitiveComponent* OtherComponent, 
		int32 OtherBodyIndex, 
		bool bFromSweep,
		const FHitResult& SweepResult);

	UFUNCTION()
	void OnSphereEndOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComponent,
		int32 OtherBodyIndex);

	void SetActiveStars();

	/** Sets properties of item's components based on state */
	virtual void SetItemProperties(EItemState State);

	void ItemInterpTimerFinished();

	void InterpolateItemLoad(float Deltatime);

    /** Get interp location based on item type */
    FVector GetInterpLocation();

	void PlayPickupSound(bool bForcePlaySound = false);
  
    virtual void InitializeCustomDepth();

    virtual void OnConstruction(const FTransform& Transform) override;

    void EnableGlowMaterial();

     void ResetPulseTimer();

    void StartPulseTimer();

    void UpdatePulse();
	
public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	void PlayEquipSound(bool bForcePlaySound = false);

    virtual void EnableCustomDepth();
    virtual void DisableCustomDepth();

    void DisableGlowMaterial();

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item Properties", meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* ItemMesh{ nullptr };

	/** Line trace collides with box to show HUD widgets */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Properties", meta = (AllowPrivateAccess = "true"))
	class UBoxComponent* CollisionBox{ nullptr };

	/** Popup widget shown to the player when their character looks at the item */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Properties", meta = (AllowPrivateAccess = "true"))
	class UWidgetComponent* PickupWidget{ nullptr };

	/** Enables item tracing when overlapped */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Properties", meta = (AllowPrivateAccess = "true"))
	class USphereComponent* AreaSphere{ nullptr };

	/** The name that appears in the item's pickup widget */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Properties", meta = (AllowPrivateAccess = "true"))
	FString ItemName;

	/** Item count, such as ammo, etc. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Properties", meta = (AllowPrivateAccess = "true"))
	int32 ItemCount;

	/** Determines number of stars in item widget */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Rarity, meta = (AllowPrivateAccess = "true"))
	EItemRarity ItemRarity;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item Properties", meta = (AllowPrivateAccess = "true"))
	TArray<bool> ActiveStars;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item Properties", meta = (AllowPrivateAccess = "true"))
	EItemState ItemState;

	/** The curve asset to use for the item's Z location when interping */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item Properties", meta = (AllowPrivateAccess = "true"))
	class UCurveFloat* ItemZCurve;

	/** Starting location when interping begins */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item Properties", meta = (AllowPrivateAccess = "true"))
	FVector ItemInterpStartLocation;

	/** Target interp location in front of the camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item Properties", meta = (AllowPrivateAccess = "true"))
	FVector CameraTargetLocation;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item Properties", meta = (AllowPrivateAccess = "true"))
	bool bInterping;

	/** Plays when we start interping */
	FTimerHandle ItemInterpTimer;

	/** Duration of the curve and timer */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item Properties", meta = (AllowPrivateAccess = "true"))
	float ZCurveTime;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item Properties", meta = (AllowPrivateAccess = "true"))
	class AShooterCharacter* Character;

	/** Initial Yaw offset between the camera and the interping item */
	float InterpInitialYawOffset;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item Properties", meta = (AllowPrivateAccess = "true"))
	UCurveFloat* ItemScaleCurve{ nullptr };

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Properties", meta = (AllowPrivateAccess = "true"))
	class USoundCue* PickupSound{ nullptr };

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Properties", meta = (AllowPrivateAccess = "true"))
	USoundCue* EquipSound{ nullptr };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Properties", meta = (AllowPrivateAccess = "true"))
	EItemType ItemType;

    /** Index of the interp location this item is interping to */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item Properties", meta = (AllowPrivateAccess = "true"))
    int32 InterpLocationIndex;

    /** Index for the material we'd like to change at runtime */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Properties", meta = (AllowPrivateAccess = "true"))
    int32 MaterialIndex;

    /** Dynamic material instance that we can change at runtime */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item Properties", meta = (AllowPrivateAccess = "true"))
    UMaterialInstanceDynamic* DynamicMaterialInstance{ nullptr };

    /** Material instance used with the dynamic material instance */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Properties", meta = (AllowPrivateAccess = "true"))
    UMaterialInstance* MaterialInstance{ nullptr };

    bool bCanChangeCustomDepth;

    /** Curve to drive the dynamic material parameters */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item Properties", meta = (AllowPrivateAccess = "true"))
    class UCurveVector* PulseCurve{ nullptr };

    /**  */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item Properties", meta = (AllowPrivateAccess = "true"))
    UCurveVector* InterpPulseCurve{ nullptr };

    FTimerHandle PulseTimer;

    /** Time for the PulseTimer */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item Properties", meta = (AllowPrivateAccess = "true"))
    float PulseCurveTime;

    UPROPERTY(VisibleAnywhere, Category = "Item Properties", meta = (AllowPrivateAccess = "true"))
    float GlowAmount;

    UPROPERTY(VisibleAnywhere, Category = "Item Properties", meta = (AllowPrivateAccess = "true"))
    float FresnelExponent;

    UPROPERTY(VisibleAnywhere, Category = "Item Properties", meta = (AllowPrivateAccess = "true"))
    float FresnelReflectFraction;

    /** Icon for this item in the inventory */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory", meta = (AllowPrivateAccess = "true"))
    UTexture2D* ItemIcon{ nullptr };

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory", meta = (AllowPrivateAccess = "true"))
    UTexture2D* QuantityIcon{ nullptr };

    /** Slot in the inventory */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item Properties", meta = (AllowPrivateAccess = "true"))
    int32 SlotIndex;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item Properties", meta = (AllowPrivateAccess = "true"))
    bool bCharacterInventoryFull;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = DataTable, meta = (AllowPrivateAccess = "true"))
    class UDataTable* ItemRarityDataTable;

    /** Color in the glow material */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Rarity, meta = (AllowPrivateAccess = "true"))
    FLinearColor GlowColor;

    /** Light color in the pickup widget */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Rarity, meta = (AllowPrivateAccess = "true"))
    FLinearColor LightColor;

    /** Dark color in the pickup widget */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Rarity, meta = (AllowPrivateAccess = "true"))
    FLinearColor DarkColor;

    /** Number of stars in the pickup widget */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Rarity, meta = (AllowPrivateAccess = "true"))
    int32 NumberOfStars;

    /** Background icon for the inventory */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Rarity, meta = (AllowPrivateAccess = "true"))
    UTexture2D* IconBackground{ nullptr };

public:
	FORCEINLINE UWidgetComponent* GetPickupWidget() const 
	{
		return PickupWidget;
	}

	FORCEINLINE EItemState GetItemState() const
	{
		return ItemState;
	}

	void SetItemState(EItemState Value);

	FORCEINLINE USkeletalMeshComponent* GetItemMesh() const
	{
		return ItemMesh;
	}

	FORCEINLINE USoundCue* GetPickupSound() const
	{
		return PickupSound;
	}

    FORCEINLINE void SetPickupSound(USoundCue* Sound)
    {
        PickupSound = Sound;
    }

	FORCEINLINE USoundCue* GetEquipSound() const
	{
		return EquipSound;
	}

    FORCEINLINE void SetEquipSound(USoundCue* Sound)
    {
        EquipSound = Sound;
    }

	FORCEINLINE int32 GetItemCount() const { return ItemCount; }

    FORCEINLINE int32 GetSlotIndex() const { return SlotIndex; }

    FORCEINLINE void SetSlotIndex(int32 Value) 
    { 
        SlotIndex = Value; 
    }

    FORCEINLINE void SetCharacter(AShooterCharacter* ShooterCharacter) 
    { 
        Character = ShooterCharacter; 
    }

    FORCEINLINE void SetCharacterInventoryFull(bool bFull)
    {
        bCharacterInventoryFull = bFull;
    }
	
    FORCEINLINE void SetItemName(FString Value)
    {
        ItemName = Value;
    }

    FORCEINLINE void SetItemIcon(UTexture2D* Icon)
    {
        ItemIcon = Icon;
    }

    FORCEINLINE void SetQuantityIcon(UTexture2D* Icon)
    {
        QuantityIcon = Icon;
    }

    FORCEINLINE UMaterialInstance* GetMaterialInstance() const
    {
        return MaterialInstance;
    }

    FORCEINLINE void SetMaterialInstance(UMaterialInstance* Material)
    {
        MaterialInstance = Material;
    }

    FORCEINLINE UMaterialInstanceDynamic* GetDynamicMaterialInstance() const
    {
        return DynamicMaterialInstance;
    }

    FORCEINLINE void SetDynamicMaterialInstance(UMaterialInstanceDynamic* DynamicMaterial)
    {
        DynamicMaterialInstance = DynamicMaterial;
    }

    FORCEINLINE FLinearColor GetGlowColor() const 
    { 
        return GlowColor; 
    }

    FORCEINLINE int32 GetMaterialIndex() const
    {
        return MaterialIndex;
    }

    FORCEINLINE void SetMaterialIndex(int32 Value)
    {
        MaterialIndex = Value;
    }

	void StartItemCurve(AShooterCharacter* OriginCharacter, bool bForcePlaySound = false);

protected:
	FORCEINLINE UBoxComponent* GetCollisionBox() const { return CollisionBox; }

	FORCEINLINE USphereComponent* GetAreaSphere() const { return AreaSphere; }

};
