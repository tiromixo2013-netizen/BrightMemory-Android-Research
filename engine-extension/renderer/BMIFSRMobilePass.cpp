// UE4.27 Renderer-private RDG adapter for the optimized mobile FSR1 shader.
#include "BMIUnifiedSpatialUpscaler.h"
#include "ScreenPass.h"

class FBMIFSRMobilePS final : public FGlobalShader
{
public:
    DECLARE_GLOBAL_SHADER(FBMIFSRMobilePS);
    SHADER_USE_PARAMETER_STRUCT(FBMIFSRMobilePS, FGlobalShader);
    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, InputTexture)
        SHADER_PARAMETER_SAMPLER(SamplerState, LinearClamp)
        SHADER_PARAMETER(FVector2D, InputViewportSize)
        SHADER_PARAMETER(FVector2D, InputTextureSize)
        SHADER_PARAMETER(FVector2D, OutputViewportSize)
        SHADER_PARAMETER(FVector2D, InputViewportMin)
        SHADER_PARAMETER(float, Sharpness)
        RENDER_TARGET_BINDING_SLOTS()
    END_SHADER_PARAMETER_STRUCT()
    static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
    {
        return IsMobilePlatform(Parameters.Platform) ||
            IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
    }
};
IMPLEMENT_GLOBAL_SHADER(FBMIFSRMobilePS, "/Plugin/BMI/BMIShaders/Private/BMIFSRMobile.usf", "MainPS", SF_Pixel);

static FScreenPassTexture AddBMIFSRMobilePass(
    FRDGBuilder& GraphBuilder,
    const FViewInfo& View,
    const ISpatialUpscaler::FInputs& Inputs,
    float Sharpness)
{
    check(Inputs.SceneColor.IsValid());
    const FIntPoint OutputExtent = View.UnscaledViewRect.Size();
    FScreenPassRenderTarget Output = Inputs.OverrideOutput;
    if (!Output.IsValid())
    {
        FRDGTextureDesc Desc = FRDGTextureDesc::Create2D(
            OutputExtent, Inputs.SceneColor.Texture->Desc.Format,
            FClearValueBinding::Black,
            TexCreate_RenderTargetable | TexCreate_ShaderResource);
        FRDGTextureRef Texture = GraphBuilder.CreateTexture(Desc, TEXT("BMI-FSRMobile-Output"));
        Output = FScreenPassRenderTarget(
            Texture, FIntRect(FIntPoint::ZeroValue, OutputExtent),
            ERenderTargetLoadAction::ENoAction);
    }

    const FScreenPassTextureViewport InputViewport(Inputs.SceneColor);
    const FScreenPassTextureViewport OutputViewport(Output);
    FBMIFSRMobilePS::FParameters* Parameters =
        GraphBuilder.AllocParameters<FBMIFSRMobilePS::FParameters>();
    Parameters->InputTexture = Inputs.SceneColor.Texture;
    Parameters->LinearClamp = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
    Parameters->InputViewportSize = FVector2D(InputViewport.Rect.Width(), InputViewport.Rect.Height());
    Parameters->InputTextureSize = FVector2D(
        Inputs.SceneColor.Texture->Desc.Extent.X, Inputs.SceneColor.Texture->Desc.Extent.Y);
    Parameters->OutputViewportSize = FVector2D(OutputViewport.Rect.Width(), OutputViewport.Rect.Height());
    Parameters->InputViewportMin = FVector2D(InputViewport.Rect.Min.X, InputViewport.Rect.Min.Y);
    Parameters->Sharpness = FMath::Clamp(Sharpness, 0.0f, 1.0f);
    Parameters->RenderTargets[0] = FRenderTargetBinding(
        Output.Texture, ERenderTargetLoadAction::ENoAction);

    TShaderMapRef<FBMIFSRMobilePS> PixelShader(View.ShaderMap);
    AddDrawScreenPass(GraphBuilder, RDG_EVENT_NAME("BMI/FSR-Mobile"),
        View, OutputViewport, InputViewport, PixelShader, Parameters);
    return Output;
}

void RegisterBMIFSRMobileUpscaler()
{
    FBMIUnifiedSpatialUpscaler::RegisterUpscaler(3, AddBMIFSRMobilePass);
}
