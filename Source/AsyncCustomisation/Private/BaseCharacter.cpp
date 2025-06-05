// Fill out your copyright notice in the Description page of Project Settings.


#include "AsyncCustomisation/Public/BaseCharacter.h"

#include "Components/CustomizationComponent.h"
#include "Components/InventoryComponent.h"

ABaseCharacter::ABaseCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	
	LegsMesh = CreateOptionalDefaultSubobject<USkeletalMeshComponent>("LegsMesh");
	ensure(LegsMesh);
	LegsMesh->SetupAttachment(GetMesh());
	
	HandsMesh = CreateOptionalDefaultSubobject<USkeletalMeshComponent>("HandsMesh");
	ensure(HandsMesh);
	HandsMesh->SetupAttachment(GetMesh());
	
	WristsMesh = CreateOptionalDefaultSubobject<USkeletalMeshComponent>("WristsMesh");
	ensure(WristsMesh);
	WristsMesh->SetupAttachment(GetMesh());
	
	FeetMesh = CreateOptionalDefaultSubobject<USkeletalMeshComponent>("FeetMesh");
	ensure(FeetMesh);
	FeetMesh->SetupAttachment(GetMesh());
	
	BeardMesh = CreateOptionalDefaultSubobject<USkeletalMeshComponent>("BeardMesh");
	ensure(BeardMesh);
	BeardMesh->SetupAttachment(GetMesh());

	TorsoMesh = CreateOptionalDefaultSubobject<USkeletalMeshComponent>("GoatMesh");
	ensure(TorsoMesh);
	TorsoMesh->SetupAttachment(GetMesh());

	NeckMesh = CreateOptionalDefaultSubobject<USkeletalMeshComponent>("NeckMesh");
	ensure(NeckMesh);
	NeckMesh->SetupAttachment(GetMesh());
	
	FaceAccessoryMesh = CreateOptionalDefaultSubobject<USkeletalMeshComponent>("FaceAccessoryMesh");
	ensure(FaceAccessoryMesh);
	FaceAccessoryMesh->SetupAttachment(GetMesh());
	
	BackAccessoryFirstMesh = CreateOptionalDefaultSubobject<USkeletalMeshComponent>("BackAccessoryFirstMesh");
	ensure(BackAccessoryFirstMesh);
	BackAccessoryFirstMesh->SetupAttachment(GetMesh());

	BackAccessorySecondaryMesh = CreateOptionalDefaultSubobject<USkeletalMeshComponent>("BackAccessorySecondaryMesh");
	ensure(BackAccessorySecondaryMesh);
	BackAccessorySecondaryMesh->SetupAttachment(GetMesh());
	
	LegKnifeMesh = CreateOptionalDefaultSubobject<USkeletalMeshComponent>("LegKnifeMesh");
	ensure(LegKnifeMesh);
	LegKnifeMesh->SetupAttachment(GetMesh());

	HairMesh = CreateOptionalDefaultSubobject<USkeletalMeshComponent>("HairMesh");
	ensure(HairMesh);
	HairMesh->SetupAttachment(GetMesh());

	CustomizationComponent = CreateDefaultSubobject<UCustomizationComponent>(TEXT("CustomizationComponent"));
	ensure(CustomizationComponent);

	InventoryComponent = CreateDefaultSubobject<UInventoryComponent>(TEXT("InventoryComponent"));
}

void ABaseCharacter::BeginPlay()
{
	Super::BeginPlay();
}

void ABaseCharacter::SetupMasterPose()
{
	HairMesh->SetLeaderPoseComponent(GetMesh(), true);
	LegKnifeMesh->SetLeaderPoseComponent(GetMesh(), true);
	FaceAccessoryMesh->SetLeaderPoseComponent(GetMesh(), true);
	NeckMesh->SetLeaderPoseComponent(GetMesh(), true);
	BeardMesh->SetLeaderPoseComponent(GetMesh(), true);
	LegsMesh->SetLeaderPoseComponent(GetMesh(), true);
	BackAccessoryFirstMesh->SetLeaderPoseComponent(GetMesh(), true);
	BackAccessorySecondaryMesh->SetLeaderPoseComponent(GetMesh(), true);
	TorsoMesh->SetLeaderPoseComponent(GetMesh(), true);
}

void ABaseCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ABaseCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

void ABaseCharacter::EquipItems(TArray<FName> InItems)
{
	CustomizationComponent->EquipItems(InItems);
}

