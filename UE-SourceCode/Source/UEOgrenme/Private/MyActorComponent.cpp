// Fill out your copyright notice in the Description page of Project Settings.


#include "MyActorComponent.h"

#include "AudioMixerBlueprintLibrary.h"
#include "DenemeActor.h"
#include "DrawDebugHelpers.h"
#include "Chaos/ChaosPerfTest.h"
#include "GeometryCollection/GeometryCollectionParticlesData.h"

// Sets default values for this component's properties
UMyActorComponent::UMyActorComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts


void UMyActorComponent::BeginPlay()
{
	Super::BeginPlay();


	GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Magenta, TEXT("YENI KOD"));

	// ...
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("%d"), health));
	SayiYaz(sayi);
	YaziYaz("L ye basarak ekrana yazi yazdirabilirsiniz!");


	MyNativeEvent();

	// GetWorld()      : component'in içinde bulunduğu dünyayı verir
	// GetTimerManager : o dünyanın zamanlayıcı yöneticisi — tüm timer'ları o tutar
	// SetTimer parametreleri sırayla:
	//   TH_TimerHandle : timer'ın kimliği, sonradan durdurmak için gerekli
	//   this           : fonksiyonun sahibi olan nesne
	//   &Sınıf::Fonksiyon : süre dolunca çağrılacak fonksiyonun adresi
	//   2.f            : kaç saniyede bir çalışacağı
	//   true           : tekrar etsin mi (false = sadece bir kez)

	GetWorld()->GetTimerManager().SetTimer(TH_TimerHandle, this, &UMyActorComponent::TetiklenecekFonksiyon, 10.f,
	                                       false);

	FVector SpawnLocation = FVector(400.f, -1320.f, 100.f);
	FActorSpawnParameters SpawnParams;
	// Doğacağı yerde başka bir nesne varsa ne yapılsın?
	// AdjustIfPossibleButAlwaysSpawn: çakışma varsa konumu biraz kaydırır,
	// kaydıramazsa bile yine de üretir — yani spawn asla iptal olmaz.
	// (Alternatifler: DontSpawnIfColliding = çakışma varsa hiç üretme,
	SpawnParams.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;
	// Dünyaya yeni bir aktör üretir.
	// SpawnClass    : hangi sınıftan üretileceği (Blueprint'te seçtiğin tip)
	// SpawnLocation : nereye konacağı
	// FRotator(0)   : hangi açıyla duracağı — 0 = dönüş yok
	// SpawnParams   : yukarıda hazırladığımız ayarlar
	GetWorld()->SpawnActor<AActor>(SpawnClass, SpawnLocation, FRotator(0), SpawnParams);
	
	// 1. Dinleyiciyi bağla (bir kez)
	OrnekDelegate.AddDynamic(this, &UMyActorComponent::OlayGeldi);

	// 2. Test için hemen tetikle
	OrnekDelegate.Broadcast();   // → OlayGeldi() çalışır → ekrana yazı
	
	
}


// Called every frame
void UMyActorComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                      FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	TraceLine();

	// ...
}

void UMyActorComponent::TraceLine()
{
	AActor* Owner = GetOwner();
	if (!Owner) return;


	FVector Start = GetOwner()->GetActorLocation();
	FVector End = (GetOwner()->GetActorForwardVector() * TraceDistance) + Start;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(Owner);

	FHitResult HitResult;
	bool bHit = GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Camera, QueryParams);

	if (bHit && HitResult.GetActor()) // çarptı VE actor geçerli mi
	{
		GEngine->AddOnScreenDebugMessage(10, 10.f, FColor::Turquoise,
		                                 FString::Printf(TEXT("Hit olan actor: %s"), *HitResult.GetActor()->GetName()));

		//! GetActor() genel bir AActor* döndürür.
		//! Cast, bu nesnenin gerçekten ADenemeActor olup olmadığını kontrol eder.
		//! Doğruysa pointer'ı, değilse nullptr döner.
		ADenemeActor* DenemeActor = Cast<ADenemeActor>(HitResult.GetActor());
		// () AActor* döndürür. Cast bunu ADenemeActor'e dönüştürmeye çalışır Döndürebilirse Pointer'a kaydeder  

		// if (DenemeActor) Bunun Olabilmesini Sağlıyor!
		// {
		// 	DenemeActor->OzelFonksiyonum();   // sadece ADenemeActor'de var
		// 	DenemeActor->OzelDegiskenim = 5;
		// }

		if (DenemeActor)
		{
			GEngine->AddOnScreenDebugMessage(11, 10.f, FColor::Orange,
			                                 FString::Printf(
				                                 TEXT("Deneme Actor'e Çarptık. Canı: %f"), DenemeActor->Health));

			DenemeActor->Health -= 1.f;

			if (DenemeActor->Health <= 0.f)
			{
				DenemeActor->Destroy();
				GEngine->AddOnScreenDebugMessage(11, 10.f, FColor::Orange,TEXT("Deneme Actor Yok Edildi!"));
			}
		}
		else
		{
			GEngine->AddOnScreenDebugMessage(11, 10.f, FColor::Orange,TEXT("Başka Actor'e Çarptık"));
		}
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(10, 10.f, FColor::Red,TEXT("Hit no actor"));
	}
}

void UMyActorComponent::SayiYaz(int32 Sayi)
{
	this->sayi++;
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Cyan, FString::Printf(TEXT("Sayi: %d"), this->sayi));
	}
}

void UMyActorComponent::YaziYaz(FString deneme)
{
	UE_LOG(LogTemp, Warning, TEXT("yaziYaz cagrildi"));

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, TEXT("yaziYaz: ") + deneme);
	}
}

void UMyActorComponent::Counter(int counterArtisi)
{
	this->counter++;
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Emerald, FString::Printf(TEXT("counter: %d"), counter));
	}
}


void UMyActorComponent::MyNativeEvent_Implementation()
{
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Purple, FString::Printf(TEXT("MyNativeEvent C++")));
	}
}

void UMyActorComponent::Interact()
{
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green,TEXT("Interact"));
}

void UMyActorComponent::TetiklenecekFonksiyon()
{
	GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Cyan,
	                                 TEXT("Oyun Başladıktan 10 Saniye Sonra CppTimer Tetiklendi!"));
}

void UMyActorComponent::OlayGeldi()
{
	GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Green,TEXT("Delegate Çalıştı!"));
	
}
