#include "Characters/CombatCharacter.h"

#include "GameFramework/PawnMovementComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/InventoryComponent.h"
#include "Items/Firearm.h"
#include "Particles/ParticleSystem.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "Camera/CameraComponent.h"
#include "Net/UnrealNetwork.h"
#include "Components/CapsuleComponent.h"
#include "Components/InteractableComponent.h"
#include "Player/RWPlayerStart.h"
#include "AIController.h"
#include "Controllers/HumanPlayerController.h"
#include "Kismet/KismetMathLibrary.h"

ACombatCharacter::ACombatCharacter()
{
	// TODO: Research on constructor pointer checks

	PrimaryActorTick.bCanEverTick = true;

	InventoryComponent = CreateDefaultSubobject<UInventoryComponent>(FName(TEXT("Inventory")));

	WeaponMesh1P = CreateDefaultSubobject<USkeletalMeshComponent>(FName(TEXT("Weapon Mesh 1P")));
	WeaponMesh1P->SetupAttachment(GetRootComponent());
	WeaponMesh1P->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponMesh1P->SetVisibility(false);
	WeaponMesh1P->CastShadow = false;
	WeaponMesh1P->bOnlyOwnerSee = true;

	HandsMesh1P = CreateDefaultSubobject<USkeletalMeshComponent>(FName(TEXT("Hands Mesh 1P")));
	HandsMesh1P->SetupAttachment(WeaponMesh1P);
	HandsMesh1P->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HandsMesh1P->SetVisibility(false);
	HandsMesh1P->CastShadow = false;
	HandsMesh1P->bOnlyOwnerSee = true;
	HandsMesh1P->MasterPoseComponent = WeaponMesh1P;
	HandsMesh1P->bUseAttachParentBound = true;

	WeaponMesh3P = CreateDefaultSubobject<UStaticMeshComponent>(FName(TEXT("Weapon Mesh 3P")));
	WeaponMesh3P->SetupAttachment(GetMesh());
	WeaponMesh3P->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponMesh3P->SetVisibility(false);
	WeaponMesh3P->CastShadow = false;
	WeaponMesh3P->bOwnerNoSee = true;

	GetMesh()->bOwnerNoSee = true;

	MaxHealth = 100.0f;
	CurrentHealth = MaxHealth;
	bIsDead = false;
	AccumulatedDamage = 0.0f;
	MaxRunSpeed = 600.0f;
	MaxWalkSpeed = 150.0f;

	bReplicates = true;
	SetReplicateMovement(true);
	SetActorTickEnabled(true);

	Tags.Add(FName(TEXT("character")));

	Affiliation = EAffiliation::Neutrals;
	MovementType = ECharacterMovementType::Run;

	TargetActorRotation = GetActorRotation();
}

void ACombatCharacter::SetPlayerDefaults()
{
	Super::SetPlayerDefaults();

	if (GetLocalRole() == ENetRole::ROLE_Authority)
	{
		SetHealth(MaxHealth);
		MovementType = ECharacterMovementType::Run;
	}
}

void ACombatCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);

	float VelocityZ = GetVelocity().Z / 2.0f;

	if (AHumanPlayerController* HumanController = GetController<AHumanPlayerController>())
	{
		HumanController->AddViewPunch(FRotator(0.0f, 0.0f, VelocityZ));
	}
}

void ACombatCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ensure(InventoryComponent);

	InventoryComponent->OnFirearmEquipDelegate.RemoveAll(this);
	InventoryComponent->OnFirearmUnequipDelegate.RemoveAll(this);
}

void ACombatCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	if (!NewController)
	{
		return;
	}

	if (ARWPlayerStart* ActorStartSpot = Cast<ARWPlayerStart>(NewController->StartSpot.Get()))
	{
		Affiliation = ActorStartSpot->PlayerStartAffiliation;
	}
}

void ACombatCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	ensure(InventoryComponent);
	InventoryComponent->OnFirearmEquipDelegate.AddDynamic(this, &ThisClass::FirearmEquip);
	InventoryComponent->OnFirearmUnequipDelegate.AddDynamic(this, &ThisClass::FirearmUnequip);
}

void ACombatCharacter::BeginPlay()
{
	Super::BeginPlay();

	FGenericTeamId Team(UTeamStatics::GetTeamIdFromAffiliation(Affiliation));
	Team.SetAttitudeSolver(&UTeamStatics::SolveAttitudeImpl);

	SetGenericTeamId(Team);
}

void ACombatCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ACombatCharacter, CurrentHealth);
	DOREPLIFETIME(ACombatCharacter, MovementType);
	DOREPLIFETIME(ACombatCharacter, HeadRotation);
	DOREPLIFETIME(ACombatCharacter, TargetActorRotation);
	DOREPLIFETIME(ACombatCharacter, LastDamageCauser);
}

void ACombatCharacter::AttachWeaponMesh(AFirearm* InFirearm)
{
	ensure(WeaponMesh3P);
	WeaponMesh3P->SetStaticMesh(InFirearm->Mesh3P);
	WeaponMesh3P->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, FName(TEXT("weapon")));
	WeaponMesh1P->SetSkeletalMesh(InFirearm->Mesh1P);
}

USkeletalMeshComponent* ACombatCharacter::GetHandsMesh1P()
{
	return HandsMesh1P;
}

USkeletalMeshComponent* ACombatCharacter::GetWeaponMesh1P()
{
	return WeaponMesh1P;
}

UStaticMeshComponent* ACombatCharacter::GetWeaponMesh3P()
{
	return WeaponMesh3P;
}

bool ACombatCharacter::IsArmed() const
{
	check(InventoryComponent);
	return InventoryComponent->GetFirearm(EInventorySlot::Primary) || InventoryComponent->GetFirearm(EInventorySlot::Sidearm);
}

void ACombatCharacter::SetMovementType_Implementation(ECharacterMovementType InMovementType)
{
	MovementType = InMovementType;

	// BroadcastUpdateMovement();
}

ECharacterMovementType ACombatCharacter::GetMovementType() const
{
	return MovementType;
}

float ACombatCharacter::GetCurrentHealth() const
{
	return CurrentHealth;
}

bool ACombatCharacter::IsDead() const
{
	return bIsDead;
}

void ACombatCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	ViewModelSwayCycle = GetVelocity().Size();

	/*if (ViewModelSwayCycle >= 3.0f)
	{
		ViewModelSwayCycle = 0.0f;
	}*/

	FString ViewModelSwayCycleStr = FString::Printf(TEXT("%f"), ViewModelSwayCycle);
	GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Green, *ViewModelSwayCycleStr);

	UpdateBodyRotation(DeltaTime);
	UpdateViewModelTransform(DeltaTime);
	TraceInteractables();
}

void ACombatCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	ensure(PlayerInputComponent);

	PlayerInputComponent->BindAxis(TEXT("MoveForward"), this, &ThisClass::MoveForward);
	PlayerInputComponent->BindAxis(TEXT("MoveRight"), this, &ThisClass::MoveRight);
	PlayerInputComponent->BindAxis(TEXT("LookUp"), this, &ThisClass::LookUp);
	PlayerInputComponent->BindAxis(TEXT("Turn"), this, &ThisClass::Turn);

	PlayerInputComponent->BindAction(TEXT("PrimaryFire"), EInputEvent::IE_Pressed, this, &ThisClass::StartPrimaryFire);
	PlayerInputComponent->BindAction(TEXT("PrimaryFire"), EInputEvent::IE_Released, this, &ThisClass::StopPrimaryFire);
	PlayerInputComponent->BindAction(TEXT("Reload"), EInputEvent::IE_Pressed, this, &ThisClass::Reload);
	PlayerInputComponent->BindAction(TEXT("DropWeapon"), EInputEvent::IE_Pressed, this, &ThisClass::DropFirearm);

	PlayerInputComponent->BindAction(TEXT("Use"), EInputEvent::IE_Pressed, this, &ThisClass::Use);

	PlayerInputComponent->BindAction(TEXT("SelectWeaponSlot0"), EInputEvent::IE_Pressed, this, &ThisClass::SelectWeaponSlot<0>);
	PlayerInputComponent->BindAction(TEXT("SelectWeaponSlot1"), EInputEvent::IE_Pressed, this, &ThisClass::SelectWeaponSlot<1>);
	PlayerInputComponent->BindAction(TEXT("SelectWeaponSlot2"), EInputEvent::IE_Pressed, this, &ThisClass::SelectWeaponSlot<2>);
	PlayerInputComponent->BindAction(TEXT("SelectWeaponSlot3"), EInputEvent::IE_Pressed, this, &ThisClass::SelectWeaponSlot<3>);
}

void ACombatCharacter::SetGenericTeamId(const FGenericTeamId& InTeamId)
{
	TeamId = InTeamId;
}

FGenericTeamId ACombatCharacter::GetGenericTeamId() const
{
	return TeamId;
}

void ACombatCharacter::StartPrimaryFire()
{
	ensure(InventoryComponent);

	if (AFirearm* CurrentFirearm = InventoryComponent->GetEquippedFirearm())
	{
		CurrentFirearm->StartPrimaryFire();
	}
}

void ACombatCharacter::StopPrimaryFire()
{
	if (!InventoryComponent)
	{
		return;
	}

	if (AFirearm* CurrentFirearm = InventoryComponent->GetEquippedFirearm())
	{
		CurrentFirearm->StopPrimaryFire();
	}
}

void ACombatCharacter::DropFirearm()
{
	ensure(InventoryComponent);
	if (AFirearm* CurrentFirearm = InventoryComponent->GetEquippedFirearm())
	{
		InventoryComponent->DropFirearm(CurrentFirearm);
	}
}

void ACombatCharacter::Use()
{
	if (GetLocalRole() < ENetRole::ROLE_Authority)
	{
		ServerUse();
		return;
	}

	if (GetLocalRole() == ENetRole::ROLE_Authority)
	{
		if (FocusedInteractable)
		{
			FocusedInteractable->Interact(this);
		}
	}
}

void ACombatCharacter::SelectWeaponSlot(int32 Index)
{
	ensure(InventoryComponent);
	InventoryComponent->EquipFirearm(static_cast<EInventorySlot>(Index));
}

float ACombatCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (GetWorldTimerManager().IsTimerActive(DamageAccumulationTimer))
	{
		AccumulatedDamage += DamageAmount;
		FString AccumulatedDamageStr = FString::Printf(TEXT("Accumulated damage: %f"), AccumulatedDamage);
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Green, *AccumulatedDamageStr);
	}
	else
	{
		GetWorldTimerManager().SetTimer(DamageAccumulationTimer, 0.1f, false);
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Green, TEXT("The timer has been reset"));
		AccumulatedDamage = DamageAmount;
	}

	SetHealth(GetCurrentHealth() - DamageAmount);
	if (IsDead())
	{
		FHitResult HitResult;

		FVector ImpulseDirection;
		DamageEvent.GetBestHitInfo(this, EventInstigator ? EventInstigator->GetPawn() : nullptr, HitResult, ImpulseDirection);

		static const float BaseForce = 1000.0f;

		BroadcastBecomeRagdoll(ImpulseDirection * AccumulatedDamage * BaseForce, HitResult.Location);
	}

	return Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
}

void ACombatCharacter::BroadcastBecomeRagdoll_Implementation(FVector ImpulseDirection, FVector ImpulseLocation)
{
	if (USkeletalMeshComponent* CharacterMesh = GetMesh())
	{
		if (!IsRagdoll())
		{
			SetRagdollEnabled(true);
		}
	}

	if (USkeletalMeshComponent* RagdollMesh = GetMesh())
	{
		FName ImpactBone = RagdollMesh->FindClosestBone(ImpulseLocation, nullptr, 0.0f, true);
		RagdollMesh->AddImpulseAtLocation(ImpulseDirection, ImpulseLocation, ImpactBone); // TODO: PHSYCICS

		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Green, *ImpactBone.ToString());
	}
}

void ACombatCharacter::Kill()
{
	if (GetLocalRole() == ENetRole::ROLE_Authority)
	{
		InventoryComponent->DropAll();
	}

	bIsDead = true;

	// TODO: Play animation
	// SetRagdollEnabled(true);

	OnKillDelegate.Broadcast(nullptr, this);
}

void ACombatCharacter::Resurrect()
{
	bIsDead = false;

	SetRagdollEnabled(false);

	OnResurrectDelegate.Broadcast(this);
}

void ACombatCharacter::MoveForward(float InRate)
{
	FRotator ControlRotator = GetControlRotation();
	ControlRotator.Pitch = 0.0f;
	AddMovementInput(ControlRotator.Quaternion().GetForwardVector(), InRate);
}

void ACombatCharacter::MoveRight(float InRate)
{
	FRotator ControlRotator = GetControlRotation();
	ControlRotator.Pitch = 0.0f;
	AddMovementInput(ControlRotator.Quaternion().GetRightVector(), InRate);
}

void ACombatCharacter::LookUp(float InRate)
{
	AddControllerPitchInput(InRate);
}

void ACombatCharacter::Turn(float InRate)
{
	AddControllerYawInput(InRate);
}

void ACombatCharacter::Reload()
{
	if (AFirearm* CurrentFirearm = InventoryComponent->GetEquippedFirearm())
	{
		CurrentFirearm->Reload();
	}
}

void ACombatCharacter::UpdateViewModelTransform(float DeltaTime)
{
	ensure(WeaponMesh1P);

	FVector EyesLocation;
	FRotator EyesRotation;
	GetActorEyesViewPoint(EyesLocation, EyesRotation);

	FVector NewPosition = EyesLocation + EyesRotation.RotateVector(ViewModelOffset);
	FRotator NewRotation = FRotator(FQuat(EyesRotation) * FQuat(FRotator::ZeroRotator) * FQuat(FRotator(0.0f, 90.0f, 0.0f)));

	WeaponMesh1P->SetWorldLocationAndRotationNoPhysics(NewPosition, NewRotation);
}

void ACombatCharacter::UpdateBodyRotation(float DeltaTime)
{
	if (GetLocalRole() == ENetRole::ROLE_Authority)
	{
		HeadRotation = GetControlRotation();

		FRotator HeadVsBodyDelta = UKismetMathLibrary::NormalizedDeltaRotator(HeadRotation, GetActorRotation());

		if (GetVelocity() != FVector::ZeroVector)
		{
			FRotator ControlRotator = HeadRotation;
			FRotator ActorRotator = GetActorRotation();

			ActorRotator.Yaw = ControlRotator.Yaw;

			TargetActorRotation = ActorRotator;
		}
		else if (HeadVsBodyDelta.Yaw < -90.0f || HeadVsBodyDelta.Yaw > 90.0f)
		{
			FRotator ControlRotator = HeadRotation;
			FRotator ActorRotator = GetActorRotation();

			ActorRotator.Yaw = ControlRotator.Yaw;
			TargetActorRotation = ActorRotator;
		}
	}

	FRotator ActorRotator = GetActorRotation();
	if (ActorRotator != TargetActorRotation)
	{
		FRotator CurrentRotation = FMath::RInterpTo(ActorRotator, TargetActorRotation, DeltaTime, 10.0f);
		FRotator PreviousRotation = GetActorRotation();

		SetActorRotation(CurrentRotation);
	}
}

void ACombatCharacter::TraceInteractables()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	FVector EyesLocation;
	FRotator EyesRotation;
	GetActorEyesViewPoint(EyesLocation, EyesRotation);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	const float MaxTraceDistance = 300.0f;

	TArray<FHitResult> Hits;
	FVector TraceDestination = EyesLocation + EyesRotation.Vector() * MaxTraceDistance;
	World->LineTraceMultiByChannel(Hits, EyesLocation, TraceDestination, ECollisionChannel::ECC_GameTraceChannel1, QueryParams);

	FocusedInteractable = nullptr;

	for (const FHitResult& Hit : Hits)
	{
		if (Hit.Actor.IsValid())
		{
			if (UInteractableComponent* Interactable = Hit.Actor->FindComponentByClass<UInteractableComponent>())
			{
				FocusedInteractable = Interactable;
			}
		}
	}
}

void ACombatCharacter::FirearmEquip(AFirearm* InFirearm)
{
	if (!InFirearm)
	{
		return;
	}

	AttachWeaponMesh(InFirearm);

	ensure(HandsMesh1P);
	ensure(WeaponMesh1P);
	ensure(WeaponMesh3P);

	HandsMesh1P->SetVisibility(true);
	WeaponMesh1P->SetVisibility(true);
	WeaponMesh3P->SetVisibility(true);

	if (UAnimInstance* AnimInstance1P = WeaponMesh1P->GetAnimInstance())
	{
		AnimInstance1P->Montage_Play(InFirearm->FirearmAnimations.Deploy);
		AnimInstance1P->Montage_JumpToSection(FName(TEXT("Default")), InFirearm->FirearmAnimations.Deploy);
	}
}

void ACombatCharacter::FirearmUnequip(AFirearm* InFirearm)
{
	ensure(HandsMesh1P);
	ensure(WeaponMesh1P);
	ensure(WeaponMesh3P);

	HandsMesh1P->SetVisibility(false);
	WeaponMesh1P->SetVisibility(false);
	WeaponMesh3P->SetVisibility(false);
}

void ACombatCharacter::OnRep_CurrentHealth()
{
	OnHealthUpdate();
}

void ACombatCharacter::OnHealthUpdate()
{
	if (CurrentHealth <= 0.0f && !IsDead())
	{
		Kill();
	}
	else if (CurrentHealth > 0.0f && IsDead())
	{
		Resurrect();
	}
}

void ACombatCharacter::SetHealth(float NewHealth)
{
	CurrentHealth = NewHealth;
	OnHealthUpdate();
}

void ACombatCharacter::SetRagdollEnabled(bool bEnableRagdoll)
{
	if (USkeletalMeshComponent* CharacterMesh = GetMesh())
	{
		CharacterMesh->SetCollisionEnabled(bEnableRagdoll ? ECollisionEnabled::PhysicsOnly : ECollisionEnabled::NoCollision);
		CharacterMesh->SetAllBodiesSimulatePhysics(bEnableRagdoll ? true : false);

		if (bEnableRagdoll)
		{
			CharacterMesh->WakeAllRigidBodies();
		}
		else
		{
			CharacterMesh->PutAllRigidBodiesToSleep();
		}

		CharacterMesh->SetOwnerNoSee(bEnableRagdoll ? false : true);

		if (UCapsuleComponent* Capsule = GetCapsuleComponent())
		{
			Capsule->SetCollisionEnabled(bEnableRagdoll ? ECollisionEnabled::NoCollision : ECollisionEnabled::QueryAndPhysics);

			if (!bEnableRagdoll)
			{
				CharacterMesh->AttachTo(Capsule);
				CharacterMesh->SetRelativeLocationAndRotation(FVector(0.0f, 0.0f, 25.0f), FQuat::MakeFromEuler(FVector(0.0f, 0.0f, -90.0f)));
			}
		}
	}

	if (AController* CurrentController = GetController())
	{
		CurrentController->SetIgnoreMoveInput(bEnableRagdoll ? true : false);
	}
}

bool ACombatCharacter::IsRagdoll() const
{
	if (USkeletalMeshComponent* CharacterMesh = GetMesh())
	{
		return CharacterMesh->IsSimulatingPhysics();
	}

	return false;
}

//void ACombatCharacter::BroadcastUpdateMovement_Implementation()
//{
//	if (UCharacterMovementComponent* CombatCharacterMovement = GetCharacterMovement())
//	{
//		switch (MovementType)
//		{
//		case ECharacterMovementType::Walk:
//			CombatCharacterMovement->MaxWalkSpeed = MaxWalkSpeed;
//			break;
//		case ECharacterMovementType::Run:
//			CombatCharacterMovement->MaxWalkSpeed = MaxRunSpeed;
//			break;
//		default:
//			CombatCharacterMovement->MaxWalkSpeed = MaxRunSpeed;
//			break;
//		}
//	}
//}

void ACombatCharacter::ServerUse_Implementation()
{
	Use();
}
