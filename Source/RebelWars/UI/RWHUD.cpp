#include "UI/RWHUD.h"

#include "Blueprint/UserWidget.h"

ARWHUD::ARWHUD()
	: Super()
{
	HUDWidgetInstance = nullptr;
}

void ARWHUD::SetHUDWidgetVisibility(bool bVisibility)
{
	if (HUDWidgetInstance)
	{
		HUDWidgetInstance->SetVisibility(bVisibility ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	}
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
			SetHUDWidgetVisibility(false);
		}
	}
}
