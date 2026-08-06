#pragma once

// #pragma once: Bu dosya derleme sırasında birden fazla kez include edilse bile
// sadece bir kez işlenir. Klasik "#ifndef ... #define" korumasının kısa hali.

#include "CoreMinimal.h"           // UE'nin temel tipleri: FVector, FString, TArray, int32...
#include "GameFramework/Actor.h"   // AActor sınıfı — dünyaya yerleştirilebilen her şeyin atası
#include "SplineTrack.generated.h" // UHT'nin (Unreal Header Tool) ürettiği dosya.
                                   // MUTLAKA en son include olmalı, yoksa derleme hatası verir.

// Forward declaration (ileri bildirim):
// "Böyle bir sınıf var, detayını sonra öğreneceğim" demek.
// Header'a tam #include koymak yerine bunu kullanmak derleme süresini kısaltır.
// Tam tanım .cpp dosyasında include ediliyor.
class USplineComponent;
class USplineMeshComponent;

UCLASS()  // Bu makro sınıfı UE'nin yansıma (reflection) sistemine kaydeder.
          // Olmadan Blueprint'te görünmez, GC yönetemez, Details paneline çıkmaz.
class SPLINETREN_API ASplineTrack : public AActor
{
    // SPLINETREN_API: Modül dışına açılım makrosu (DLL export).
    // Başka modüller bu sınıfı kullanabilsin diye gerekli.

    GENERATED_BODY()  // UHT'nin ürettiği gizli kodu buraya enjekte eder.
                      // Her UCLASS'ın ilk satırı bu olmalı.

public:
    ASplineTrack();  // Constructor — bileşenler burada oluşturulur.
                     // DİKKAT: Constructor editör açılırken de çalışır (CDO üretimi),
                     // o yüzden burada oyun mantığı yazılmaz.

    // Rayın izleyeceği görünmez eğri. Kök bileşen olacak.
    // VisibleAnywhere: Details'te görünür ama başka bir şeyle DEĞİŞTİRİLEMEZ
    //                  (bileşenler için doğru olan budur; içindeki ayarlar yine düzenlenebilir).
    // BlueprintReadOnly: Blueprint'ten okunabilir, yazılamaz.
    // Category: Details panelinde hangi başlık altında görüneceği.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ray")
    USplineComponent* Spline;

    // Her segmentte tekrarlanacak mesh (bir parça ray).
    // EditAnywhere: Details'ten değiştirilebilir — hem asset'te hem sahnedeki kopyada.
    // UStaticMesh* bir ASSET pointer'ı; UPROPERTY olduğu için GC onu canlı tutar.
    UPROPERTY(EditAnywhere, Category = "Ray")
    UStaticMesh* SegmentMesh;

    // OnConstruction: Editörde aktör her değiştiğinde çalışır
    // (taşındığında, bir property düzenlendiğinde, sahneye ilk konduğunda).
    // Play'e basmadan sonucu görmeni sağlayan şey bu.
    // Runtime'da da aktör spawn edilirken bir kez çalışır.
    virtual void OnConstruction(const FTransform& Transform) override;

private:
    // Ürettiğimiz segmentleri saklıyoruz ki bir sonraki OnConstruction'da silebilelim.
    // Bu liste olmasaydı her düzenlemede yeni mesh'ler eskilerin üstüne birikirdi.
    //
    // UPROPERTY() (boş parantez): Details'te görünmez ama GC'ye
    // "bu pointer'lar canlı, silme" der. Aksi halde bileşenler beklenmedik anda yok olabilir.
    UPROPERTY()
    TArray<USplineMeshComponent*> Segmentler;
};