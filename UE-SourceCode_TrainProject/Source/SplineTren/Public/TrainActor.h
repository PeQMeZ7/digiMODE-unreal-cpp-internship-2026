#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TrainActor.generated.h"  // Her zaman EN SON include

// Forward declaration'lar — header'da sadece "bu tip var" demek yeterli.
// Tam tanımları .cpp'de include ediliyor, böylece derleme hızlanır
// ve header'lar arası döngüsel bağımlılık riski azalır.
class UStaticMeshComponent;
class ASplineTrack;        // Takip edilecek ray aktörü
class USpringArmComponent;
class UCameraComponent;
class USceneComponent;
class ASignalActor;        // Trafik sinyali aktörü

UCLASS()
class SPLINETREN_API ATrainActor : public AActor
{
    GENERATED_BODY()

public:
    ATrainActor();

    // Tick: her karede çağrılan fonksiyon. Trenin konumu burada güncelleniyor.
    // DeltaTime = önceki kareden bu yana geçen saniye. FPS'ten bağımsız
    // hareket için hız hesabında bununla çarpmak şart.
    virtual void Tick(float DeltaTime) override;

    // ---- BİLEŞENLER ----
    // Bileşenler VisibleAnywhere olur (EditAnywhere değil):
    // Details'te görünür ve iç ayarları düzenlenebilir, ama bileşenin
    // kendisi başka bir nesneyle DEĞİŞTİRİLEMEZ.

    // Trenin görünen gövdesi (mesh burada atanır)
    UPROPERTY(VisibleAnywhere, Category = "Tren")
    UStaticMeshComponent* Govde;

    // Kamerayı taşıyan kol. TargetArmLength = 0 yapıldığı için
    // aslında kol görevi görmüyor, kamerayı konumlandıran bir dayanak.
    UPROPERTY(VisibleAnywhere, Category = "Tren")
    USpringArmComponent* KameraKolu;

    // Oyuncunun baktığı kamera
    UPROPERTY(VisibleAnywhere, Category = "Tren")
    UCameraComponent* Kamera;

    // Kök bileşen. Görünmez, boş bir dayanak noktası.
    // Neden var: Gövdeye mesh düzeltmesi (90°/180° dönüş) uyguluyoruz.
    // Kamera gövdeye bağlı olsaydı o dönüşü miras alır ve yan yatardı.
    // Kamerayı bu tarafsız köke bağlayarak ikisini birbirinden ayırdık.
    UPROPERTY(VisibleAnywhere, Category = "Tren")
    USceneComponent* Kok;

    // ---- AYARLANABİLİR PARAMETRELER ----
    // EditAnywhere: Details panelinden hem Blueprint'te hem sahnedeki
    // kopyada değiştirilebilir. Play sırasında bile canlı ayar yapılabilir.

    // Hangi rayı takip edecek. Details'ten sahnedeki SplineTrack aktörü seçilir.
    // Bu bir AKTÖR referansı — bir nesnenin başka bir nesneyi tanıması.
    UPROPERTY(EditAnywhere, Category = "Tren")
    ASplineTrack* Ray;

    // Saniyede kaç Unreal birimi ilerlesin (1 birim ≈ 1 cm)
    UPROPERTY(EditAnywhere, Category = "Tren")
    float Hiz = 400.f;

    // Rayın sonuna gelince başa dönsün mü, yoksa dursun mu
    UPROPERTY(EditAnywhere, Category = "Tren")
    bool bDonguselMi = true;

    // NOT: Bu alan artık kullanılmıyor (kod Govde'yi constructor'da sabit döndürüyor).
    // İleride Details'ten ayarlanabilir yapmak istersen OnConstruction'da uygula.
    UPROPERTY(EditAnywhere, Category = "Tren")
    FRotator MeshDuzeltme = FRotator(0.f, 0.f, 0.f);

    // Aktörün ray yönüne EKLENEN düzeltme.
    // Mesh'in ileri ekseni rayın yönüyle uyuşmuyorsa buradan çevrilir.
    // Sadece Z (Yaw) kullanılmalı — X/Y verirsen kamera da yatar.
    UPROPERTY(EditAnywhere, Category = "Tren")
    FRotator YonDuzeltme = FRotator(0.f, 0.f, 0.f);

    // NOT: Fare bakışı kaldırıldığı için bu alan şu an kullanılmıyor.
    UPROPERTY(EditAnywhere, Category = "Tren")
    float BakisHizi = 1.5f;

    // ---- SİNYAL SİSTEMİ ----

    // Raydaki trafik sinyalleri. Details'ten elle eklenir.
    // TArray = UE'nin dinamik dizisi (C++'ın std::vector karşılığı).
    // İçindeki pointer'lar UPROPERTY sayesinde GC tarafından korunur.
    UPROPERTY(EditAnywhere, Category = "Tren")
    TArray<ASignalActor*> Sinyaller;

    // Kırmızı sinyale bu mesafede yaklaşınca durur.
    // Küçültürsen tren sinyale daha çok sokulur.
    UPROPERTY(EditAnywhere, Category = "Tren")
    float FrenMesafesi = 300.f;

protected:
    // BeginPlay: Oyun başladığında BİR KEZ çalışır.
    // Constructor'dan farkı: constructor editör açılırken de çalışır,
    // BeginPlay ise sadece Play'e basınca. Oyun mantığı buraya yazılır.
    virtual void BeginPlay() override;

private:
    // Trenin spline üzerinde kat ettiği toplam mesafe.
    // Trenin "nerede olduğunu" tutan tek durum değişkeni —
    // konum ve rotasyon her karede bundan hesaplanıyor.
    // private çünkü dışarıdan kimsenin karışmasına gerek yok.
    float KatEdilenMesafe = 0.f;
};