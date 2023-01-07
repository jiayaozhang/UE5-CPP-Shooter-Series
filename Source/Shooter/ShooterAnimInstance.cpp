// Fill out your copyright notice in the Description page of Project Settings.


#include "ShooterAnimInstance.h"
#include "ShooterCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Weapon.h"

UShooterAnimInstance::UShooterAnimInstance() :
    Speed(0.f),
    bIsInAir(false),
    bIsAccelerating(false),
    MovementOffsetYaw(0.f),
    LastMovementOffsetYaw(0.f),
    bAiming(false),
    TurnInPlaceCharacterYaw(0.f),
    TurnInPlaceCharacterYawLastFrame(0.f),
    RootYawOffset(0.f),
    RotationCurveValue(0.f),
    RotationCurveValueLastFrame(0.f),
    Pitch(0.f),
    bReloading(false),
    OffsetState(EOffsetState::EOS_Aiming),
    CharacterRotation(FRotator(0.f)),
    CharacterRotationLastFrame(FRotator(0.f)),
    LeaningYawDelta(0.f),
    bCrouching(false),
    RecoilWeight(1.f),
    bTurningInPlace(false),
    EquippedWeaponType(EWeaponType::EWT_MAX),
    bShouldUseFABRIK(false)
{
}

void UShooterAnimInstance::NativeInitializeAnimation()
{
    ShooterCharacter = Cast<AShooterCharacter>(TryGetPawnOwner());
}

void UShooterAnimInstance::UpdateAnimationProperties(float DeltaTime)
{
    if (ShooterCharacter == nullptr)
    {
        ShooterCharacter = Cast<AShooterCharacter>(TryGetPawnOwner());
    }

    if (ShooterCharacter)
    {
        bCrouching = ShooterCharacter->IsCrouching();

        bReloading = ShooterCharacter->GetCombatState() == ECombatState::ECS_Reloading;

        bEquipping = ShooterCharacter->GetCombatState() == ECombatState::ECS_Equipping;

        bShouldUseFABRIK = ShooterCharacter->GetCombatState() == ECombatState::ECS_Ready 
            || ShooterCharacter->GetCombatState() == ECombatState::ECS_FiringInProgress;

        // Get the speed of the character from velocity
        FVector Velocity{ ShooterCharacter->GetVelocity() };
        Velocity.Z = 0; // We just need the lateral velocity components, so Z isn't needed
        Speed = Velocity.Size();

        bIsInAir = ShooterCharacter->GetCharacterMovement()->IsFalling();

        // Check whether character is moving
        bIsAccelerating = ShooterCharacter->GetCharacterMovement()->GetCurrentAcceleration().Size() > 0.f;

        FRotator AimRotation = ShooterCharacter->GetBaseAimRotation();
        FRotator MovementRotation = UKismetMathLibrary::MakeRotFromX(ShooterCharacter->GetVelocity());
        MovementOffsetYaw = UKismetMathLibrary::NormalizedDeltaRotator(MovementRotation, AimRotation).Yaw;

        if (ShooterCharacter->GetVelocity().Size() > 0.f)
        {
            LastMovementOffsetYaw = MovementOffsetYaw;
        }

        bAiming = ShooterCharacter->IsAiming();

        // if (GEngine)
        // {
        //     FString RotationMessage = FString::Printf(TEXT("Base Aim Rotation: %f"), AimRotation.Yaw);
        //     GEngine->AddOnScreenDebugMessage(1, 0, FColor::White, RotationMessage);

        //     FString MovementRotationMessage = FString::Printf(TEXT("Movement Rotation: %f"), MovementRotation.Yaw);
        //     GEngine->AddOnScreenDebugMessage(2, 0, FColor::Green, MovementRotationMessage);

        //     FString OffsetMessage = FString::Printf(TEXT("Movement Offset Yaw: %f"), MovementOffsetYaw);
        //     GEngine->AddOnScreenDebugMessage(3, 0, FColor::Cyan, OffsetMessage);
        // }

        if (bReloading)
        {
            OffsetState = EOffsetState::EOS_Reloading;
        }
        else if (bIsInAir)
        {
            OffsetState = EOffsetState::EOS_InAir;
        }
        else if (bAiming)
        {
            OffsetState = EOffsetState::EOS_Aiming;
        }
        else
        {
            OffsetState = EOffsetState::EOS_Hip;
        }

        if (ShooterCharacter->GetEquippedWeapon())
        {
            EquippedWeaponType = ShooterCharacter->GetEquippedWeapon()->GetWeaponType();
        }
    }

    TurnInPlace();

    Lean(DeltaTime);
}

void UShooterAnimInstance::TurnInPlace()
{
    if (!ShooterCharacter)
    {
        return;
    }

    Pitch = ShooterCharacter->GetBaseAimRotation().Pitch;

    if (Speed > 0 || bIsInAir)
    {
        // Don't turn in place because character is moving
        RootYawOffset = 0.f;
        TurnInPlaceCharacterYaw = ShooterCharacter->GetActorRotation().Yaw;
        TurnInPlaceCharacterYawLastFrame = TurnInPlaceCharacterYaw;
        RotationCurveValueLastFrame = 0.f;
        RotationCurveValue = 0.f;
    }
    else
    {
        TurnInPlaceCharacterYawLastFrame = TurnInPlaceCharacterYaw;
        TurnInPlaceCharacterYaw = ShooterCharacter->GetActorRotation().Yaw;

        const float TurnInPlaceYawDelta{ TurnInPlaceCharacterYaw - TurnInPlaceCharacterYawLastFrame };

        // Root yaw offset updated and clamped [-180, 180]
        RootYawOffset = UKismetMathLibrary::NormalizeAxis(RootYawOffset - TurnInPlaceYawDelta);

        const float TurningCurveValue{ GetCurveValue(TEXT("Turning")) };

        // 1.0 if turning; 0 if not
        bTurningInPlace = TurningCurveValue > 0.f;

        if (bTurningInPlace)
        {
            RotationCurveValueLastFrame = RotationCurveValue;
            RotationCurveValue = GetCurveValue(TEXT("Rotation"));

            const float DeltaRotation{ RotationCurveValue - RotationCurveValueLastFrame };

            // If root yaw offset is positive, we're turning left
            // but if root yaw offset is negative, we're turning right
            RootYawOffset = (RootYawOffset > 0) 
                ? RootYawOffset - DeltaRotation 
                : RootYawOffset + DeltaRotation;

            const float AbsoluteRootYawOffset = FMath::Abs(RootYawOffset);
            if (AbsoluteRootYawOffset > 90.f)
            {
                const float ExcessYaw{ AbsoluteRootYawOffset - 90.f };
                RootYawOffset = (RootYawOffset > 0) 
                    ? RootYawOffset - ExcessYaw 
                    : RootYawOffset + ExcessYaw;
            }
        }

        // if (GEngine)
        // {
        //     GEngine->AddOnScreenDebugMessage(
        //         1, 
        //         -1,
        //         FColor::Green, 
        //         FString::Printf(TEXT("CharacterYaw: %f"), TurnInPlaceCharacterYaw)
        //     );

        //     GEngine->AddOnScreenDebugMessage(
        //         2, 
        //         -1,
        //         FColor::Yellow, 
        //         FString::Printf(TEXT("RootYawOffset: %f"), RootYawOffset)
        //     );
        // }
    }

    // TODO: Move to a function. This is so hard to read :-(
    // Calculate recoil weight
    if (bTurningInPlace)
    {
        if (bReloading || bEquipping)
        {
            RecoilWeight = 1.f;
        }
        else
        {
            RecoilWeight = 0.f;
        }
    }
    else // not turning in place
    {
        if (bCrouching)
        {
            if (bReloading || bEquipping)
            {
                RecoilWeight = 1.f;
            }
            else
            {
                RecoilWeight = 0.1f;
            }
        }
        else
        {
            if (bAiming || bReloading || bEquipping)
            {
                RecoilWeight = 1.f;
            }
            else
            {
                RecoilWeight = 0.5f;
            }
        }
    }    
}

void UShooterAnimInstance::Lean(float DeltaTime)
{
    if (!ShooterCharacter)
    {
        return;
    }

    CharacterRotationLastFrame = CharacterRotation;
    CharacterRotation = ShooterCharacter->GetActorRotation();

    const FRotator DeltaRotation = UKismetMathLibrary::NormalizedDeltaRotator(
        CharacterRotation, 
        CharacterRotationLastFrame);

    float TargetYawRateOfChange{ ((float) DeltaRotation.Yaw) / DeltaTime };

    const float Interp{ FMath::FInterpTo(LeaningYawDelta, TargetYawRateOfChange, DeltaTime, 6.f) };

    LeaningYawDelta = FMath::Clamp(Interp, -90.f, 90.f);

    // if (GEngine)
    // {
    //     GEngine->AddOnScreenDebugMessage(
    //         3,
    //         -1,
    //         FColor::Cyan,
    //         FString::Printf(TEXT("LeaningYawDelta %f"), LeaningYawDelta)
    //     );

    //     GEngine->AddOnScreenDebugMessage(
    //         4,
    //         -1,
    //         FColor::Cyan,
    //         FString::Printf(TEXT("DeltaRotation.Yaw %f"), DeltaRotation.Yaw)
    //     );
    // }
}
