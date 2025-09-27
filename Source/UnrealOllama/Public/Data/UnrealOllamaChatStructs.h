// Copyright 2025, Muddy Terrain Games, All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UnrealOllama.h"
#include "UnrealOllamaChatStructs.generated.h"

USTRUCT(BlueprintType)
struct UNREALOLLAMA_API FUnrealOllamaChatMessage
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnrealOllama")
    FString Role;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnrealOllama")
    FString Content;

    // For multimodal models, as Base64 encoded strings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnrealOllama", meta = (ToolTip = "A list of Base64-encoded images to include in the message."))
    TArray<FString> Images;

    // For multimodal models, as UTexture2D objects
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnrealOllama", meta = (ToolTip = "A list of Texture2D assets to include in the message. These will be converted to Base64 internally."))
    TArray<UTexture2D*> ImagesAsTextures;
};

USTRUCT(BlueprintType)
struct UNREALOLLAMA_API FUnrealOllamaChatSettings
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnrealOllama")
    FString Model;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnrealOllama")
    TArray<FUnrealOllamaChatMessage> Messages;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnrealOllama", meta = (ToolTip = "The format to return a response in. Currently the only accepted value is json"))
    FString Format;

    // Add other options from the Ollama API as needed, e.g., temperature, top_p, etc.
    // UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnrealOllama")
    // float Temperature = 0.8f;
};

USTRUCT(BlueprintType)
struct UNREALOLLAMA_API FUnrealOllamaChatResponse
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UnrealOllama")
    FString Model;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UnrealOllama")
    FString CreatedAt;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UnrealOllama")
    FUnrealOllamaChatMessage Message;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UnrealOllama")
    bool bDone = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UnrealOllama")
    FString ErrorMessage;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UnrealOllama")
    bool bSuccess = false;
};

UCLASS()
class UNREALOLLAMA_API UUnrealOllamaChatStructs : public UObject
{
    GENERATED_BODY()
};
