// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon.h"
#include "Engine/SkeletalMeshSocket.h"

AWeapon::AWeapon() :
    ThrowWeaponTime(DEFAULT_THROW_WEAPON_TIME),
    bFalling(false),
    MagazineCapacity(30),
    Ammo(30),
    WeaponType(EWeaponType::EWT_SubmachineGun),
    AmmoType(EAmmoType::EAT_9mm),
    ReloadMontageSectionName(FName(TEXT("Reload SMG"))),
    ClipBoneName(TEXT("smg_clip")),
    SlideDisplacement(0.f),
    SlideDisplacementTime(0.2f),
    bMovingSlide(false),
    MaxSlideDisplacement(4.f),
    MaxRecoilRotation(20.f),
    bAutomatic(true)
{
    PrimaryActorTick.bCanEverTick = true;
}

void AWeapon::BeginPlay()
{
    Super::BeginPlay();

    if (BoneToHide != FName(""))
    {
        GetItemMesh()->HideBoneByName(BoneToHide, EPhysBodyOp::PBO_None);
    }
}


void AWeapon::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // Ensure weapon is upright
    if (IsFalling())
    {
        EnsureWeaponIsUpright();
    }

    UpdateSlideDisplacement();
}

void AWeapon::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    
    const FString WeaponTablePath = TEXT("DataTable'/Game/_Game_/DataTable/WeaponDataTable.WeaponDataTable'");
    UDataTable* WeaponTableObject = Cast<UDataTable>(StaticLoadObject(UDataTable::StaticClass(), nullptr, *WeaponTablePath));

    if (WeaponTableObject)
    {
        FWeaponDataTable* WeaponDataRow = nullptr;
        switch (WeaponType)
        {
            case EWeaponType::EWT_SubmachineGun:
                WeaponDataRow = WeaponTableObject->FindRow<FWeaponDataTable>(FName("SubmachineGun"), TEXT(""));
                break;

            case EWeaponType::EWT_AssaultRifle:
                WeaponDataRow = WeaponTableObject->FindRow<FWeaponDataTable>(FName("AssaultRifle"), TEXT(""));
                break;

            case EWeaponType::EWT_Pistol:
                WeaponDataRow = WeaponTableObject->FindRow<FWeaponDataTable>(FName("Pistol"), TEXT(""));
                break;
        }

        if (WeaponDataRow)
        {
            AmmoType = WeaponDataRow->AmmoType;
            Ammo = WeaponDataRow->WeaponAmmo;
            MagazineCapacity = WeaponDataRow->MagazineCapacity;
            SetPickupSound(WeaponDataRow->PickupSound);
            SetEquipSound(WeaponDataRow->EquipSound);
            GetItemMesh()->SetSkeletalMesh(WeaponDataRow->ItemMesh);
            SetItemName(WeaponDataRow->ItemName);
            SetItemIcon(WeaponDataRow->InventoryIcon);
            SetQuantityIcon(WeaponDataRow->AmmoIcon);
            SetMaterialInstance(WeaponDataRow->MaterialInstance);

            PreviousMaterialIndex = GetMaterialIndex();
            GetItemMesh()->SetMaterial(PreviousMaterialIndex, nullptr);
            SetMaterialIndex(WeaponDataRow->MaterialIndex);
            SetClipBoneName(WeaponDataRow->ClipBoneName);
            SetReloadMontageSectionName(WeaponDataRow->ReloadMontageSection);
            GetItemMesh()->SetAnimInstanceClass(WeaponDataRow->AnimBP);
            
            CrosshairsMiddle = WeaponDataRow->CrosshairsMiddle;
            CrosshairsLeft = WeaponDataRow->CrosshairsLeft;
            CrosshairsRight = WeaponDataRow->CrosshairsRight;
            CrosshairsBottom = WeaponDataRow->CrosshairsBottom;
            CrosshairsTop = WeaponDataRow->CrosshairsTop;

            AutoFireRate = WeaponDataRow->AutoFireRate;
            MuzzleFlash = WeaponDataRow->MuzzleFlash;
            FireSound = WeaponDataRow->FireSound;

            BoneToHide = WeaponDataRow->BoneToHide;
            GetItemMesh()->HideBoneByName(BoneToHide, EPhysBodyOp::PBO_None);

            bAutomatic = WeaponDataRow->bAutomatic;
        }

        if (GetMaterialInstance())
        {
            SetDynamicMaterialInstance(UMaterialInstanceDynamic::Create(GetMaterialInstance(), this));
            GetDynamicMaterialInstance()->SetVectorParameterValue(TEXT("FresnelColor"), GetGlowColor());
            GetItemMesh()->SetMaterial(GetMaterialIndex(), GetDynamicMaterialInstance());

            EnableGlowMaterial();
        }
    }
}


void AWeapon::ThrowWeapon()
{
   EnsureWeaponIsUpright();

    const FVector MeshForewardVector{ GetItemMesh()->GetForwardVector() };
    const FVector MeshRightVector{ GetItemMesh()->GetRightVector() };

    // Direction in which character throws weapon
    FVector ImpulseDirection = MeshRightVector.RotateAngleAxis(-20.f, MeshForewardVector);

    float RandomRotation{ FMath::FRandRange(0, 30.f) };
    ImpulseDirection = ImpulseDirection.RotateAngleAxis(RandomRotation, FVector(0.f, 0.f, 1.f));
    ImpulseDirection *= 10'000.f;
    GetItemMesh()->AddImpulse(ImpulseDirection);

    bFalling = true;

    GetWorldTimerManager().SetTimer(
        ThrowWeaponTimer, 
        this, 
        &AWeapon::StopFalling, 
        ThrowWeaponTime);

    EnableGlowMaterial();
}

void AWeapon::StopFalling()
{
    bFalling = false;

    SetItemState(EItemState::EIS_Pickup);

    StartPulseTimer();
}

void AWeapon::EnsureWeaponIsUpright()
{
    const FRotator MeshRotation{ 0.f, GetItemMesh()->GetComponentRotation().Yaw, 0.f };
    GetItemMesh()->SetWorldRotation(MeshRotation, false, nullptr, ETeleportType::TeleportPhysics);
}

void AWeapon::DecrementAmmo()
{
    if (Ammo - 1 <= 0)
    {
        Ammo = 0;
    }
    else
    {
        Ammo--;
    }
}

bool AWeapon::TryGetBarrelSocketTransform(FTransform& OutBarrelSocketTransform) const
{
    if (GetItemMesh())
    {
        const USkeletalMeshSocket* BarrelSocket = GetItemMesh()->GetSocketByName("BarrelSocket");
        if (BarrelSocket)
        {
            OutBarrelSocketTransform = BarrelSocket->GetSocketTransform(GetItemMesh());

            return true;
        }
    }

    return false;
}

void AWeapon::ReloadAmmo(int32 Amount)
{
    checkf(Ammo + Amount <= MagazineCapacity, TEXT("Attempted to reload with more than magazine capacity"));
    Ammo += Amount;
}

bool AWeapon::IsClipFull() const 
{ 
    return Ammo >= MagazineCapacity; 
}

void AWeapon::StartSlideTimer()
{
    bMovingSlide = true;
    GetWorldTimerManager().SetTimer(
        SlideTimer, 
        this, 
        &AWeapon::FinishMovingSlide, 
        SlideDisplacementTime
    );
}

void AWeapon::FinishMovingSlide()
{
    bMovingSlide = false;
}

void AWeapon::UpdateSlideDisplacement()
{
    if (SlideDisplacementCurve && bMovingSlide)
    {
        const float ElapsedTime{ GetWorldTimerManager().GetTimerElapsed(SlideTimer) };

        const float CurveValue{ SlideDisplacementCurve->GetFloatValue(ElapsedTime) };

        SlideDisplacement = CurveValue * MaxSlideDisplacement;
        RecoilRotation = CurveValue * MaxRecoilRotation;
    }
}
