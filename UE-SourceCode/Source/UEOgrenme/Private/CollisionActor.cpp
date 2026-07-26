// Fill out your copyright notice in the Description page of Project Settings.


#include "CollisionActor.h"

#include "MyActorComponent.h"
#include "MyInterface.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"

// Sets default values
ACollisionActor::ACollisionActor()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Kutu şeklinde bir çarpışma (collision) bileşeni üretir.
	// TEXT içindeki isim bu component'in kimliğidir, proje içinde benzersiz olmalı.
	// Sadece constructor içinde çağrılabilir.
	BoxCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxCompCpp"));

	// Bileşeni root'a bağlar; artık actor hareket edince onunla birlikte hareket eder.
	// Bağlanmazsa sahnede bağımsız kalır ve konumu güncellenmez.
	BoxCollision->SetupAttachment(GetRootComponent());
}

// Called when the game starts or when spawned
void ACollisionActor::BeginPlay()
{
	Super::BeginPlay();

	// Kutunun "biri içime girdi" olayını (delegate) kendi fonksiyonumuza bağlar.
	// Bu satır olmadan BeginOverlap asla çağrılmaz — motor kime haber vereceğini bilmez.
	// this = haberi alacak nesne, & ile fonksiyonun adresi verilir.
	// Bağlanan fonksiyon header'da UFUNCTION() işaretli olmak zorunda.
	BoxCollision->OnComponentBeginOverlap.AddDynamic(this, &ACollisionActor::BeginOverlap);
}

// Called every frame
void ACollisionActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// Bir başka actor kutunun sınırlarına GİRDİĞİ anda motor bu fonksiyonu çağırır.
// Parametreleri motor doldurur, biz sadece kullanırız:
//   OverlappedComponent : temas eden kendi bileşenimiz (BoxCollision)
//   OtherActor          : içeri giren actor — genelde en çok işimize yarayan bu
//   OtherComp           : giren actor'ün hangi bileşeniyle değdiği (mesh, capsule vs.)
//   OtherBodyIndex      : çoklu gövdeli mesh'lerde hangi parça (çoğunlukla 0)
//   bFromSweep          : temas hareket sırasında mı oluştu
//   SweepResult         : temas noktası, normal vektörü gibi detaylar
void ACollisionActor::BeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                                   UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
                                   const FHitResult& SweepResult)
{
	// OtherActor null gelebilir; kontrolsüz kullanmak çökmeye yol açar.
	if (OtherActor)
	{
		GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Green,
		                                 FString::Printf(TEXT("Giren Actor: %s"), *OtherActor->GetName()));
	}
	
	
	// 1. ADIM — Actor'ün içindeki component'i bul.
	// OtherActor bir "kap"tır; interface'i o değil, içindeki component taşıyor.
	// Bu yüzden doğrudan Cast<IMyInterface>(OtherActor) her zaman nullptr döner.
	//! FindComponentByClass, actor'ün component listesini gezip istenen tipi arar.
	// Bulamazsa nullptr döner — bu yüzden if içinde kontrol ediyoruz.
	UMyActorComponent* Comp = OtherActor->FindComponentByClass<UMyActorComponent>();

	if (Comp)
	{
		// 2. ADIM — Bulunan component gerçekten interface'i uyguluyor mu?
		//! Cast, çalışma anında tip kontrolü yapar. Uyuyorsa pointer, uymuyorsa nullptr.
		// Component var ama interface'i miras almamış olabilir — o yüzden bu kontrol ayrı.
		IMyInterface* InteractActor = Cast<IMyInterface>(Comp);
        	
        	if (InteractActor)
        	{
        		InteractActor->Interact();
        	}
        	else
        	{
        		GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Green,TEXT("Interface Algılanamadı"));
        	}
	}
	
}
