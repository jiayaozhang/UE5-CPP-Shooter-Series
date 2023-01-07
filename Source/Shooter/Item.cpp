// Fill out your copyright notice in the Description page of Project Settings.


#include "Item.h"
#include "Components/BoxComponent.h"
#include "Components/WidgetComponent.h"
#include "Components/SphereComponent.h"
#include "Camera/CameraComponent.h"
#include "ShooterCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"
#include "Curves/CurveVector.h"

// Sets default values
AItem::AItem() :
	ItemName(FString("Unnamed")),
	ItemCount(0),
	ItemRarity(EItemRarity::EIR_Common),
	ItemState(EItemState::EIS_Pickup),

	// Item interp
	ItemInterpStartLocation(FVector(0.f)),
	CameraTargetLocation(FVector(0.f)),
	bInterping(false),
	ZCurveTime(0.7f),
	InterpInitialYawOffset(0.f),

    ItemType(EItemType::EIT_MAX),

    InterpLocationIndex(0),

    MaterialIndex(0),

    bCanChangeCustomDepth(true),

    // Dynamic material parameters
    PulseCurveTime(5.f),
    GlowAmount(150.f),
    FresnelExponent(3.f),
    FresnelReflectFraction(4.f),

    SlotIndex(0),

    bCharacterInventoryFull(false)
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	ItemMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("ItemMesh"));
	SetRootComponent(ItemMesh);

	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
	CollisionBox->SetupAttachment(ItemMesh);
	CollisionBox->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	CollisionBox->SetCollisionResponseToChannel(
		ECollisionChannel::ECC_Visibility,
		ECollisionResponse::ECR_Block);

	PickupWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("PickupWidget"));
	PickupWidget->SetupAttachment(GetRootComponent());

	AreaSphere = CreateDefaultSubobject<USphereComponent>(TEXT("AreaSphere"));
	AreaSphere->SetupAttachment(GetRootComponent());
}

// Called when the game starts or when spawned
void AItem::BeginPlay()
{
	Super::BeginPlay();
	
	if (PickupWidget)
	{
		PickupWidget->SetVisibility(false);
	}

	if (AreaSphere)
	{
		AreaSphere->OnComponentBeginOverlap.AddDynamic(this, &AItem::OnSphereOverlap);
		AreaSphere->OnComponentEndOverlap.AddDynamic(this, &AItem::OnSphereEndOverlap);
	}

	SetActiveStars();

	SetItemProperties(ItemState);

    InitializeCustomDepth();

	StartPulseTimer();
}

void AItem::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, 
		AActor* OtherActor, 
		UPrimitiveComponent* OtherComponent, 
		int32 OtherBodyIndex, 
		bool bFromSweep,
		const FHitResult& SweepResult)
{
	if (!OtherActor)
	{
		return;
	}

	AShooterCharacter* ShooterCharacter = Cast<AShooterCharacter>(OtherActor);
	if (ShooterCharacter)
	{
		ShooterCharacter->AddOverlappedItemCount(1);
	}
}

void AItem::OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComponent,
		int32 OtherBodyIndex)
{
	if (!OtherActor)
	{
		return;
	}

	AShooterCharacter* ShooterCharacter = Cast<AShooterCharacter>(OtherActor);
	if (ShooterCharacter)
	{
		ShooterCharacter->AddOverlappedItemCount(-1);
        ShooterCharacter->UnhighlightInventorySlot();
	}
}

// Called every frame
void AItem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	InterpolateItemLoad(DeltaTime);

    // Set dynamic material parameters using pulse curve values
    UpdatePulse();
}

void AItem::SetActiveStars()
{
	for (int32 i = 0; i < MAX_RARITY_SCORE; i++)
	{
		ActiveStars.Add(false);
	}

	switch (ItemRarity)
	{
		case EItemRarity::EIR_Damaged:
			ActiveStars[0] = true;
			break;

		case EItemRarity::EIR_Common:
			ActiveStars[0] = true;
			ActiveStars[1] = true;
			break;

		case EItemRarity::EIR_Uncommon:
			ActiveStars[0] = true;
			ActiveStars[1] = true;
			ActiveStars[2] = true;
			break;

		case EItemRarity::EIR_Rare:
			ActiveStars[0] = true;
			ActiveStars[1] = true;
			ActiveStars[2] = true;
			ActiveStars[3] = true;
			break;

		case EItemRarity::EIR_Legendary:
			ActiveStars[0] = true;
			ActiveStars[1] = true;
			ActiveStars[2] = true;
			ActiveStars[3] = true;
			ActiveStars[4] = true;
			break;
	}
}

void AItem::SetItemState(EItemState State)
{
	ItemState = State;

	SetItemProperties(State);
}

void AItem::SetItemProperties(EItemState State)
{
	switch (State)
	{
		case EItemState::EIS_Pickup:
			ItemMesh->SetSimulatePhysics(false);
			ItemMesh->SetVisibility(true);
			ItemMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
			ItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			ItemMesh->SetEnableGravity(false);

			AreaSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Overlap);
			AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

			CollisionBox->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
			CollisionBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);
			CollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			break;

		case EItemState::EIS_Equipped:
			ItemMesh->SetSimulatePhysics(false);
			ItemMesh->SetVisibility(true);
			ItemMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
			ItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			ItemMesh->SetEnableGravity(false);

			AreaSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
			AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);

			CollisionBox->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
			CollisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);

			PickupWidget->SetVisibility(false);
			break;

		case EItemState::EIS_Falling:
			ItemMesh->SetSimulatePhysics(true);
			ItemMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			ItemMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
			ItemMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECollisionResponse::ECR_Block);
			ItemMesh->SetEnableGravity(true);
            ItemMesh->SetVisibility(true);

			AreaSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
			AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);

			CollisionBox->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
			CollisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			break;

		case EItemState::EIS_EquipInterpolating:
			PickupWidget->SetVisibility(false);

			ItemMesh->SetSimulatePhysics(false);
			ItemMesh->SetVisibility(true);
			ItemMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
			ItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			ItemMesh->SetEnableGravity(false);

			AreaSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
			AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);

			CollisionBox->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
			CollisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			break;

        case EItemState::EIS_PickedUp:
            PickupWidget->SetVisibility(false);

			ItemMesh->SetSimulatePhysics(false);
             // Disable visibility: This item will exist but it won't be visible or interact with anything
			ItemMesh->SetVisibility(false);
			ItemMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
			ItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			ItemMesh->SetEnableGravity(false);

			AreaSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
			AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);

			CollisionBox->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
			CollisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            break;
	}
}

void AItem::StartItemCurve(AShooterCharacter* OriginCharacter, bool bForcePlaySound)
{
    if (!OriginCharacter)
    {
        return;
    }

	Character = OriginCharacter;

    // Get array index in InterpLocations with the lowest item count
    InterpLocationIndex = Character->GetInterpLocationIndex();
    // Add 1 to the item count for this interp location struct
    Character->IncrementInterpLocationItemCount(InterpLocationIndex, 1);

	PlayPickupSound(bForcePlaySound);

	ItemInterpStartLocation = GetActorLocation();

	bInterping = true;	// TODO: We actually don't need this if EIS_EquipInterpolating is the current state

	SetItemState(EItemState::EIS_EquipInterpolating);

    GetWorldTimerManager().ClearTimer(PulseTimer);

	GetWorldTimerManager().SetTimer(
		ItemInterpTimer, 
		this, 
		&AItem::ItemInterpTimerFinished, 
		ZCurveTime);

	const float InitialCameraRotationYaw = Character->GetFollowCamera()->GetComponentRotation().Yaw;
	const float InitialItemRotationYaw = GetActorRotation().Yaw;

	InterpInitialYawOffset = InitialItemRotationYaw - InitialCameraRotationYaw;

    bCanChangeCustomDepth = false;
}

void AItem::ItemInterpTimerFinished()
{
	bInterping = false;

	if (Character)
	{
        // subtract 1 from the item count of the interp location struct
        Character->IncrementInterpLocationItemCount(InterpLocationIndex, -1);

		Character->LoadPickupItem(this);

        Character->UnhighlightInventorySlot();
	}

	// Restore item's normal size
	SetActorScale3D(FVector(1.f));

    DisableGlowMaterial();

    bCanChangeCustomDepth = true;
    DisableCustomDepth();
}

void AItem::InterpolateItemLoad(float DeltaTime)
{
	if (!bInterping)
	{
		return;
	}

	if (!(Character || ItemZCurve))
	{
		return;
	}

	const float ElapsedTime = GetWorldTimerManager().GetTimerElapsed(ItemInterpTimer);
	const float CurveValue = ItemZCurve->GetFloatValue(ElapsedTime);

    // Get the item's initial location when the curve started
	FVector ItemLocation = ItemInterpStartLocation;

    // Get location in front of camera
	FVector CameraInterpLocation{ GetInterpLocation() };

	const FVector ItemToCameraInterpLocation{ 
		FVector(0.f, 0.f, (CameraInterpLocation -  ItemLocation).Z) 
	};
	const float DeltaZScaleFactor = ItemToCameraInterpLocation.Size();

	const FVector CurrentLocation = GetActorLocation();
	const float InterpXValue = FMath::FInterpTo(
		CurrentLocation.X, 
		CameraInterpLocation.X, 
		DeltaTime, 
		30.f);

	const float InterpYValue = FMath::FInterpTo(
		CurrentLocation.Y,
		CameraInterpLocation.Y,
		DeltaTime,
		30.f);

	ItemLocation.X = InterpXValue;
	ItemLocation.Y = InterpYValue;

	ItemLocation.Z +=  CurveValue * DeltaZScaleFactor;
	SetActorLocation(ItemLocation, true, nullptr, ETeleportType::TeleportPhysics);

	/** Camera rotation this frame and its rotation relative to initial yaw offset */
	const FRotator CameraRotation{ Character->GetFollowCamera()->GetComponentRotation() };
	const FRotator ItemRotation{ 0.f, CameraRotation.Yaw + InterpInitialYawOffset, 0.f };

	SetActorRotation(ItemRotation, ETeleportType::TeleportPhysics);

	if (ItemScaleCurve)
	{
		const float ScaleCurveValue = ItemScaleCurve->GetFloatValue(ElapsedTime);
		SetActorScale3D(FVector(ScaleCurveValue, ScaleCurveValue, ScaleCurveValue));
	}
}

FVector AItem::GetInterpLocation()
{
    if (!Character)
    {
        return FVector(0.f);
    }

    switch (ItemType)
    {
        case EItemType::EIT_Ammo:
            return Character->GetInterpLocation(InterpLocationIndex).SceneComponent->GetComponentLocation();

        case EItemType::EIT_Weapon:
            return Character->GetInterpLocation(0).SceneComponent->GetComponentLocation();
            break;
    }

    return FVector();
}

void AItem::PlayPickupSound(bool bForcePlaySound)
{
    if (bForcePlaySound)
    {
        if (PickupSound)
		{
			UGameplayStatics::PlaySound2D(this, PickupSound);
		}
    }
	else if (Character && Character->ShouldPlayPickupSound())
	{
		Character->StartPickupSoundTimer();
		if (PickupSound)
		{
			UGameplayStatics::PlaySound2D(this, PickupSound);
		}
	}
}

void AItem::PlayEquipSound(bool bForcePlaySound)
{
    if (bForcePlaySound)
    {
        if (EquipSound)
		{
			UGameplayStatics::PlaySound2D(this, EquipSound);
		}
    }
    else if (Character && Character->ShouldPlayEquipSound())
	{
		Character->StartEquipSoundTimer();
		if (EquipSound)
		{
			UGameplayStatics::PlaySound2D(this, EquipSound);
		}
	}
}

void AItem::EnableCustomDepth()
{
    if (bCanChangeCustomDepth)
    {
        ItemMesh->SetRenderCustomDepth(true);
    }
}

void AItem::DisableCustomDepth()
{
    if (bCanChangeCustomDepth)
    {
        ItemMesh->SetRenderCustomDepth(false);
    }
}

void AItem::InitializeCustomDepth()
{
    DisableCustomDepth();
}

void AItem::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    
    // Load the data in the Item Rarity Data Table
    FString RarityTablePath(TEXT("DataTable'/Game/_Game_/DataTable/ItemRarityDataTable.ItemRarityDataTable'"));
    UDataTable* RarityTableObject = Cast<UDataTable>(StaticLoadObject(UDataTable::StaticClass(), nullptr, *RarityTablePath));
    if (RarityTableObject)
    {
        FItemRarityTable* RarityRow = nullptr;
        switch (ItemRarity)
        {
            case EItemRarity::EIR_Damaged:
                RarityRow = RarityTableObject->FindRow<FItemRarityTable>(FName("Damaged"), TEXT(""));
                break;

            case EItemRarity::EIR_Common:
                RarityRow = RarityTableObject->FindRow<FItemRarityTable>(FName("Common"), TEXT(""));
                break;

            case EItemRarity::EIR_Uncommon:
                RarityRow = RarityTableObject->FindRow<FItemRarityTable>(FName("Uncommon"), TEXT(""));
                break;

            case EItemRarity::EIR_Rare:
                RarityRow = RarityTableObject->FindRow<FItemRarityTable>(FName("Rare"), TEXT(""));
                break;

            case EItemRarity::EIR_Legendary:
                RarityRow = RarityTableObject->FindRow<FItemRarityTable>(FName("Legendary"), TEXT(""));
                break;
        }

        if (RarityRow)
        {
            GlowColor = RarityRow->GlowColor;
            LightColor = RarityRow->LightColor;
            DarkColor = RarityRow->DarkColor;
            NumberOfStars = RarityRow->NumberOfStars;
            IconBackground = RarityRow->IconBackground;

            if (GetItemMesh())
            {
                GetItemMesh()->SetCustomDepthStencilValue(RarityRow->CustomDepthStencil);
            }
        }
    }

    if (MaterialInstance)
    {
        DynamicMaterialInstance = UMaterialInstanceDynamic::Create(MaterialInstance, this);
        DynamicMaterialInstance->SetVectorParameterValue(TEXT("FresnelColor"), GlowColor);
        ItemMesh->SetMaterial(MaterialIndex, DynamicMaterialInstance);

        EnableGlowMaterial();
    }
}

void AItem::EnableGlowMaterial()
{
    if (DynamicMaterialInstance)
    {
        DynamicMaterialInstance->SetScalarParameterValue(TEXT("GlowBlendAlpha"), 0.f);
    }
}

void AItem::DisableGlowMaterial()
{
    if (DynamicMaterialInstance)
    {
        DynamicMaterialInstance->SetScalarParameterValue(TEXT("GlowBlendAlpha"), 1.f);
    }
}


void AItem::ResetPulseTimer()
{
    StartPulseTimer();
}

void AItem::StartPulseTimer()
{
    if (ItemState == EItemState::EIS_Pickup)
    {
        GetWorldTimerManager().SetTimer(PulseTimer, this, &AItem::ResetPulseTimer, PulseCurveTime);
    }
}

void AItem::UpdatePulse()
{
    float ElapsedTime{  };
    FVector CurveValue{};

    switch (ItemState)
    {
		case EItemState::EIS_Pickup:
			if (PulseCurve)
			{
				ElapsedTime = GetWorldTimerManager().GetTimerElapsed(PulseTimer);
				CurveValue = PulseCurve->GetVectorValue(ElapsedTime);
			}
			break;

		case EItemState::EIS_EquipInterpolating:
			if (InterpPulseCurve)
			{
                ElapsedTime = GetWorldTimerManager().GetTimerElapsed(ItemInterpTimer);
                CurveValue = InterpPulseCurve->GetVectorValue(ElapsedTime);
			}
			break;
    }

    if (DynamicMaterialInstance)
    {
        DynamicMaterialInstance->SetScalarParameterValue(TEXT("GlowAmount"), CurveValue.X * GlowAmount);
        DynamicMaterialInstance->SetScalarParameterValue(TEXT("FresnelExponent"), CurveValue.Y * FresnelExponent);
        DynamicMaterialInstance->SetScalarParameterValue(TEXT("FresnelReflectFraction"), CurveValue.Z * FresnelReflectFraction);
    }
}
