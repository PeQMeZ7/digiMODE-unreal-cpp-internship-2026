// Fill out your copyright notice in the Description page of Project Settings.


#include "MyUserWidget.h"

#include "Components/TextBlock.h"

void UMyUserWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	// SetText → Text Block'un ekranda gösterdiği yazıyı değiştirir.
	// Ama SetText FString kabul etmez, FText ister — bu yüzden çevirme gerekir.
	//
	// FText::FromString(...) → FString'i FText'e çevirir.
	// Neden iki farklı tip var?
	//   FString → düz metin. Dosya yolu, isim, log gibi teknik işler için.
	//   FText   → KULLANICIYA gösterilen metin. Dil desteği (localization) taşır,
	//             yani ileride oyunu İngilizce'ye çevirirsen bu metinler tabloya girer.
	// Arayüzdeki her yazı FText'tir, bu yüzden çevirmeden veremezsin.
	MyText->SetText(FText::FromString(TEXT("UNREAL ÖĞRENME PROJESİ - CPP TARAFINDAN DOLDURULDU")));
}

void UMyUserWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	
	GecenSure += InDeltaTime;
	
	// İç içe üç işlem var, EN İÇTEN dışa doğru okunur:
	//
	// 1) FString::Printf(...) → şablon metne değişken yerleştirip FString üretir.
	//    TEXT("...") sadece METNİ sarar ve KAPANIR. Değişkenler virgülden sonra,
	//    TEXT'in DIŞINDA verilir — içine yazarsan "too many arguments" hatası alırsın.
	//    %d → int32 için yer tutucu. Kaç tane %X varsa o kadar değişken verilir.
	//    (InDeltaTime float ise %d yerine %.0f kullanılmalı, yoksa çöp değer basar)
	//
	// 2) FText::FromString(...) → üretilen FString'i FText'e çevirir.
	//    Arayüzdeki her yazı FText'tir (dil desteği taşır), SetText FString kabul etmez.
	//
	// 3) SetText(...) → Text Block'un ekranda gösterdiği yazıyı günceller.
	MyCounterText->SetText(FText::FromString(FString::Printf(TEXT("Geçen Süre: %0.f Saniye - CPP TARAFINDAN DOLDURULDU"), GecenSure)));
	
}

