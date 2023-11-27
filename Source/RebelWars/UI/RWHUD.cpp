#include "UI/RWHUD.h"

#include "Blueprint/UserWidget.h"

ARWHUD::ARWHUD()
	: Super()
{
	HUDWidgetInstance = nullptr;
}

void ARWHUD::BeginPlay()
{
	Super::BeginPlay();

	if (HUDWidgetClass)
	{
		HUDWidgetInstance = CreateWidget(PlayerOwner, HUDWidgetClass, FName(TEXT("HUD User Widget")));
		if (HUDWidgetInstance)
		{
			HUDWidgetInstance->AddToViewport();
		}
	}
}
