#include "RWGameInstance.h"

 #include "UI/UIManager.h"

UUIManager* URWGameInstance::GetUIManager()
{
	return UIManager;
}

void URWGameInstance::Init()
{
	Super::Init();

	UIManager = NewObject<UUIManager>(GetTransientPackage());
	if (UIManager)
	{
		UIManager->CreateWidgets();
	}
}
