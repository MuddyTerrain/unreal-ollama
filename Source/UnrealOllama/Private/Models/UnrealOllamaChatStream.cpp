// Copyright 2025, Muddy Terrain Games, All Rights Reserved.

#include "Models/UnrealOllamaChatStream.h"
#include "CoreMinimal.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "JsonUtilities.h"
#include "UnrealOllamaLog.h"
#include "Runtime/Launch/Resources/Version.h"

UUnrealOllamaChatStream* UUnrealOllamaChatStream::RequestOllamaChatStream(UObject* WorldContextObject,
                                                              const FUnrealOllamaChatSettings& ChatSettings)
{
	UUnrealOllamaChatStream* AsyncAction = NewObject<UUnrealOllamaChatStream>();
	AsyncAction->ChatSettings = ChatSettings;
	AsyncAction->RegisterWithGameInstance(WorldContextObject);
	return AsyncAction;
}

TSharedPtr<IHttpRequest, ESPMode::ThreadSafe> UUnrealOllamaChatStream::SendStreamChatRequest(
	const FUnrealOllamaChatSettings& InChatSettings, const FOnUnrealOllamaChatStreamResponse& OnEventCallback)
{
	UUnrealOllamaChatStream* AsyncAction = NewObject<UUnrealOllamaChatStream>();
	AsyncAction->ChatSettings = InChatSettings;
	AsyncAction->OnResponseCallback = OnEventCallback;

	AsyncAction->AddToRoot();
	AsyncAction->Activate();
	return AsyncAction->HttpRequest;
}

void UUnrealOllamaChatStream::Activate()
{
	StreamBuffer.Empty();
	LastProcessedStringLength = 0;
	bWasCancelled = false;
	
	const FString FullUrl = TEXT("http://localhost:11434/api/chat");

	TSharedPtr<FJsonObject> JsonPayload = MakeShareable(new FJsonObject());
	JsonPayload->SetStringField(TEXT("model"), ChatSettings.Model);
	JsonPayload->SetBoolField(TEXT("stream"), true);

	TArray<TSharedPtr<FJsonValue>> MessagesArray;
	for (const auto& Message : ChatSettings.Messages)
	{
		TSharedPtr<FJsonObject> MessageObject = MakeShareable(new FJsonObject());
		MessageObject->SetStringField(TEXT("role"), Message.Role);
		MessageObject->SetStringField(TEXT("content"), Message.Content);
		
		TArray<TSharedPtr<FJsonValue>> ImagesJsonArray;
		for (const FString& Base64Image : Message.Images)
		{
			ImagesJsonArray.Add(MakeShareable(new FJsonValueString(Base64Image)));
		}
		for (UTexture2D* Texture : Message.ImagesAsTextures)
		{
			FString Base64Image = FUnrealOllamaUtils::TextureToBase64(Texture);
			if (!Base64Image.IsEmpty())
			{
				ImagesJsonArray.Add(MakeShareable(new FJsonValueString(Base64Image)));
			}
		}

		if (ImagesJsonArray.Num() > 0)
		{
			MessageObject->SetArrayField(TEXT("images"), ImagesJsonArray);
		}
		
		MessagesArray.Add(MakeShareable(new FJsonValueObject(MessageObject)));
	}
	JsonPayload->SetArrayField(TEXT("messages"), MessagesArray);

	if (!ChatSettings.Format.IsEmpty())
	{
		JsonPayload->SetStringField(TEXT("format"), ChatSettings.Format);
	}

	FString PayloadString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&PayloadString);
	FJsonSerializer::Serialize(JsonPayload.ToSharedRef(), Writer);

	UE_LOG(LogUnrealOllama, Log, TEXT("Ollama Chat Stream Request: %s"), *PayloadString);

	HttpRequest = FHttpModule::Get().CreateRequest();
	HttpRequest->SetVerb(TEXT("POST"));
	HttpRequest->SetURL(FullUrl);
	HttpRequest->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	HttpRequest->SetContentAsString(PayloadString);

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 4
	HttpRequest->OnRequestProgress64().BindUObject(this, &UUnrealOllamaChatStream::HandleRequestProgress);
#else
	HttpRequest->OnRequestProgress().BindUObject(this, &UUnrealOllamaChatStream::HandleRequestProgress_Compatibility);
#endif
	
	HttpRequest->OnProcessRequestComplete().BindUObject(this, &UUnrealOllamaChatStream::HandleRequestCompletion);

	if (!HttpRequest->ProcessRequest())
	{
		FUnrealOllamaStreamEvent ErrorEvent;
		OnEvent.Broadcast(ErrorEvent);
		(void)OnResponseCallback.ExecuteIfBound(ErrorEvent);
		Cancel();
	}
}

void UUnrealOllamaChatStream::HandleRequestProgress_Compatibility(FHttpRequestPtr Request, int32 BytesSent, int32 BytesReceived)
{
	HandleRequestProgress(Request, static_cast<uint64>(BytesSent), static_cast<uint64>(BytesReceived));
}

void UUnrealOllamaChatStream::HandleRequestProgress(FHttpRequestPtr Request, uint64 BytesSent, uint64 BytesReceived)
{
	FHttpResponsePtr Response = Request->GetResponse();
	if (!Response.IsValid())
	{
		return;
	}

	const FString FullResponseContent = Response->GetContentAsString();
	if (FullResponseContent.Len() <= LastProcessedStringLength)
	{
		return;
	}

	FString NewData = FullResponseContent.RightChop(LastProcessedStringLength);
	LastProcessedStringLength = FullResponseContent.Len();

	StreamBuffer.Append(NewData);
	
    int32 EOLPos = -1;
    while ((EOLPos = StreamBuffer.Find(TEXT("\n"), ESearchCase::CaseSensitive)) != INDEX_NONE)
    {
        FString Line = StreamBuffer.Left(EOLPos);
#if ENGINE_MAJOR_VERSION > 5 || (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 4)
        StreamBuffer.RightChopInline(EOLPos + 1, EAllowShrinking::No);
#else
        StreamBuffer.RightChopInline(EOLPos + 1, false);
#endif
		ProcessSSEChunk(Line);
    }
}

void UUnrealOllamaChatStream::ProcessSSEChunk(const FString& ChunkData)
{
	UE_LOG(LogUnrealOllama, Log, TEXT("Ollama Stream Chunk: %s"), *ChunkData);

    FUnrealOllamaChatResponse FullResponse;
    if (FJsonObjectConverter::JsonObjectStringToUStruct(ChunkData, &FullResponse, 0, 0))
    {
		FUnrealOllamaStreamEvent StreamEvent;
        StreamEvent.PartialMessage = FullResponse.Message;
        if (FullResponse.bDone)
        {
            StreamEvent.bIsFinished = true;
        }
		OnEvent.Broadcast(StreamEvent);
		(void)OnResponseCallback.ExecuteIfBound(StreamEvent);
    }
}

void UUnrealOllamaChatStream::HandleRequestCompletion(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccessHttp)
{
    if (bWasCancelled)
    {
        UE_LOG(LogUnrealOllama, Log, TEXT("OllamaChatStream: Request was cancelled by the user."));
    }
    else if (bSuccessHttp && Response.IsValid() && Response->GetResponseCode() >= 200 && Response->GetResponseCode() < 300)
    {
        HandleRequestProgress(Request, 0, Response->GetContentLength());

        FUnrealOllamaStreamEvent CompletionEvent;
        CompletionEvent.bIsFinished = true;
        OnEvent.Broadcast(CompletionEvent);
        (void)OnResponseCallback.ExecuteIfBound(CompletionEvent);
    }
    else
    {
        FString ErrorMsg;
        if (Response.IsValid())
        {
            ErrorMsg = FString::Printf(
                TEXT("HTTP Request failed. Code: %d. Response: %s"), Response->GetResponseCode(),
                *Response->GetContentAsString().Left(512));
        }
        else
        {
            ErrorMsg = TEXT("Request failed with no valid response object.");
        }

        FUnrealOllamaStreamEvent ErrorEvent;
        UE_LOG(LogUnrealOllama, Error, TEXT("OllamaChatStream: %s"), *ErrorMsg);
        OnEvent.Broadcast(ErrorEvent);
        (void)OnResponseCallback.ExecuteIfBound(ErrorEvent);
    }

    HttpRequest.Reset();
    if (IsRooted())
    {
        RemoveFromRoot();
    }
    SetReadyToDestroy();
}

void UUnrealOllamaChatStream::Cancel()
{
    if (HttpRequest.IsValid() &&
        (HttpRequest->GetStatus() == EHttpRequestStatus::Processing || HttpRequest->GetStatus() ==
            EHttpRequestStatus::NotStarted))
    {
        bWasCancelled = true;
        HttpRequest->CancelRequest();
    }

    Super::Cancel();
}
