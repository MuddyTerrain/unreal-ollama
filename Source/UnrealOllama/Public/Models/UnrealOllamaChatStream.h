// Copyright 2025, Muddy Terrain Games, All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UnrealOllama.h"
#include "Data/UnrealOllamaChatStructs.h"
#include "Engine/CancellableAsyncAction.h"
#include "HttpModule.h"
#include "UnrealOllamaChatStream.generated.h"

class IHttpRequest;

USTRUCT(BlueprintType)
struct UNREALOLLAMA_API FUnrealOllamaStreamEvent
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "UnrealOllama")
    FUnrealOllamaChatMessage PartialMessage;

    UPROPERTY(BlueprintReadOnly, Category = "UnrealOllama")
    bool bIsFinished = false;
};

DECLARE_DELEGATE_OneParam(FOnUnrealOllamaChatStreamResponse, const FUnrealOllamaStreamEvent&);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUnrealOllamaStreamEvent, const FUnrealOllamaStreamEvent&, StreamEvent);

UCLASS(meta = (ToolTip = "Asynchronous action for streaming Ollama chat completion requests."))
class UNREALOLLAMA_API UUnrealOllamaChatStream : public UCancellableAsyncAction
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintAssignable)
    FOnUnrealOllamaStreamEvent OnEvent;

    UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject", DisplayName = "Request Ollama Chat Stream",  Category = "UnrealOllama", ToolTip = "Starts an asynchronous streaming request to Ollama for chat completion."))
    static UUnrealOllamaChatStream* RequestOllamaChatStream(UObject* WorldContextObject, const FUnrealOllamaChatSettings& ChatSettings);

    static TSharedPtr<IHttpRequest, ESPMode::ThreadSafe> SendStreamChatRequest(const FUnrealOllamaChatSettings& ChatSettings, const FOnUnrealOllamaChatStreamResponse& OnEventCallback);

    virtual void Cancel() override;

private:
    FUnrealOllamaChatSettings ChatSettings;
    TSharedPtr<IHttpRequest, ESPMode::ThreadSafe> HttpRequest;
    FOnUnrealOllamaChatStreamResponse OnResponseCallback;
    int32 LastProcessedStringLength = 0;
    FString StreamBuffer; 
    bool bWasCancelled = false;

    void HandleRequestProgress(FHttpRequestPtr Request, uint64 BytesSent, uint64 BytesReceived);
	void HandleRequestProgress_Compatibility(FHttpRequestPtr Request, int32 BytesSent, int32 BytesReceived);
    void HandleRequestCompletion(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess);
    void ProcessSSEChunk(const FString& Chunk);

protected:
    virtual void Activate() override;
};
