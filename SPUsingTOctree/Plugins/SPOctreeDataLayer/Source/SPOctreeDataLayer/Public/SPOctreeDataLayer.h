#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#if WITH_EDITOR
#include "DataLayer/DataLayerEditorSubsystem.h"
#endif

DECLARE_LOG_CATEGORY_EXTERN(SPOctreeDataLayerMod, Log, All);
class FSPOctreeDataLayerModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

#if WITH_EDITOR
	UDataLayerEditorSubsystem* GetDataLayerSystem();
#endif

};
