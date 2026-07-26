// Fill out your copyright notice in the Description page of Project Settings.


#include "AI_CharacterCpp.h"
#include "AIController.h"
#include "Navigation/PathFollowingComponent.h"  // EPathFollowingResult için

#include "Runtime/AIModule/Classes/AIController.h"
#include "AI_Character.h"
#include "NavigationSystem.h"
// Sets default values
AAI_CharacterCpp::AAI_CharacterCpp()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void AAI_CharacterCpp::BeginPlay()
{
	Super::BeginPlay();

	// NavigationSystem'e erişim: haritadaki NavMesh verisini tutan sistem.
	// GetWorld() ile hangi dünyadaki nav sistemini istediğimizi söylüyoruz.
	NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());

	// Bulunacak rastgele noktanın saklanacağı yapı.
	// İçinde .Location (FVector) ve NavMesh üzerindeki poligon bilgisi vardır.
	//TODO FNavLocation NavLocation;

	// Karakterin bulunduğu konumdan 10000 birim yarıçapta,
	// NavMesh üzerinde YÜRÜNEBİLİR rastgele bir nokta bul.
	// Sonuç NavLocation'a yazılır (out parametre).
	NavSys->GetRandomReachablePointInRadius(GetActorLocation(), 2000.f, NavLocation);

	// Bu karakteri kontrol eden Controller'ı al ve AAIController'a dönüştür.
	// Cast<> güvenlidir: tip uymuyorsa çökmez, nullptr döner.
	// (Oyuncu karakteri olsaydı PlayerController dönerdi ve cast nullptr verirdi)
	AIC_Ref = Cast<AAIController>(GetController());

	if (AIC_Ref)
	{
		GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Green,TEXT("AAI_Character CPP ÇALIŞIYOR!"));

		// Delegate bağlantısı: hareket bittiğinde HareketBitti() otomatik çağrılır.
		// DİKKAT: MoveToLocation'dan ÖNCE bağlanmalı, yoksa ilk hareketin
		// sonucunu kaçırabilir. Ayrıca bu satır her çalıştığında yeni bir
		// bağlantı eklenir — tekrar tekrar çağrılırsa fonksiyon birden çok kez tetiklenir.
		AIC_Ref->ReceiveMoveCompleted.AddDynamic(this, &AAI_CharacterCpp::HareketBitti);

		// AI'ya "şu koordinata git" emri. Pathfinding + NavMesh kullanır.
		// NavLocation.Location → yukarıda bulunan rastgele noktanın FVector'ü
		AIC_Ref->MoveToLocation(NavLocation.Location);
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Green,TEXT("Çalışmıyor"));
	}
	
	// Broadcast'i hemen yapma. 1 saniye bekle ki BP tarafındaki
	// bind (dinleyici bağlama) tamamlanmış olsun. Sonra tetikle.
	GetWorldTimerManager().SetTimer(GecikmeTimer, this, &AAI_CharacterCpp::GecikmeliTetikle, 1.f, false);
	
}

// Called every frame
void AAI_CharacterCpp::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AAI_CharacterCpp::HareketBitti(FAIRequestID RequestID, EPathFollowingResult::Type Result)
{
	GEngine->AddOnScreenDebugMessage(1019, 10.f, FColor::White,TEXT("HAREKET BİTTİ, BİR SONRAKİ HAREKETE GEÇİLİYOR!"));
	
	GetWorldTimerManager().SetTimer(TimerHandle, this, &AAI_CharacterCpp::YeniHareket, 2.f);
}

void AAI_CharacterCpp::YeniHareket()
{
	NavSys->GetRandomReachablePointInRadius(GetActorLocation(), 2000.f, NavLocation);
	if (AIC_Ref && NavSys)
	{
		AIC_Ref->ReceiveMoveCompleted.AddDynamic(this, &AAI_CharacterCpp::HareketBitti);
		AIC_Ref->MoveToLocation(NavLocation.Location);
	}
}

void AAI_CharacterCpp::GecikmeliTetikle()
{
	// Başlangıç değerini bir kez yayınla (BP ekranda 10 görsün)
	OrnekDelegate2.Broadcast(Zaman);

	// Son parametre true → TEKRARLAYAN timer. Her 1 sn'de SaniyeGecti() çalışır.
	GetWorldTimerManager().SetTimer(GecikmeTimer, this, &AAI_CharacterCpp::SaniyeGecti, 1.f, true);
}

void AAI_CharacterCpp::SaniyeGecti()
{
	Zaman--;
	
	OrnekDelegate2.Broadcast(Zaman);
	
	 // Süre bittiyse timer'ı durdur — yoksa eksiye doğru sonsuza kadar sayar
        if (Zaman <= 0)
        {
            GetWorldTimerManager().ClearTimer(GecikmeTimer);
            GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Sure bitti!"));
        }
	
}