#include "SignalActor.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"  // UMaterialInstanceDynamic::Create için

ASignalActor::ASignalActor()
{
    // Sinyal her karede iş yapmıyor — renk değişimini Timer yönetiyor.
    // Tick'i kapalı bırakmak boşuna CPU harcamayı önler.
    //
    // Kural: Bir şey düzenli aralıklarla oluyorsa Timer, her karede
    // oluyorsa Tick kullanılır. Buradaki iş 5 saniyede bir, yani Timer'lık.
    PrimaryActorTick.bCanEverTick = false;

    // Direk: hem görsel hem kök bileşen
    Govde = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Govde"));
    RootComponent = Govde;

    // Lamba: direğe bağlı
    Lamba = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Lamba"));
    Lamba->SetupAttachment(Govde);

    // Direğin tepesine yerleştir. Relative = Govde'ye göre,
    // yani direk nereye taşınırsa lamba da onunla gider.
    Lamba->SetRelativeLocation(FVector(0.f, 0.f, 200.f));

    // Küreyi küçült. FVector(0.4f) tek değerle üç ekseni birden ayarlar
    // (0.4, 0.4, 0.4 yazmanın kısa hali).
    Lamba->SetRelativeScale3D(FVector(0.4f));
}

void ASignalActor::BeginPlay()
{
    Super::BeginPlay();

    // ---- DİNAMİK MATERYAL OLUŞTURMA ----
    // Lamba'nın 0. slotundaki mevcut materyali al.
    // "if (Tip* X = ifade)" deseni hem atar hem null kontrolü yapar.
    if (UMaterialInterface* Mevcut = Lamba->GetMaterial(0))
    {
        // Mevcut materyalden bu aktöre özel bir kopya üret.
        // İkinci parametre (this) = sahibi, GC bu ilişkiye göre davranır.
        LambaMat = UMaterialInstanceDynamic::Create(Mevcut, this);

        // Kopyayı mesh'e geri ata. Bu satır olmazsa kopya üretilir
        // ama mesh hâlâ eski materyali kullanır, renk değişimi görünmez.
        Lamba->SetMaterial(0, LambaMat);
    }

    // Başlangıç rengini uygula (bYesilMi'nin varsayılan değerine göre)
    RengiGuncelle();

    // ---- OTOMATİK DEĞİŞİM TIMER'I ----
    if (bOtomatikDegissin)
    {
        // SetTimer parametreleri sırayla:
        //   TimerHandle          → timer'ın kimliği (üye değişken olmalı)
        //   this                 → hangi nesnenin fonksiyonu çağrılacak
        //   &ASignalActor::...   → çağrılacak fonksiyonun adresi
        //   DegisimSuresi        → kaç saniyede bir
        //   true                 → TEKRARLASIN (false olsaydı bir kez çalışıp biterdi)
        GetWorldTimerManager().SetTimer(
            TimerHandle, this, &ASignalActor::DurumDegistir, DegisimSuresi, true);
    }
}

void ASignalActor::DurumDegistir()
{
    // ! operatörü bool'u tersine çevirir: true→false, false→true.
    // Yeşilse kırmızı, kırmızıysa yeşil olur.
    bYesilMi = !bYesilMi;

    // Durum değişti, görseli de güncelle.
    // İki işi ayrı fonksiyonlara bölmek: DurumDegistir "ne oldu"yu,
    // RengiGuncelle "nasıl görünecek"i yönetir. Böylece rengi
    // başka yerden de (örneğin Blueprint'ten) güncelleyebiliriz.
    RengiGuncelle();
}

void ASignalActor::RengiGuncelle()
{
    // Materyal oluşturulamadıysa (mesh'e materyal atanmamışsa) çık.
    // Null pointer'a erişmek çökmeye sebep olur.
    if (LambaMat)
    {
        // Üçlü operatör (ternary): "koşul ? doğruysa : yanlışsa"
        // Kısa if-else yazmanın yolu.
        //
        // FLinearColor: 0-1 aralığında float renk (0-255 değil).
        // Render sistemi bu formatı kullanır.
        const FLinearColor Renk = bYesilMi ? FLinearColor::Green : FLinearColor::Red;

        // Materyaldeki "Renk" adlı Vector Parameter'ı değiştir.
        //
        // ÖNEMLİ: Bu isim materyal editöründeki parametrenin adıyla
        // BİREBİR aynı olmalı. Yanlış yazarsan hata vermez, sessizce
        // hiçbir şey olmaz — bu tip hataları bulmak zordur.
        LambaMat->SetVectorParameterValue(TEXT("Renk"), Renk);
    }
}