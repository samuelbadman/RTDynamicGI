#include "Octahedral.hlsl"
#include "Common.hlsl"

cbuffer PerFrameConstants : register(b0)
{
    float4x4 LightMatrix;
    float4 ProbePositionsWS[MAX_PROBE_COUNT];
    float4 LightDirectionWS;
    float4 packedData; // Stores probe count (x), probe spacing (y), light intensity (z)
}

struct VertexOut
{
    float4 ProjectionSpacePosition : SV_POSITION;
    float2 TextureCoordinate : TEXTURE_COORDINATE;
    float4 BaseColor : BASE_COLOR;
    float3 CameraVectorWS : CAMERA_VECTOR_WS;
    float3 NormalWS : NORMAL_WS;
    float3 LightVectorWS : LIGHT_VECTOR_WS;
    uint Lit : Lit;
    float4 LightSpacePosition : POSITION_LS;
    float3 WorldPosition : POSITION_WS;
};

SamplerState pointSampler : register(s0, space0);
SamplerState linearSampler : register(s0, space1);
Texture2D<float> shadowMap : register(t0);
Texture2D<float3> irradianceData : register(t1);
Texture2D<float2> visibilityData : register(t2);

float Square(float x)
{
    return x * x;
}

float3 Irradiance(float3 shadingPoint, float3 shadingPointNormal)
{
    shadingPointNormal = normalize(shadingPointNormal);
    float3 sumIrradiance = float3(0.0, 0.0, 0.0);
    for (int i = 0; i < (int) packedData.x; ++i)
    {
        float3 probePosition = ProbePositionsWS[i].rgb;

        float3 pointToProbe = probePosition - shadingPoint;
        float distance = length(pointToProbe);
        float3 direction = normalize(pointToProbe);

        // Sample irradiance and visibility from this probe
        float2 irradianceTexelIndex = GetProbeTexelCoordinate(direction, i, IRRADIANCE_PROBE_SIDE_LENGTH, PROBE_PADDING);
        //float2 visibilityTexelIndex = GetProbeTexelCoordinate(direction, i, VISIBILITY_PROBE_SIDE_LENGTH, PROBE_PADDING);

        float3 probeIrradiance = irradianceData.SampleLevel(linearSampler, irradianceTexelIndex / float2(IRRADIANCE_TEXTURE_WIDTH, IRRADIANCE_TEXTURE_HEIGHT), 0).rgb;
        //float2 probeVisibility = irradianceData.SampleLevel(linearSampler, visibilityTexelIndex / float2(VISIBILITY_TEXTURE_WIDTH, VISIBILITY_TEXTURE_HEIGHT), 0).rg;

        // Weight irradiance by distance and orientation to the probe
        float weight = 1.0 / max(distance, 0.001);

        float orientation = dot(shadingPointNormal, direction);
        weight *= max(0.0, orientation);

        weight *= min(distance, 1.0);

        // Sum irradiance
        sumIrradiance += weight * probeIrradiance;
    }
    return sumIrradiance;
}

float4 main(VertexOut input) : SV_TARGET
{
    const float4 baseColor = input.BaseColor;

    float4 finalColor = float4(0.0f, 0.0f, 0.0f, 1.0);

    [branch]
    if (input.Lit)
    {
        // Light and shadow the point
        float shadow = CalculateShadow(input.LightSpacePosition, SHADOW_BIAS, saturate(dot(input.LightVectorWS, input.NormalWS)), shadowMap, pointSampler);
        finalColor = float4(baseColor.xyz * Lighting(
                                                input.NormalWS,
                                                input.LightVectorWS,
                                                input.CameraVectorWS,
                                                shadow,
                                                packedData.z),
                            baseColor.a);

        // Diffuse global illumination
        const float ddgiPower = 1.0;
        finalColor.rgb = finalColor.rgb + Irradiance(input.WorldPosition, input.NormalWS) * ddgiPower;
    }
    else
    {
        finalColor = baseColor;
    }
    
    return finalColor;
}