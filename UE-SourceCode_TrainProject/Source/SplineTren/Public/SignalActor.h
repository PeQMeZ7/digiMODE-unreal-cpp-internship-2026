#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SignalActor.generated.h"  // Her zaman EN SON include

// Forward declaration — tam tanım .cpp'de
class UStaticMeshComponent;

UCLASS()
class SPLINETREN_API ASignalActor : public AActor
{
    GENERATED_BODY()

public:
    ASignalActor();

    // ---- GÖRSEL BİLEŞENLER ----

    // Direk gövdesi. Aynı zamanda kök bileşen (RootComponent).
    // Not: Ayrı bir SceneComponent kök yapmadık çünkü buradaki mesh
    // hiç döndürülmüyor — TrainActor'deki kamera sorunu burada yok.
    UPROPERTY(VisibleAnywhere, Category = "Sinyal")
    UStaticMeshComponent* Govde;

    // Işık küresi. Rengi çalışma zamanında değişecek asıl parça.
    UPROPERTY(VisibleAnywhere, Category = "Sinyal")
    UStaticMeshComponent* Lamba;

    // ---- DURUM ----

    // Sinyalin mevcut hali. true = yeşil (tren geçer), false = kırmızı (durur).
    //
    // BlueprintReadWrite: Blueprint'ten hem okunabilir hem yazılabilir.
    // TrainActor bu değeri C++'tan doğrudan okuyor (Sinyal->bYesilMi),
    // ama Blueprint'ten de manuel kontrol edilebilsin diye açık bıraktık.
    //
    // Bool isimlendirme kuralı: UE'de bool üyeler "b" öneki alır (bYesilMi).
    // Details panelinde "b" gizlenir, "Yesil Mi" olarak görünür.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sinyal")
    bool bYesilMi = true;

    // Bu sinyalin spline üzerinde kaçıncı metrede olduğu.
    // TrainActor bunu kendi KatEdilenMesafe'siyle karşılaştırıp
    // duracak mı diye karar veriyor.
    //
    // DİKKAT: Bu değer sinyalin FİZİKSEL konumuyla otomatik senkronize DEĞİL.
    // Sinyali sahnede taşırsan bu sayıyı elle güncellemen gerekir.
    UPROPERTY(EditAnywhere, Category = "Sinyal")
    float SplineMesafesi = 1000.f;

    // Kapalıysa sinyal sabit kalır (elle veya Blueprint'ten değiştirilir).
    UPROPERTY(EditAnywhere, Category = "Sinyal")
    bool bOtomatikDegissin = true;

    // Kaç saniyede bir renk değişsin
    UPROPERTY(EditAnywhere, Category = "Sinyal")
    float DegisimSuresi = 5.f;

protected:
    virtual void BeginPlay() override;

private:
    // Timer'ın kimlik kartı. Timer'ı sonradan durdurmak/kontrol etmek için gerekli.
    //
    // KRİTİK: Bu MUTLAKA sınıf üyesi olmalı. Fonksiyon içinde yerel değişken
    // olarak tanımlarsan fonksiyon bitince yok olur ve timer kontrolden çıkar.
    // (Bu hata bu projede daha önce yaşandı.)
    //
    // FTimerHandle bir USTRUCT olmadığı için UPROPERTY konulmaz.
    FTimerHandle TimerHandle;

    // Timer'ın çağırdığı fonksiyon: rengi ters çevirir
    void DurumDegistir();

    // Materyalin rengini bYesilMi'ye göre günceller
    void RengiGuncelle();

    // Çalışma zamanında üretilen materyal kopyası.
    //
    // Neden dinamik? Normal materyal asset'i paylaşılır — birini değiştirirsen
    // o materyali kullanan HER nesne değişir. Dinamik instance ise
    // sadece bu aktöre ait bir kopyadır, ötekiler etkilenmez.
    //
    // "class UMaterialInstanceDynamic*" yazımı: forward declaration'ı
    // satır içinde yapmanın kısa yolu. Yukarıda ayrı "class X;" yazmaya gerek kalmaz.
    //
    // UPROPERTY() boş parantez: Details'te görünmez ama GC'ye
    // "bu nesne canlı, silme" der. Olmasaydı materyal beklenmedik anda yok olabilirdi.
    UPROPERTY()
    class UMaterialInstanceDynamic* LambaMat;
};