#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Components/PrimitiveComponent.h"
#include "Materials/MaterialInterface.h"
#include "MatematikAraclariBPLibrary.generated.h"

UCLASS()
class MATEMATIKARACLARI_API UMatematikAraclariBPLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Matematik Araclari")
	static float Toplama(float Sayi1, float Sayi2);

	UFUNCTION(BlueprintCallable, Category = "Matematik Araclari")
	static float Cikartma(float Sayi1, float Sayi2);

	UFUNCTION(BlueprintCallable, Category = "Matematik Araclari")
	static float Carpma(float Sayi1, float Sayi2);

	UFUNCTION(BlueprintCallable, Category = "Matematik Araclari")
	static float Bolme(float Sayi1, float Sayi2);

	UFUNCTION(BlueprintCallable, Category = "Matematik Araclari")
	static float KareAlma(float Sayi);

	UFUNCTION(BlueprintCallable, Category = "Matematik Araclari|Gorsel")
	static bool MateryalAta(UPrimitiveComponent* Mesh, UMaterialInterface* YeniMateryal, int32 SlotIndex = 0,
	                        bool All = false);
};
