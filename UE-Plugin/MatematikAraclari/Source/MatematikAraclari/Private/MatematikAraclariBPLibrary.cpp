#include "MatematikAraclariBPLibrary.h"
#include "Materials/MaterialInstanceDynamic.h"

float UMatematikAraclariBPLibrary::Toplama(float Sayi1, float Sayi2)
{
	return Sayi1 + Sayi2;
}

float UMatematikAraclariBPLibrary::Cikartma(float Sayi1, float Sayi2)
{
	return Sayi1 - Sayi2;
}

float UMatematikAraclariBPLibrary::Carpma(float Sayi1, float Sayi2)
{
	return Sayi1 * Sayi2;
}

float UMatematikAraclariBPLibrary::Bolme(float Sayi1, float Sayi2)
{
	if (Sayi2 != 0.0)
	{
		return Sayi1 / Sayi2;
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red,TEXT("Bir Sayı Sıfıra Bölünemez!"));
		return -1.f;
	}
}

float UMatematikAraclariBPLibrary::KareAlma(float Sayi)
{
	return Sayi * Sayi;
}

bool UMatematikAraclariBPLibrary::MateryalAta(UPrimitiveComponent* Mesh, UMaterialInterface* YeniMateryal,
                                              int32 SlotIndex, bool All)
{
	if (Mesh == nullptr)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Mesh bos!"));
		return false;
	}
	if (!YeniMateryal)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Materyal bos!"));
		return false;
	}
	if (All)
	{
		for (int i = 0; i < Mesh->GetNumMaterials(); i++)
		{
			Mesh->SetMaterial(i, YeniMateryal);
			//! GEngine->AddOnScreenDebugMessage(-1,5.f,FColor::Red, FString::Printf(TEXT("Materyal Sayısı: %d"),Mesh->GetNumMaterials()));
		}
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green,TEXT("Tum Materyaller degistirildi!"));
		return true;
	}
	if (SlotIndex < 0 || SlotIndex >= Mesh->GetNumMaterials())
	{
		GEngine->AddOnScreenDebugMessage(-1, 20.f, FColor::Red, TEXT("Gecersiz slot index!"));
		return false;
	}


	Mesh->SetMaterial(SlotIndex, YeniMateryal);
	GEngine->AddOnScreenDebugMessage(-1, 20.f, FColor::Yellow, TEXT("Secili Slot Degistirildi!"));
	return true;
}
