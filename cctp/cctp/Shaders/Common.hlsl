#ifndef COMMON
#define COMMON
#include "Octahedral.hlsl"

#ifndef PI
#define PI 3.14159274
#endif // PI

struct RayPayload
{
    float3 HitIrradiance;
    float HitDistance;
};

// The max number of probes in the probe field
#define MAX_PROBE_COUNT 350
// The number of rays traced from a probe. McGuire uses up to 256 rays
#define PROBE_RAY_COUNT 32
// The amount of texels in a square side to use to store a probes irradiance data in
#define IRRADIANCE_PROBE_SIDE_LENGTH 8 
// The amount of texels in a square side to use to store a probes visibility data in
#define VISIBILITY_PROBE_SIDE_LENGTH 16 
// Border size in pixels around each probe's data pack. Should be at least 1 to protect data from blurring with next probe
#define PROBE_PADDING 1
// The maximum distance a ray can travel
#define MAX_DISTANCE 1.0

// Irradiance texture dimension
#define IRRADIANCE_TEXTURE_WIDTH 4300.0
#define IRRADIANCE_TEXTURE_HEIGHT 16.0

// Visibility texture dimension
#define VISIBILITY_TEXTURE_WIDTH 7000.0
#define VISIBILITY_TEXTURE_HEIGHT 32.0

// The number of blur iterations to perform on each output texture
#define IRRADIANCE_BLUR_ITERATIONS 2
#define VISIBILITY_BLUR_ITERATIONS 0

#define SHADOW_BIAS 0.04

float2 GetProbeTopLeftPosition(uint probeIndex, float singleProbeSideLength, uint padding)
{
    return float2(
                    (float) (padding * probeIndex) + ((float) probeIndex * singleProbeSideLength),
                    0.0
                 );
}

float2 GetProbeTexelCoordinate(float3 direction, uint probeIndex, float singleProbeSideLength, uint padding)
{
    // Encode the direction to oct texture coordinate in [0, 1] range
    float2 normalizedOctCoordZeroOne = (OctEncode(direction) + 1.0) * 0.5;

    // Calculate the oct coordinate in the dimensions of the probe output texture
    float2 normalizedOctCoordTextureDimensions = (normalizedOctCoordZeroOne * singleProbeSideLength);

    // Calculate the top left texel of this probe's output in the texture
    float2 probeTopLeftPosition = GetProbeTopLeftPosition(probeIndex, singleProbeSideLength, padding);

    return probeTopLeftPosition + normalizedOctCoordTextureDimensions;
}

float3 Lighting(float3 vertexNormalWS, float3 lightVectorWS, float3 cameraVectorWS, float shadow, float lightIntensity)
{
    // Ambient
    const float3 ambient = float3(0.0, 0.0, 0.0);
    
    // Diffuse
    const float3 lightColor = float3(1.0, 1.0, 1.0);
    const float3 diffuse = lightColor * saturate(dot(vertexNormalWS, lightVectorWS));
    
    // Specular
    //const float3 H = normalize(lightVectorWS + cameraVectorWS);
    //const float NoH = saturate(dot(vertexNormalWS, H));
    //const float gloss = 256.0;
    //const float3 specColor = float3(0.4, 0.4, 0.4);
    //const float3 specular = specColor * pow(NoH, gloss);
    const float3 specular = float3(0.0, 0.0, 0.0);
    
    return ambient + (((diffuse + specular) * lightIntensity * shadow));
}

float CalculateShadow(float4 lightSpacePosition, float bias, float LoN, Texture2D<float> shadowMap, SamplerState shadowMapSampler)
{
    // Cascaded shadow maps can be implemented to reduce swimming at the edges of the map
    // Alternatively, the shadow map can be raytraced with DXR for an accurate shadow map 

    float currentDepth = lightSpacePosition.z / lightSpacePosition.w;
    
    if (currentDepth > 1.0)
        return 1.0;

    float2 projectedCoord;
    projectedCoord.x = lightSpacePosition.x / lightSpacePosition.w / 2.0 + 0.5;
    projectedCoord.y = -lightSpacePosition.y / lightSpacePosition.w / 2.0 + 0.5;
    
    [branch]
    if ((saturate(projectedCoord.x) == projectedCoord.x) && (saturate(projectedCoord.y) == projectedCoord.y))
    {
        // Percentage closer filtering for soft shadows
        const float minShadowBias = 0.001;
        float shadow = 0.0;
        float2 shadowMapDims;
        shadowMap.GetDimensions(shadowMapDims.x, shadowMapDims.y);
        float2 texelSize = 1.0 / shadowMapDims;
        float currentDepth = lightSpacePosition.z / lightSpacePosition.w;
        bias = max(bias * (1.0 - LoN), minShadowBias);
        for (int x = -1; x <= 1; ++x)
        {
            for (int y = -1; y <= 1; ++y)
            {
                float pcfDepth = shadowMap.SampleLevel(shadowMapSampler, projectedCoord + float2(x, y) * texelSize, 0).r;
                shadow += currentDepth - bias < pcfDepth ? 1.0 : 0.0;
            }
        }
        return shadow / 9.0;
    }
    else
    {
        return 1.0;
    }
}

#endif // COMMON_INCLUDE