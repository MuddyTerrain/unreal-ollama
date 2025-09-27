// Copyright 2025, Muddy Terrain Games, All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/Texture2D.h"
#include "Misc/Base64.h"
#include "IImageWrapperModule.h"
#include "IImageWrapper.h"
#include "Modules/ModuleManager.h"
#include "TextureResource.h"
#include "RHI.h"
#include "RHICommandList.h"
#include "RenderCommandFence.h"
#include "RenderingThread.h"

class FUnrealOllamaUtils
{
public:
    static FString TextureToBase64(UTexture2D* Texture)
    {
        if (!Texture || !Texture->GetResource())
        {
            return TEXT("");
        }

        FTextureResource* TextureResource = Texture->GetResource();
        if (!TextureResource)
        {
            return TEXT("");
        }

        TArray<FColor> RawData;
        FRHITexture* RHITexture = TextureResource->GetTextureRHI();

        if (!RHITexture)
        {
            return TEXT("");
        }

        FIntPoint Size(TextureResource->GetSizeX(), TextureResource->GetSizeY());

        FRenderCommandFence Fence;
        ENQUEUE_RENDER_COMMAND(ReadSurfaceData)(
            [RHITexture, &RawData, Size](FRHICommandListImmediate& RHICmdList)
            {
                FReadSurfaceDataFlags ReadSurfaceDataFlags;
                ReadSurfaceDataFlags.SetLinearToGamma(false);
                RHICmdList.ReadSurfaceData(
                    RHITexture,
                    FIntRect(0, 0, Size.X, Size.Y),
                    RawData,
                    ReadSurfaceDataFlags
                );
            }
        );
        Fence.BeginFence();
        Fence.Wait();

        if (RawData.Num() == 0)
        {
            return TEXT("");
        }

        IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
        TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::PNG);

        if (ImageWrapper.IsValid() && ImageWrapper->SetRaw(RawData.GetData(), RawData.Num() * sizeof(FColor), Size.X, Size.Y, ERGBFormat::BGRA, 8))
        {
            const TArray64<uint8>& PngData = ImageWrapper->GetCompressed();
            return FBase64::Encode(TArray<uint8>(PngData.GetData(), PngData.Num()));
        }

        return TEXT("");
    }
};
