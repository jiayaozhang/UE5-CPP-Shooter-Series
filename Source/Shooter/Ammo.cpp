// Fill out your copyright notice in the Description page of Project Settings.


#include "Ammo.h"
#include "Components/BoxComponent.h"
#include "Components/WidgetComponent.h"
#include "Components/SphereComponent.h"
#include "ShooterCharacter.h"

AAmmo::AAmmo()
{
    AmmoMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AmmoMesh"));
    SetRootComponent(AmmoMesh);

    GetCollisionBox()->SetupAttachment(GetRootComponent());
    GetPickupWidget()->SetupAttachment(GetRootComponent());
    GetAreaSphere()->SetupAttachment(GetRootComponent());

    AmmoCollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("AmmoCollisionSphere"));
    AmmoCollisionSphere->SetupAttachment(GetRootComponent());
    AmmoCollisionSphere->SetSphereRadius(50.f);

}

void AAmmo::BeginPlay()
{
    Super::BeginPlay();

    AmmoCollisionSphere->OnComponentBeginOverlap.AddDynamic(this, &AAmmo::OnAmmoSphereOverlap);
}

void AAmmo::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

void AAmmo::SetItemProperties(EItemState State)
{
    Super::SetItemProperties(State);

    switch (State)
	{
		case EItemState::EIS_Pickup:
			AmmoMesh->SetSimulatePhysics(false);
			AmmoMesh->SetVisibility(true);
			AmmoMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
			AmmoMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			AmmoMesh->SetEnableGravity(false);
			break;

		case EItemState::EIS_Equipped:
			AmmoMesh->SetSimulatePhysics(false);
			AmmoMesh->SetVisibility(true);
			AmmoMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
			AmmoMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			AmmoMesh->SetEnableGravity(false);
			break;

		case EItemState::EIS_Falling:
			AmmoMesh->SetSimulatePhysics(true);
			AmmoMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			AmmoMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
			AmmoMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECollisionResponse::ECR_Block);
			AmmoMesh->SetEnableGravity(true);
			break;

		case EItemState::EIS_EquipInterpolating:
			AmmoMesh->SetSimulatePhysics(false);
			AmmoMesh->SetVisibility(true);
			AmmoMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
			AmmoMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			AmmoMesh->SetEnableGravity(false);
			break;
	}
}

void AAmmo::OnAmmoSphereOverlap(
    UPrimitiveComponent* OverlappedComponent, 
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
     
    auto OverlappedCharacter = Cast<AShooterCharacter>(OtherActor);
    if (OverlappedCharacter)
    {
        StartItemCurve(OverlappedCharacter);
        AmmoCollisionSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }
}

void AAmmo::EnableCustomDepth()
{
	AmmoMesh->SetRenderCustomDepth(true);
}

void AAmmo::DisableCustomDepth()
{
	AmmoMesh->SetRenderCustomDepth(false);
}
