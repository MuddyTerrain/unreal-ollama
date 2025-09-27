// Copyright 2025, Muddy Terrain Games, All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UnrealOllama.h"
#include "Data/UnrealOllamaChatStructs.h"
#include "Engine/CancellableAsyncAction.h"
#include "HttpModule.h"
#include "UnrealOllamaChat.generated.h"

class IHttpRequest;

DECLARE_DELEGATE_ThreeParams(FOnOllamaChatCompletionResponse, const FUnrealOllamaChatMessage&, const FString&, bool);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FUnrealOllamaChatCompletionDelegate, const FUnrealOllamaChatMessage&, Message, const FString&, Error, bool, Success);

UCLASS(meta=(ToolTip="Asynchronous action for Ollama chat completion requests."))
class UNREALOLLAMA_API UUnrealOllamaChat : public UCancellableAsyncAction
{
    GENERATED_BODY()

public:
    static TSharedPtr<IHttpRequest, ESPMode::ThreadSafe> SendChatRequest(const FUnrealOllamaChatSettings& ChatSettings, const FOnOllamaChatCompletionResponse& OnComplete);

    UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject", DisplayName = "Request Ollama Chat Completion", Category = "UnrealOllama", ToolTip="Starts an asynchronous Ollama chat completion request."))
    static UUnrealOllamaChat* RequestOllamaChat(UObject* WorldContextObject, const FUnrealOllamaChatSettings& ChatSettings);
    
    UPROPERTY(BlueprintAssignable, meta=(ToolTip="Delegate triggered when the Ollama chat request completes or fails."))
    FUnrealOllamaChatCompletionDelegate OnComplete;
    
    virtual void Cancel() override;

private:
    FUnrealOllamaChatSettings ChatSettings;

    static TSharedPtr<IHttpRequest, ESPMode::ThreadSafe> MakeRequest(const FUnrealOllamaChatSettings& ChatSettings, const TFunction<void(const FUnrealOllamaChatMessage&, const FString&, bool)>& ResponseCallback);
    
    static void ProcessResponse(const FString& ResponseStr, const TFunction<void(const FUnrealOllamaChatMessage&, const FString&, bool)>& ResponseCallback);
    
    TSharedPtr<IHttpRequest> HttpRequest;

protected:
    virtual void Activate() override;
};
