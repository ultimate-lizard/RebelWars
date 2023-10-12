#include "Characters/CombatCharacter.h"

#include "GameFramework/PawnMovementComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Items/InventoryComponent.h"
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
	MaxRunSpeed = 600.0f;
	MaxWalkSpeed = 150.0f;

	bReplicates = true;
	SetReplicateMovement(true);

	Tags.Add(FName(TEXT("character")));

	Affiliation = EAffiliation::Neutrals;

	TargetRotation = GetActorRotation();
}

void ACombatCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ensure(InventoryComponent);

	InventoryComponent->OnFirearmPickupDelegate.RemoveAll(this);
	InventoryComponent->OnFirearmDropDelegate.RemoveAll(this);
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

	FGenericTeamId Team(UTeamStatics::GetTeamIdFromAffiliation(Affiliation));
	Team.SetAttitudeSolver(&UTeamStatics::SolveAttitudeImpl);

	SetGenericTeamId(Team);
}

void ACombatCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	ensure(InventoryComponent);
	InventoryComponent->OnFirearmPickupDelegate.AddDynamic(this, &ThisClass::FirearmPickup);
	InventoryComponent->OnFirearmDropDelegate.AddDynamic(this, &ThisClass::FirearmDrop);
}

void ACombatCharacter::BeginPlay()
{
	Super::BeginPlay();
}

void ACombatCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ACombatCharacter, CurrentHealth);
	DOREPLIFETIME(ACombatCharacter, MovementType);
	DOREPLIFETIME(ACombatCharacter, HeadRotation);
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
	return InventoryComponent->GetPrimaryFirearm() != nullptr;
}

void ACombatCharacter::SetMovementType_Implementation(ECharacterMovementType InMovementType)
{
	MovementType = InMovementType;

	BroadcastUpdateMovement();
}

ECharacterMovementType ACombatCharacter::GetMovementType() const
{
	return MovementType;
}

float ACombatCharacter::GetCurrentHealth() const
{
	return CurrentHealth;
}

void ACombatCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdateViewModelTransform();
	UpdateBodyRotation(DeltaTime);
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

	if (AFirearm* CurrentFirearm = InventoryComponent->GetPrimaryFirearm())
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

	if (AFirearm* CurrentFirearm = InventoryComponent->GetPrimaryFirearm())
	{
		CurrentFirearm->StopPrimaryFire();
	}
}

void ACombatCharacter::DropFirearm()
{
	ensure(InventoryComponent);
	if (AFirearm* CurrentFirearm = InventoryComponent->GetPrimaryFirearm())
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

float ACombatCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	return 0.0f;

	if (GetLocalRole() == ENetRole::ROLE_Authority)
	{
		CurrentHealth -= DamageAmount;
		OnHealthUpdate();
	}

	return DamageAmount;
}

void ACombatCharacter::Kill()
{
	if (AFirearm* CurrentFirearm = InventoryComponent->GetPrimaryFirearm())
	{
		InventoryComponent->DropFirearm(CurrentFirearm);
	}

	InventoryComponent->OnFirearmPickupDelegate.RemoveAll(this);
	InventoryComponent->OnFirearmDropDelegate.RemoveAll(this);

	SetReplicatingMovement(false);
	SetActorTickEnabled(false);

	if (USkeletalMeshComponent* CharacterMesh = GetMesh())
	{
		CharacterMesh->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
		CharacterMesh->SetCollisionProfileName(FName(TEXT("Ragdoll")));
		CharacterMesh->SetSimulatePhysics(true);
		CharacterMesh->SetAllBodiesSimulatePhysics(true);
		CharacterMesh->WakeAllRigidBodies();
		CharacterMesh->SetOwnerNoSee(false);
	}

	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	if (AController* CurrentController = GetController())
	{
		CurrentController->SetIgnoreMoveInput(true);
	}
}

void ACombatCharacter::DebugDropWeapon()
{
	DropFirearm();
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
	if (AFirearm* CurrentFirearm = InventoryComponent->GetPrimaryFirearm())
	{
		CurrentFirearm->Reload();
	}
}

void ACombatCharacter::UpdateViewModelTransform()
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
	if (GetLocalRole() != ENetRole::ROLE_Authority)
	{
		return;
	}

	HeadRotation = GetControlRotation();

	FRotator HeadVsBodyDelta = UKismetMathLibrary::NormalizedDeltaRotator(HeadRotation, GetActorRotation());

	if (GetVelocity() != FVector::ZeroVector)
	{
		FRotator ControlRotator = HeadRotation;
		FRotator ActorRotator = GetActorRotation();

		ActorRotator.Yaw = ControlRotator.Yaw;

		TargetRotation = ActorRotator;
	}
	else if (HeadVsBodyDelta.Yaw < -90.0f || HeadVsBodyDelta.Yaw > 90.0f)
	{
		FRotator ControlRotator = HeadRotation;
		FRotator ActorRotator = GetActorRotation();

		ActorRotator.Yaw = ControlRotator.Yaw;
		TargetRotation = ActorRotator;
	}

	FRotator ActorRotator = GetActorRotation();
	if (ActorRotator != TargetRotation)
	{
		FRotator CurrentRotation = FMath::RInterpTo(ActorRotator, TargetRotation, DeltaTime, 10.0f);
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

void ACombatCharacter::FirearmPickup(AFirearm* InFirearm)
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

void ACombatCharacter::FirearmDrop(AFirearm* InFirearm)
{
	ensure(HandsMesh1P);
	ensure(WeaponMesh1P);
	ensure(WeaponMesh3P);

	HandsMesh1P->SetVisibility(false);
	WeaponMesh1P->SetVisibility(false);
	WeaponMesh3P->SetVisibility(false);

	if (!InFirearm)
	{
		return;
	}
}

void ACombatCharacter::OnRep_CurrentHealth()
{
	OnHealthUpdate();
}

void ACombatCharacter::OnHealthUpdate()
{
	if (CurrentHealth <= 0.0f)
	{
		Kill();
	}
}

void ACombatCharacter::BroadcastUpdateMovement_Implementation()
{
	if (UCharacterMovementComponent* CombatCharacterMovement = GetCharacterMovement())
	{
		switch (MovementType)
		{
		case ECharacterMovementType::Walk:
			CombatCharacterMovement->MaxWalkSpeed = MaxWalkSpeed;
			break;
		case ECharacterMovementType::Run:
			CombatCharacterMovement->MaxWalkSpeed = MaxRunSpeed;
			break;
		default:
			CombatCharacterMovement->MaxWalkSpeed = MaxRunSpeed;
			break;
		}
	}
}

void ACombatCharacter::ServerPrimaryFire_Implementation(AFirearm* InFirearm)
{
	if (!InFirearm)
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		FVector ViewLocation;
		FRotator ViewRotation;
		GetActorEyesViewPoint(ViewLocation, ViewRotation);

		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(this);

		TArray<FHitResult> HitResults;
		if (World->LineTraceMultiByChannel(HitResults, ViewLocation, ViewLocation + ViewRotation.Vector() * 30'000.0f, ECollisionChannel::ECC_WorldStatic, QueryParams))
		{
			for (const FHitResult& HitResult : HitResults)
			{
				if (HitResult.Actor.Get() != this)
				{
					if (HitResult.Actor.IsValid() && HitResult.Actor.Get() && HitResult.Actor->GetClass()->IsChildOf<APawn>())
					{
						const float BaseDamage = 30.0f;

						UGameplayStatics::ApplyDamage(HitResult.Actor.Get(), BaseDamage, GetController(), this, UDamageType::StaticClass());
						break;
					}
				}
			}
		}
	}
}

void ACombatCharacter::ServerUse_Implementation()
{
	Use();
}
