#include "Items/ItemBase.h"

#include "Components/SphereComponent.h"
#include "Components/InteractableComponent.h"
#include "Items/InventoryComponent.h"
#include "Net/UnrealNetwork.h"
#include "Perception/AIPerceptionStimuliSourceComponent.h"
#include "Perception/AISense_Sight.h"
#include "Characters/CombatCharacter.h"

AItemBase::AItemBase()
{
	PickupMesh = CreateDefaultSubobject<UStaticMeshComponent>(FName(TEXT("Item Pickup Mesh")));
	PickupMesh->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
	PickupMesh->SetSimulatePhysics(true);
	PickupMesh->SetGenerateOverlapEvents(false);
	PickupMesh->SetEnableGravity(true);
	PickupMesh->SetCollisionProfileName(FName(TEXT("Pickup")));
	PickupMesh->CanCharacterStepUpOn = ECanBeCharacterBase::ECB_No;

	AIPerceptionStimuliSource = CreateDefaultSubobject<UAIPerceptionStimuliSourceComponent>(FName(TEXT("AI Perception Stimuli")));

	InteractableComponent = CreateDefaultSubobject<UInteractableComponent>(FName(TEXT("Interactable Component")));

	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	SetReplicates(true);
	SetReplicateMovement(true);
	bNetUseOwnerRelevancy = true;

	CachedOwner = nullptr;
}

void AItemBase::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	AIPerceptionStimuliSource->RegisterForSense(UAISense_Sight::StaticClass());

	InteractableComponent->OnInteract.AddDynamic(this, &AItemBase::OnInteract);
	InteractableComponent->Prompt = GetName();
}

void AItemBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AItemBase, Inventory);
}

UInventoryComponent* AItemBase::GetInventory()
{
	return Inventory;
}

UStaticMeshComponent* AItemBase::GetPickupMesh()
{
	return PickupMesh;
}

void AItemBase::OnInteract(AActor* Initiator)
{
	GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, TEXT("The player has interacted with an item, but nothing happened"));
}

void AItemBase::Pickup(class UInventoryComponent* InInventory)
{
	SetOwner(InInventory->GetOwner());

	ensure(PickupMesh);
	ensure(AIPerceptionStimuliSource);

	PickupMesh->SetVisibility(false);
	PickupMesh->SetSimulatePhysics(false);

	AttachToActor(GetOwner(), FAttachmentTransformRules(EAttachmentRule::SnapToTarget, false));

	AIPerceptionStimuliSource->UnregisterFromPerceptionSystem();

	CachedOwner = Cast<ACombatCharacter>(GetOwner());
}

void AItemBase::Drop()
{
	if (GetLocalRole() == ENetRole::ROLE_Authority)
	{
		SetOwner(nullptr);

		ensure(PickupMesh);
		ensure(AIPerceptionStimuliSource);

		PickupMesh->SetSimulatePhysics(true);

		AIPerceptionStimuliSource->RegisterWithPerceptionSystem();

		
	}

	DropOnGround();

	if (GetLocalRole() == ENetRole::ROLE_Authority)
	{
		CachedOwner = nullptr;
	}

	PickupMesh->SetVisibility(true);
}

void AItemBase::DropOnGround()
{
	DetachFromActor(FDetachmentTransformRules(EDetachmentRule::KeepWorld, false));

	if (!CachedOwner)
	{
		return;
	}

	FVector EyesLocation;
	FRotator EyesRotation;
	CachedOwner->GetActorEyesViewPoint(EyesLocation, EyesRotation);

	FRotator ItemRotator = GetActorRotation();
	ItemRotator.Yaw += 70.0f;
	SetActorRotation(ItemRotator, ETeleportType::ResetPhysics);

	FVector SpawnLocation = EyesLocation + EyesRotation.Vector() * 20.0f;

	SetActorLocation(SpawnLocation, false, nullptr, ETeleportType::ResetPhysics);

	if (GetLocalRole() == ENetRole::ROLE_Authority)
	{
		PickupMesh->SetPhysicsLinearVelocity(FVector::ZeroVector);
		PickupMesh->AddImpulse(EyesRotation.Vector() * 500.0f * PickupMesh->GetMass());
	}
}
