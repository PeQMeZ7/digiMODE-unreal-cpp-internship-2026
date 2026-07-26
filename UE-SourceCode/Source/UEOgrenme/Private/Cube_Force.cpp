// Fill out your copyright notice in the Description page of Project Settings.


#include "Cube_Force.h"

// Sets default values
ACube_Force::ACube_Force()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	
	// CreateDefaultSubobject → sadece CONSTRUCTOR içinde kullanılır.
	// Actor'ün varsayılan (default) parçasını üretir. Bu component,
	// Actor her doğduğunda otomatik gelir ve editörde Details panelinde görünür.
	// <UStaticMeshComponent> → hangi tip component üretileceği (köşeli parantez = template).
	// TEXT("CubeMesh") → component'in DAHILI adı. Editörde bu isimle listelenir.
	// Her component için benzersiz olmalı, yoksa çakışır.
	CubeMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CubeMesh"));
	
	// SetupAttachment → bu component'i başka bir component'in ALTINA bağlar.
    // Bağlandığı şey hareket edince bu da onunla birlikte hareket eder.
    // GetRootComponent() → Actor'ün kök component'i (hiyerarşinin en tepesi).
    // Bağlamazsan component havada kalır, Actor hareket edince peşinden gelmez.
	CubeMesh->SetupAttachment(GetRootComponent());
	
	CubeMesh->SetSimulatePhysics(true);
}

// Called when the game starts or when spawned
void ACube_Force::BeginPlay()
{
	Super::BeginPlay();

	if (CubeMesh)
	{
		FVector ForceVector = FVector(0.f, 10000000.f,0.f);
		CubeMesh->AddForce(ForceVector);
	}
	
	
}

// Called every frame
void ACube_Force::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

