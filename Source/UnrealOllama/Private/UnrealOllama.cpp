// Copyright 2025, Muddy Terrain Games, All Rights Reserved.

#include "UnrealOllama.h"
#include "UnrealOllamaLog.h"

#define LOCTEXT_NAMESPACE "FUnrealOllamaModule"

DEFINE_LOG_CATEGORY(LogUnrealOllama);

void FUnrealOllamaModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	UE_LOG(LogUnrealOllama, Log, TEXT("UnrealOllama Module Started"));
}

void FUnrealOllamaModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
	UE_LOG(LogUnrealOllama, Log, TEXT("UnrealOllama Module Shutdown"));
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FUnrealOllamaModule, UnrealOllama)
