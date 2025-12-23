struct VertexOut
{
    float4 FinalPosition : SV_POSITION;
    float2 TextureCoordinate : TEXTURE_COORDINATE;
};

struct GBuffer
{
    float4 SceneColor;
    float SceneDepth;
    float ShadowMap;
};

SamplerState linearSampler : register(s0, space0);
SamplerState pointWrapSampler : register(s0, space1);
Texture2D<float4> shaderResources[3] : register(t0);

void SetupGBuffer(inout GBuffer gbuffer, in float2 textureCoordinate)
{
    gbuffer.SceneColor = shaderResources[0].Sample(linearSampler, textureCoordinate);
    gbuffer.SceneDepth = shaderResources[1].Sample(linearSampler, textureCoordinate).r;
    gbuffer.ShadowMap = shaderResources[2].Sample(pointWrapSampler, textureCoordinate).r;
}

float4 main(VertexOut input) : SV_TARGET
{
    GBuffer gbuffer;
    SetupGBuffer(gbuffer, input.TextureCoordinate);
    
    float4 finalColor = gbuffer.SceneColor;

    // Gamma correct
    const float gamma = 2.2;
    finalColor.rgb = pow(finalColor.rgb, 1.0 / gamma);
    
    return finalColor;
    //return float4(gbuffer.SceneDepth, gbuffer.SceneDepth, gbuffer.SceneDepth, 1.0);
    //return float4(gbuffer.ShadowMap, gbuffer.ShadowMap, gbuffer.ShadowMap, 1.0);
}