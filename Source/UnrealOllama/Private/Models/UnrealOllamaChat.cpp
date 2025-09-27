// Copyright 2025, Muddy Terrain Games, All Rights Reserved.

#include "Models/UnrealOllamaChat.h"
#include "CoreMinimal.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonSerializer.h"
#include "JsonUtilities.h"
#include "UnrealOllamaLog.h"
#include "UnrealOllamaUtils.h"
#include "Engine/Texture2D.h"

TSharedPtr<IHttpRequest, ESPMode::ThreadSafe> UUnrealOllamaChat::SendChatRequest(const FUnrealOllamaChatSettings& ChatSettings, const FOnOllamaChatCompletionResponse& OnComplete)
{
	check(OnComplete.IsBound());
	
	return MakeRequest(ChatSettings, [OnComplete](const FUnrealOllamaChatMessage& Message, const FString& Error, bool Success)
	{
		OnComplete.Execute(Message, Error, Success);
	});
}

UUnrealOllamaChat* UUnrealOllamaChat::RequestOllamaChat(UObject* WorldContextObject, const FUnrealOllamaChatSettings& ChatSettings)
{
	UUnrealOllamaChat* AsyncAction = NewObject<UUnrealOllamaChat>();
	AsyncAction->ChatSettings = ChatSettings;
	AsyncAction->RegisterWithGameInstance(WorldContextObject);
	return AsyncAction;
}

void UUnrealOllamaChat::Activate()
{
	TWeakObjectPtr<UUnrealOllamaChat> WeakThis(this);
	HttpRequest = MakeRequest(ChatSettings, [WeakThis](const FUnrealOllamaChatMessage& Message, const FString& Error, bool Success)
	{
		if (WeakThis.IsValid())
		{
			UUnrealOllamaChat* StrongThis = WeakThis.Get();
			StrongThis->OnComplete.Broadcast(Message, Error, Success);
			StrongThis->Cancel();
		}
		else
		{
			UE_LOG(LogUnrealOllama, Warning, TEXT("UUnrealOllamaChat: Async action object was destroyed before the request completed. Callback skipped."));
		}
	});
}

void UUnrealOllamaChat::Cancel()
{
	if (HttpRequest.IsValid() && HttpRequest->GetStatus() == EHttpRequestStatus::Processing)
	{
		HttpRequest->CancelRequest();
	}
    
	Super::Cancel();
}

TSharedPtr<IHttpRequest, ESPMode::ThreadSafe> UUnrealOllamaChat::MakeRequest(const FUnrealOllamaChatSettings& InChatSettings,
                              const TFunction<void(const FUnrealOllamaChatMessage&, const FString&, bool)>& ResponseCallback)
{
	const FString FullUrl = TEXT("http://localhost:11434/api/chat");

	TSharedPtr<FJsonObject> JsonPayload = MakeShareable(new FJsonObject());
	JsonPayload->SetStringField(TEXT("model"), InChatSettings.Model);
	JsonPayload->SetBoolField(TEXT("stream"), false);

	TArray<TSharedPtr<FJsonValue>> MessagesArray;
	for (const auto& Message : InChatSettings.Messages)
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

	if (!InChatSettings.Format.IsEmpty())
	{
		JsonPayload->SetStringField(TEXT("format"), InChatSettings.Format);
	}

	FString PayloadString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&PayloadString);
	FJsonSerializer::Serialize(JsonPayload.ToSharedRef(), Writer);

	UE_LOG(LogUnrealOllama, Log, TEXT("Ollama Chat Request: %s"), *PayloadString);

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> NewHttpRequest = FHttpModule::Get().CreateRequest();
	NewHttpRequest->SetVerb(TEXT("POST"));
	NewHttpRequest->SetURL(FullUrl);
	NewHttpRequest->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	NewHttpRequest->SetContentAsString(PayloadString);

	NewHttpRequest->OnProcessRequestComplete().BindLambda(
		[ResponseCallback](FHttpRequestPtr Request, const FHttpResponsePtr& Response, const bool bSuccess)
		{
			if (!bSuccess || !Response.IsValid())
			{
                FUnrealOllamaChatMessage ErrorMessage;
				ResponseCallback(ErrorMessage, TEXT("Request failed or response invalid"), false);
				return;
			}

			FString ResponseStr = Response->GetContentAsString();
			UE_LOG(LogUnrealOllama, Log, TEXT("Ollama Chat Response: %s"), *ResponseStr);
			ProcessResponse(ResponseStr, ResponseCallback);
		});

	NewHttpRequest->ProcessRequest();
	return NewHttpRequest;
}

void UUnrealOllamaChat::ProcessResponse(const FString& ResponseStr,
                                  const TFunction<void(const FUnrealOllamaChatMessage&, const FString&, bool)>& ResponseCallback)
{
    FUnrealOllamaChatResponse Response;
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseStr);

    if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
    {
        FString ErrorMessageStr;
        if (JsonObject->TryGetStringField(TEXT("error"), ErrorMessageStr))
        {
            FUnrealOllamaChatMessage ErrorMessage;
            ResponseCallback(ErrorMessage, ErrorMessageStr, false);
            return;
        }

        if (FJsonObjectConverter::JsonObjectStringToUStruct(ResponseStr, &Response, 0, 0))
        {
            ResponseCallback(Response.Message, TEXT(""), true);
            return;
        }
    }

    FString ErrorMsg = FString::Printf(TEXT("Failed to parse response: %s"), *ResponseStr);
    FUnrealOllamaChatMessage ErrorMessage;
	ResponseCallback(ErrorMessage, ErrorMsg, false);
}
