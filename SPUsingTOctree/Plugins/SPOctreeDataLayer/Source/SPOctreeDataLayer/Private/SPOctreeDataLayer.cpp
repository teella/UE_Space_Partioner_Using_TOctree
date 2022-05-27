#include "SPOctreeDataLayer.h"

#define LOCTEXT_NAMESPACE "FSPOctreeDataLayerModule"
DEFINE_LOG_CATEGORY(SPOctreeDataLayerMod);

void FSPOctreeDataLayerModule::StartupModule()
{
}

void FSPOctreeDataLayerModule::ShutdownModule()
{
}

#if WITH_EDITOR
UDataLayerEditorSubsystem* FSPOctreeDataLayerModule::GetDataLayerSystem()
{
	return UDataLayerEditorSubsystem::Get();
}
#endif

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FSPOctreeDataLayerModule, SPOctreeDataLayer)
