#include "TrainActor.h"
#include "SplineTrack.h"                        // Ray->Spline'a erişmek için tam tanım gerekli
#include "GameFramework/SpringArmComponent.h"
#include "Components/SceneComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/SplineComponent.h"         // Spline fonksiyonlarını çağırmak için
#include "Components/StaticMeshComponent.h"
#include "SignalActor.h"                        // Sinyal->bYesilMi okumak için

ATrainActor::ATrainActor()
{
    // Tren her karede hareket edecek, Tick açık olmalı.
    PrimaryActorTick.bCanEverTick = true;

    // ---- BİLEŞEN HİYERARŞİSİ ----
    // Kok (kök, hiç döndürülmez)
    //  ├── Govde (mesh düzeltmesi burada uygulanır)
    //  └── KameraKolu → Kamera (gövdenin dönüşünden etkilenmez)

    Kok = CreateDefaultSubobject<USceneComponent>(TEXT("Kok"));
    RootComponent = Kok;  // Aktörün konum/rotasyon sahibi bu

    Govde = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Govde"));
    Govde->SetupAttachment(Kok);  // Constructor'da bağlama yöntemi bu
                                  // (runtime'da AttachToComponent kullanılır)

    // Trenin ray içine gömülmemesi için Z ekseninde yukarı al
    Govde->SetRelativeLocation(FVector(0.f, 0.f, 50.f));

    // DİKKAT: Aşağıda iki SetRelativeRotation var, ikincisi birincisini eziyor.
    // Etkin olan sadece FRotator(90, 0, 180). İlk satır silinebilir.
    Govde->SetRelativeRotation(FRotator(0.f, 0.f, 90.f));  // (etkisiz — aşağıda eziliyor)

    // ---- KAMERA ----
    KameraKolu = CreateDefaultSubobject<USpringArmComponent>(TEXT("KameraKolu"));
    KameraKolu->SetupAttachment(Kok);  // Kok'a bağlı, Govde'ye DEĞİL — kritik nokta

    // TargetArmLength = 0: Kol uzunluğu yok, kamera doğrudan bu noktada.
    // Sıfırdan büyük olsaydı kamera geriden takip eden TPS kamerası olurdu.
    KameraKolu->TargetArmLength = 0.f;
    KameraKolu->SetRelativeLocation(FVector(0.f, 0.f, 0.f));

    // FRotator(Pitch, Yaw, Roll) — parametre sırası bu.
    // Details panelinde ise X=Roll, Y=Pitch, Z=Yaw olarak görünür. Karıştırması kolay.
    Govde->SetRelativeRotation(FRotator(90.f, 0.f, 180.f));

    // Çarpışma testi: açıkken kol, araya giren nesneye çarpınca kendini kısaltır.
    // TargetArmLength zaten 0 olduğu için gereksiz, kapatıyoruz.
    KameraKolu->bDoCollisionTest = false;

    // Camera lag: kameranın hedefi gecikmeli takip etmesi. Yumuşak TPS için güzel,
    // kabin içi görüş için kaymaya sebep olur, kapalı.
    KameraKolu->bEnableCameraLag = false;

    Kamera = CreateDefaultSubobject<UCameraComponent>(TEXT("Kamera"));

    // SocketName: Spring Arm'ın ucundaki bağlantı noktası.
    // Kamerayı buraya takmak standart yöntemdir.
    Kamera->SetupAttachment(KameraKolu, USpringArmComponent::SocketName);

    // Kameranın kendi yön düzeltmesi — mesh'in bakış yönüne göre ayarlandı
    Kamera->SetRelativeRotation(FRotator(0.f, 180.f, 0.f));

    // Bu bayrak açıkken, oyuncu bu aktöre baktığında UE otomatik olarak
    // içindeki CameraComponent'i bulup onu kullanır.
    bFindCameraComponentWhenViewTarget = true;
}

void ATrainActor::BeginPlay()
{
    Super::BeginPlay();  // Üst sınıfın BeginPlay'ini atlama

    // Oyuncunun kamerasını bu aktöre çevir.
    // "if (Tip* Degisken = ifade)" deseni: hem atama yapar hem null kontrolü.
    // PC sadece bu if bloğu içinde geçerli — kapsamı dar tutmak iyi pratiktir.
    if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
    {
        // 0.5 saniyelik yumuşak geçiş (blend). 0 verilirse anında zıplar.
        PC->SetViewTargetWithBlend(this, 0.5f);
    }

    // Ray atanmamışsa tren hiçbir şey yapamaz. Sessizce durmak yerine
    // Output Log'a uyarı basıyoruz — hata ayıklarken çok zaman kazandırır.
    if (!IsValid(Ray))
    {
        UE_LOG(LogTemp, Warning, TEXT("TrainActor: Ray atanmamis!"));
    }
}

void ATrainActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // ---- GÜVENLİK KONTROLÜ ----
    // Ray yoksa ya da rayın spline'ı yoksa çalışamayız.
    // Erken çıkış (early return) deseni: hata durumunu başta eleyip
    // asıl mantığı iç içe if'lerden kurtarır.
    if (!IsValid(Ray) || !IsValid(Ray->Spline))
    {
        return;
    }

    // Kısa isim — aşağıda defalarca kullanacağız
    USplineComponent* S = Ray->Spline;

    // ---- 1) SİNYAL KONTROLÜ ----
    bool bDurmaliMi = false;

    // Range-based for: dizideki her sinyali tek tek gez
    for (ASignalActor* Sinyal : Sinyaller)
    {
        // Geçersizse veya yeşilse bu sinyalle işimiz yok, sonrakine geç
        if (!IsValid(Sinyal) || Sinyal->bYesilMi)
        {
            continue;
        }

        // Trenin sinyale kalan mesafesi.
        // Pozitifse sinyal ÖNDE, negatifse tren onu çoktan geçmiş.
        const float Fark = Sinyal->SplineMesafesi - KatEdilenMesafe;

        // İki koşul birden:
        //   Fark > 0        → sinyal hâlâ önümüzde (arkadakine fren yapmayız)
        //   Fark < FrenMesafesi → yeterince yaklaştık
        if (Fark > 0.f && Fark < FrenMesafesi)
        {
            bDurmaliMi = true;
            break;  // Bir kırmızı yeterli, kalanlara bakmaya gerek yok
        }
    }

    // ---- 2) MESAFEYİ İLERLET ----
    // Durma mantığı burada çok basit: kırmızıysa mesafe hiç artmaz,
    // tren olduğu yerde donar. (Gerçekçi fren için hız kademeli
    // azaltılabilirdi ama bu proje için bu yeterli.)
    if (!bDurmaliMi)
    {
        // DeltaTime ile çarpmak: 30 FPS'te de 120 FPS'te de
        // saniyede aynı mesafe kat edilir.
        KatEdilenMesafe += Hiz * DeltaTime;
    }

    // ---- 3) RAYIN SONUNA GELDİYSE ----
    const float ToplamUzunluk = S->GetSplineLength();

    if (KatEdilenMesafe > ToplamUzunluk)
    {
        if (bDonguselMi)
        {
            // Fmod = modülo'nun ondalıklı hali (kalan bulma).
            // Örnek: 1050 mesafe, 1000 uzunluk → 50'den devam eder.
            // Doğrudan 0'a çekseydik her turda küçük bir zıplama olurdu.
            KatEdilenMesafe = FMath::Fmod(KatEdilenMesafe, ToplamUzunluk);
        }
        else
        {
            // Döngüsel değilse sonda kilitle
            KatEdilenMesafe = ToplamUzunluk;
        }
    }

    // ---- 4) KONUM VE ROTASYONU SPLINE'DAN SOR ----
    // World uzayı: aktörü doğrudan dünya koordinatına koyacağız.
    // (SplineTrack'te Local kullanmıştık çünkü orada bileşenler spline'a bağlıydı.)
    const FVector YeniKonum = S->GetLocationAtDistanceAlongSpline(
        KatEdilenMesafe, ESplineCoordinateSpace::World);

    const FRotator YeniRotasyon = S->GetRotationAtDistanceAlongSpline(
        KatEdilenMesafe, ESplineCoordinateSpace::World);

    SetActorLocation(YeniKonum);

    // ---- 5) ROTASYON ----
    // Spline'ın verdiği rotasyondan sadece Yaw'ı (yatay dönüş) alıyoruz.
    // Roll ve Pitch alsaydık tren virajlarda yan yatar, yokuşta öne eğilirdi —
    // ve kamera da onunla birlikte döndüğü için görüntü berbat olurdu.
    const FRotator RayYonu = FRotator(0.f, YeniRotasyon.Yaw, 0.f);

    // İki rotasyonu birleştirmek için QUATERNION çarpımı kullanılır.
    // Rotator'ların açılarını tek tek toplamak yanlış sonuç verir
    // (gimbal lock ve eksen sırası problemleri yüzünden).
    // Quaternion → çarp → tekrar Rotator'a çevir: doğru yöntem budur.
    SetActorRotation((RayYonu.Quaternion() * YonDuzeltme.Quaternion()).Rotator());
}