// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MyUserWidget.generated.h"

/**
 * 
 */
UCLASS()
class UEOGRENME_API UMyUserWidget : public UUserWidget
{
	GENERATED_BODY()
	
	// meta = (BindWidget) → C++ ile Blueprint widget'ı arasında OTOMATIK bağ kurar.
    // UE, bu C++ sınıfından türeyen WBP'de "MyText" ADINDA bir Text Block arar
    // ve bulduğunu bu pointer'a kendisi atar. Sen elle atama yapmazsın.
    //
    // KRİTİK: İsim birebir aynı olmalı. WBP'deki Text Block'un adı da "MyText" olmalı,
    // yoksa BP derlenmez ve "A required widget binding is missing" hatası verir.
    //
    // class UTextBlock* → "class" öneki = inline forward declaration.
    // Header'da TextBlock.h include etmeden tip tanıtmanın kısa yolu.
    // .cpp'de kullanmadan önce gerçek include gerekir: #include "Components/TextBlock.h"
	UPROPERTY(meta = (BindWidget))
	class UTextBlock* MyText;
	
	UPROPERTY(meta = (BindWidget))
	class UTextBlock* MyCounterText;
	
	float GecenSure;
	
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
};
