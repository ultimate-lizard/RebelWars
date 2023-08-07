#include "Items/ItemBase.h"

#include "Components/SphereComponent.h"
#include "Items/InventoryComponent.h"
#include "Net/UnrealNetwork.h"
#include "Perception/AIPerceptionStimuliSourceComponent.h"
#include "Perception/AISense_Sight.h"
#include "Characters/CombatCharacter.h"

AItemBase::AItemBase()
{
	PickupTraceSphere = CreateDefaultSubobject<USphereComponent>(FName(TEXT("Pickup Trace Sphere")));
	PickupTraceSphere->SetupAttachment(RootComponent);
	PickupTraceSphere->InitSphereRadius(60.0f);
	PickupTraceSphere->SetGenerateOverlapEvents(true);
	PickupTraceSphere->SetCollisionProfileName(FName(TEXT("Pickup")));
	PickupTraceSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

	PickupMesh = CreateDefaultSubobject<UStaticMeshComponent>(FName(TEXT("Item Pickup Mesh")));
	PickupMesh->SetupAttachment(PickupTraceSphere);
	PickupMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PickupMesh->SetGenerateOverlapEvents(false);

	AIPerceptionStimuliSource = CreateDefaultSubobject<UAIPerceptionStimuliSourceComponent>(FName(TEXT("AI Perception Stimuli")));

	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;

	SetReplicates(true);
	SetReplicateMovement(true);
	bNetUseOwnerRelevancy = true;

	CachedOwner = nullptr;
}

void AItemBase::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	AIPerceptionStimuliSource->RegisterForSense(UAISense_Sight::StaticClass());
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

void AItemBase::Pickup(class UInventoryComponent* InInventory)
{
	SetOwner(InInventory->GetOwner());

	ensure(PickupMesh);
	ensure(PickupTraceSphere);
	ensure(AIPerceptionStimuliSource);

	PickupMesh->SetVisibility(false);

	PickupTraceSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	AttachToActor(GetOwner(), FAttachmentTransformRules(EAttachmentRule::SnapToTarget, false));

	AIPerceptionStimuliSource->UnregisterFromPerceptionSystem();

	CachedOwner = Cast<ACombatCharacter>(GetOwner());

	SetActorTickEnabled(true);
}

void AItemBase::Drop()
{
	SetOwner(nullptr);

	ensure(PickupMesh);
	ensure(PickupTraceSphere);
	ensure(AIPerceptionStimuliSource);

	if (GetLocalRole() == ENetRole::ROLE_Authority)
	{
		DropOnGround();
	}

	PickupMesh->SetVisibility(true);

	AIPerceptionStimuliSource->RegisterWithPerceptionSystem();

	CachedOwner = nullptr;

	PickupTraceSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

	SetActorTickEnabled(false);
}

void AItemBase::DropOnGround()
{
	if (UWorld* World = GetWorld())
	{
		FHitResult HitResult;
		World->LineTraceSingleByObjectType(HitResult, GetActorLocation(), GetActorLocation() + FVector(0.0f, 0.0f, -1'000'000.0f), FCollisionObjectQueryParams(ECollisionChannel::ECC_WorldStatic));

		FRotator ItemRotator = GetActorRotation();
		ItemRotator.Pitch = 90.0f;
		ItemRotator.Roll = 90.0f;

		SetActorRotation(ItemRotator);

		FVector MeshMin;
		FVector MeshMax;

		PickupMesh->GetLocalBounds(MeshMin, MeshMax);

		float ItemWidth = FMath::Abs(MeshMin.X) + FMath::Abs(MeshMax.X);

		FVector ItemLocation = HitResult.Location;
		ItemLocation.Z += ItemWidth / 2.0f;

		SetActorLocation(ItemLocation, false, nullptr, ETeleportType::ResetPhysics);
	}

	DetachFromActor(FDetachmentTransformRules(EDetachmentRule::KeepWorld, false));
}
