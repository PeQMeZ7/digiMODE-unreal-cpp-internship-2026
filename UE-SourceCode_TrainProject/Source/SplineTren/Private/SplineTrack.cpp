#include "SplineTrack.h"
#include "Components/SplineComponent.h"      // USplineComponent'in TAM tanımı burada gerekli
#include "Components/SplineMeshComponent.h"  // Header'da sadece forward declaration vardı

ASplineTrack::ASplineTrack()
{
    // Tick, her karede çalışan fonksiyon. Ray hiç hareket etmediği için
    // kapatıyoruz — açık bırakmak boşuna CPU harcamak olurdu.
    PrimaryActorTick.bCanEverTick = false;

    // CreateDefaultSubobject: Bileşenleri SADECE constructor'da bu şekilde üretilir.
    // TEXT("Spline") verdiğimiz iç isim; UE bu isimle bileşeni takip eder,
    // sonradan değiştirirsen mevcut sahne referansları kopar.
    Spline = CreateDefaultSubobject<USplineComponent>(TEXT("Spline"));

    // Her aktörün bir kök bileşeni (RootComponent) olmalı.
    // Kök, aktörün konum/rotasyon/ölçek sahibidir; diğer bileşenler ona bağlanır.
    // Kök atanmazsa aktör viewport'ta taşınamaz, gizmo bile çıkmaz.
    RootComponent = Spline;
}

void ASplineTrack::OnConstruction(const FTransform& Transform)
{
    // Üst sınıfın kendi işini yapmasına izin ver — atlarsan beklenmedik davranışlar olur.
    Super::OnConstruction(Transform);

    // ---- 1) TEMİZLİK ----
    // OnConstruction her düzenlemede yeniden çalışır. Eski segmentleri silmezsek
    // her seferinde yenileri eskilerin üstüne eklenir ve sahne şişer.
    for (USplineMeshComponent* S : Segmentler)
    {
        // IsValid(): Pointer null mı VE nesne yok edilmeyi bekliyor mu, ikisini birden kontrol eder.
        // UObject'ler için bare "if (S)" yetersizdir — UE'de idiomatik olan IsValid'dir.
        if (IsValid(S))
        {
            S->DestroyComponent();  // Bileşeni dünyadan kaldır
        }
    }
    Segmentler.Empty();  // Diziyi de boşalt, yoksa ölü pointer'lar kalır

    // ---- 2) ERKEN ÇIKIŞ KONTROLLERİ ----
    // Mesh atanmamışsa yapacak iş yok. "Early return" deseni:
    // hata durumlarını başta eleyip asıl mantığı iç içe if'lerden kurtarır.
    if (!SegmentMesh)
    {
        return;
    }

    const int32 NoktaSayisi = Spline->GetNumberOfSplinePoints();

    // Bir segment iki nokta arasına gerilir; tek nokta varsa segment üretilemez.
    if (NoktaSayisi < 2)
    {
        return;
    }

    // Kapalı döngü (Closed Loop) ise son nokta başa bağlanır → nokta sayısı kadar segment.
    // Açık spline'da son noktadan sonrası yok → bir eksik segment.
    // Örnek: 5 nokta, açık → 4 segment. 5 nokta, kapalı → 5 segment.
    const int32 SegmentSayisi = Spline->IsClosedLoop() ? NoktaSayisi : NoktaSayisi - 1;

    // ---- 3) SEGMENTLERİ ÜRET ----
    for (int32 i = 0; i < SegmentSayisi; ++i)
    {
        // Constructor DIŞINDA bileşen üretmenin yolu: NewObject + RegisterComponent.
        // CreateDefaultSubobject burada çalışmaz (o sadece constructor'a özeldir).
        // "this" outer parametresi: bu bileşen bu aktöre ait demek, GC ona göre davranır.
        USplineMeshComponent* Seg = NewObject<USplineMeshComponent>(this);

        Seg->SetStaticMesh(SegmentMesh);

        // Mobility, RegisterComponent'ten ÖNCE ayarlanmalı.
        // Movable: runtime'da taşınabilir/değiştirilebilir. Static olsaydı
        // spline değiştiğinde mesh güncellenmezdi ve UE uyarı basardı.
        Seg->SetMobility(EComponentMobility::Movable);

        // Bileşeni spline'a bağla. KeepRelativeTransform: yerel konumunu koru,
        // dünya konumuna göre yeniden hesaplama yapma.
        Seg->AttachToComponent(Spline, FAttachmentTransformRules::KeepRelativeTransform);

        // Mesh'in hangi ekseni "ileri" sayılacak. Ray mesh'i X boyunca uzanıyorsa X.
        // Yanlış eksen seçilirse mesh yan yatar veya garip esner.
        Seg->SetForwardAxis(ESplineMeshAxis::X);

        // KRİTİK: Bunu çağırmazsan bileşen oluşur ama dünyaya kaydolmaz —
        // görünmez, çarpışması olmaz, hiçbir işe yaramaz.
        Seg->RegisterComponent();

        // ---- 3a) SEGMENTİN İKİ UCUNU HESAPLA ----
        // Konumu "nokta indeksi" yerine "spline üzerindeki mesafe" ile alıyoruz.
        // Mesafe tabanlı sorgular her zaman tutarlı sonuç verir.
        const float BasMesafe = Spline->GetDistanceAlongSplineAtSplinePoint(i);

        // % NoktaSayisi: Kapalı döngüde son segment başa dönmeli.
        // i = son indeks ise (i+1) taşar, modülo onu 0'a çevirir.
        const float SonMesafe = Spline->GetDistanceAlongSplineAtSplinePoint((i + 1) % NoktaSayisi);

        // Local uzay: Bileşen spline'a bağlı olduğu için konumlar spline'a göre olmalı.
        // World kullanılsaydı aktör taşındığında mesh'ler yerinde kalırdı.
        FVector BasKonum = Spline->GetLocationAtDistanceAlongSpline(
            BasMesafe, ESplineCoordinateSpace::Local);
        FVector SonKonum = Spline->GetLocationAtDistanceAlongSpline(
            SonMesafe, ESplineCoordinateSpace::Local);

        // ---- 3b) TEĞETLER ----
        // Teğet = mesh'in o uçtan hangi YÖNE ve ne kadar GÜÇLE fırlayacağı.
        // Uzunluğu segment mesafesine eşitlemezsek mesh ya aşırı gerilir ya büzüşür,
        // sonuç kopuk/kırık bir ray olur. (Bu hata projede bizzat yaşandı.)
        const float SegmentUzunluk = SonMesafe - BasMesafe;

        // GetSafeNormal(): Vektörü birim uzunluğa indirger (yön korunur, boy 1 olur).
        // "Safe" olması sıfır vektörde çökmemesi demek.
        // Sonra segment uzunluğuyla çarpıp doğru gücü veriyoruz.
        FVector BasTeget = Spline->GetTangentAtDistanceAlongSpline(
            BasMesafe, ESplineCoordinateSpace::Local).GetSafeNormal() * SegmentUzunluk;
        FVector SonTeget = Spline->GetTangentAtDistanceAlongSpline(
            SonMesafe, ESplineCoordinateSpace::Local).GetSafeNormal() * SegmentUzunluk;

        // Asıl iş: mesh'i iki nokta arasına gerdirip teğetlere göre büker.
        // SplineMeshComponent'in tek marifeti budur.
        Seg->SetStartAndEnd(BasKonum, BasTeget, SonKonum, SonTeget);

        // ---- 3c) KALINLIK ----
        // FVector2D: sadece iki eksen. X (ileri) zaten SetStartAndEnd ile belirlendi,
        // burada ayarladığımız enine kesit: Y = genişlik, Z = yükseklik.
        // 0.3 / 0.05 → yassı ve ince bir ray görünümü.
        Seg->SetStartScale(FVector2D(0.3f, 0.05f));
        Seg->SetEndScale(FVector2D(0.3f, 0.05f));

        // Listeye ekle — bir sonraki OnConstruction'da silinebilsin diye.
        Segmentler.Add(Seg);
    }
}