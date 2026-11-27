// UE4.27 RDG adapter for MWSR's single-pass spatial reconstruction.
#include "BMIUnifiedSpatialUpscaler.h"
#include "ScreenPass.h"

BEGIN_SHADER_PARAMETER_STRUCT(FBMIMWSRConstants, )
    SHADER_PARAMETER(FVector2D, texel)
    SHADER_PARAMETER(FVector2D, scale)
    SHADER_PARAMETER(FVector2D, dims)
    SHADER_PARAMETER(FVector2D, across)
    SHADER_PARAMETER(float, kc)
    SHADER_PARAMETER(float, ft)
    SHADER_PARAMETER(float, kg)
    SHADER_PARAMETER(float, kwc)
    SHADER_PARAMETER(FVector2D, zc)
    SHADER_PARAMETER(FVector2D, zr)
    SHADER_PARAMETER(float, zring)
    SHADER_PARAMETER(float, zft)
    SHADER_PARAMETER(float, zkg)
    SHADER_PARAMETER(float, gaze)
    SHADER_PARAMETER(float, kl)
    SHADER_PARAMETER(float, zref)
    SHADER_PARAMETER(float, pad1)
    SHADER_PARAMETER(float, pad2)
END_SHADER_PARAMETER_STRUCT()

class FBMIMWSRPS final : public FGlobalShader
{
public:
    DECLARE_GLOBAL_SHADER(FBMIMWSRPS);
    SHADER_USE_PARAMETER_STRUCT(FBMIMWSRPS, FGlobalShader);
    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
        SHADER_PARAMETER_STRUCT(FBMIMWSRConstants, MwsrConstants)
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, tex)
        SHADER_PARAMETER_SAMPLER(SamplerState, samp)
        RENDER_TARGET_BINDING_SLOTS()
    END_SHADER_PARAMETER_STRUCT()
    static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
    {
        return IsMobilePlatform(Parameters.Platform) ||
            IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
    }
};
IMPLEMENT_GLOBAL_SHADER(FBMIMWSRPS, "/Plugin/BMI/BMIShaders/Private/BMIMWSR.usf", "MwsrPS", SF_Pixel);

static FBMIMWSRConstants BuildBMIMWSRConstants(int32 InW, int32 InH, int32 OutW, int32 OutH, float Sharpness)
{
    const double S = FMath::Clamp(FMath::Min(double(OutW) / InW, double(OutH) / InH), 1.07, 4.0);
    const double X = FMath::Loge(S);
    const double KC = 0.6353583202388096 + X * (0.021845681633595574 + X * 0.12781268100437676);
    const double KG0 = -0.1288413863403059 + X * (2.5844683855070816 - X * 1.260092021482419);
    const double KR = 1.0071546009267338 + X * (-1.0209066704914191 + X * 0.5699605831985769);
    const double KL0 = 0.33416994543071277 + X * (-0.2962022126856069 + X * 0.08628272130287401);
    const double Force = 1.15 + (1.35 - 1.15) * FMath::Clamp(
        FMath::Loge(S / 1.3333333333333333) / FMath::Loge(2.0 / 1.3333333333333333), 0.0, 1.0);
    const double Strength = FMath::Clamp((S - 1.0) / 0.07, 0.0, 1.0);
    const double SharpnessScale = FMath::Lerp(0.5, 1.5, double(FMath::Clamp(Sharpness, 0.0f, 1.0f)));

    FBMIMWSRConstants C;
    C.texel = FVector2D(1.0 / InW, 1.0 / InH);
    C.scale = FVector2D(double(OutW) / InW, double(OutH) / InH);
    C.dims = FVector2D(InW, InH);
    C.across = FVector2D(KR / InW, KR / InH);
    C.kc = float(KC);
    C.ft = 0.005f;
    C.kg = float(KG0 * Force * Strength * SharpnessScale);
    C.kwc = 0.1f;
    C.zc = FVector2D(0.5, 0.5);
    C.zr = FVector2D(0.2, 0.45);
    C.zring = 0.75f;
    C.zft = 0.005f;
    C.zkg = C.kg;
    C.gaze = 0.0f;
    C.kl = float(KL0 * Force * Strength * SharpnessScale);
    C.zref = 0.0f;
    C.pad1 = C.pad2 = 0.0f;
    return C;
}

static FScreenPassTexture AddBMIMWSRPass(
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
        FRDGTextureRef Texture = GraphBuilder.CreateTexture(Desc, TEXT("BMI-MWSR-Output"));
        Output = FScreenPassRenderTarget(
            Texture, FIntRect(FIntPoint::ZeroValue, OutputExtent),
            ERenderTargetLoadAction::ENoAction);
    }

    const FScreenPassTextureViewport InputViewport(Inputs.SceneColor);
    const FScreenPassTextureViewport OutputViewport(Output);
    FBMIMWSRPS::FParameters* Parameters = GraphBuilder.AllocParameters<FBMIMWSRPS::FParameters>();
    Parameters->MwsrConstants = BuildBMIMWSRConstants(
        InputViewport.Rect.Width(), InputViewport.Rect.Height(),
        OutputViewport.Rect.Width(), OutputViewport.Rect.Height(), Sharpness);
    Parameters->tex = Inputs.SceneColor.Texture;
    Parameters->samp = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
    Parameters->RenderTargets[0] = FRenderTargetBinding(Output.Texture, ERenderTargetLoadAction::ENoAction);

    TShaderMapRef<FBMIMWSRPS> PixelShader(View.ShaderMap);
    AddDrawScreenPass(GraphBuilder, RDG_EVENT_NAME("BMI/MWSR"),
        View, OutputViewport, InputViewport, PixelShader, Parameters);
    return Output;
}

void RegisterBMIMWSRUpscaler()
{
    FBMIUnifiedSpatialUpscaler::RegisterUpscaler(2, AddBMIMWSRPass);
}
