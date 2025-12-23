#include "Common.hlsl"

RaytracingAccelerationStructure SceneBVH : register(t0);
RWTexture2D<float3> irradianceOutput : register(u0);
RWTexture2D<float2> visibilityOutput : register(u1);

cbuffer PerFrameConstants : register(b0)
{
    float4x4 LightMatrix;
    float4 ProbePositionsWS[MAX_PROBE_COUNT];
    float4 LightDirectionWS;
    float4 packedData; // Stores probe count (x), probe spacing (y), light intensity (z)
};

// Majercik et al. https://jcgt.org/published/0008/02/01/
float3 SphericalFibonacci(float i, float n)
{
    const float PHI = sqrt(5) * 0.5 + 0.5;
#define madfrac(A, B) ((A)*(B)-floor((A)*(B)))
    float phi = 2.0 * PI * madfrac(i, PHI - 1);
    float cosTheta = 1.0 - (2.0 * i + 1.0) * (1.0 / n);
    float sinTheta = sqrt(saturate(1.0 - cosTheta * cosTheta));
    return float3(
        cos(phi) * sinTheta,
        sin(phi) * sinTheta,
        cosTheta);
#undef madfrac
}

void BlurIrradianceOutput(const float2 probeTopLeft)
{
    for (int y = probeTopLeft.y; y < probeTopLeft.y + IRRADIANCE_PROBE_SIDE_LENGTH; ++y)
    {
        for (int x = probeTopLeft.x; x < probeTopLeft.x + IRRADIANCE_PROBE_SIDE_LENGTH; ++x)
        {
            [branch]
            if (x < probeTopLeft.x || x > probeTopLeft.x + IRRADIANCE_PROBE_SIDE_LENGTH ||
                        y < probeTopLeft.y || y > probeTopLeft.y + IRRADIANCE_PROBE_SIDE_LENGTH)
            {
                continue;
            }
            else
            {
                float2 top = float2(x, y) + float2(0, 1);
                float2 right = float2(x, y) + float2(1, 0);
                float2 left = float2(x, y) + float2(-1, 0);
                float2 bottom = float2(x, y) + float2(0, -1);
                float3 newValue = irradianceOutput[float2(x, y)];
                newValue = lerp(newValue, irradianceOutput[top], 0.5f);
                newValue = lerp(newValue, irradianceOutput[left], 0.5f);
                newValue = lerp(newValue, irradianceOutput[bottom], 0.5f);
                newValue = lerp(newValue, irradianceOutput[right], 0.5f);
                irradianceOutput[float2(x, y)] = newValue;
            }
        }
    }
}

void BlurVisibilityOutput(const float2 probeTopLeft)
{
    for (int y = probeTopLeft.y; y < probeTopLeft.y + VISIBILITY_PROBE_SIDE_LENGTH; ++y)
    {
        for (int x = probeTopLeft.x; x < probeTopLeft.x + VISIBILITY_PROBE_SIDE_LENGTH; ++x)
        {
            [branch]
            if (x < probeTopLeft.x || x > probeTopLeft.x + VISIBILITY_PROBE_SIDE_LENGTH ||
                        y < probeTopLeft.y || y > probeTopLeft.y + VISIBILITY_PROBE_SIDE_LENGTH)
            {
                continue;
            }
            else
            {
                float2 top = float2(x, y) + float2(0, 1);
                float2 right = float2(x, y) + float2(1, 0);
                float2 left = float2(x, y) + float2(-1, 0);
                float2 bottom = float2(x, y) + float2(0, -1);
                float2 newValue = visibilityOutput[float2(x, y)].rg;
                newValue = lerp(newValue, visibilityOutput[top], 0.5f);
                newValue = lerp(newValue, visibilityOutput[left], 0.5f);
                newValue = lerp(newValue, visibilityOutput[bottom], 0.5f);
                newValue = lerp(newValue, visibilityOutput[right], 0.5f);
                visibilityOutput[float2(x, y)] = newValue;
            }
        }
    }
}

[shader("raygeneration")]
void RayGen()
{    
    // Shoot rays from each probe
    for (int p = 0; p < (int) packedData.x; ++p)
    {
        for (int r = 0; r < PROBE_RAY_COUNT; ++r)
        {
            float3 dir = normalize(SphericalFibonacci((float) r, (float) PROBE_RAY_COUNT));

            RayDesc ray;
            ray.Origin = ProbePositionsWS[p].xyz;
            ray.Direction = dir;
            ray.TMin = 0.0;
            ray.TMax = MAX_DISTANCE;

            RayPayload payload =
            {
                float3(0.0, 0.0, 0.0),
                0.0
            };
            TraceRay(SceneBVH, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, 0xff, 0, 0, 0, ray, payload);
            
            // Store irradiance for probe
            irradianceOutput[GetProbeTexelCoordinate(dir, p, IRRADIANCE_PROBE_SIDE_LENGTH, PROBE_PADDING)].rgb = payload.HitIrradiance;

            // Store visibility for probe as distance and square distance
            float2 visibilityTexel = GetProbeTexelCoordinate(dir, p, VISIBILITY_PROBE_SIDE_LENGTH, PROBE_PADDING);
            visibilityOutput[visibilityTexel].r = payload.HitDistance;
            visibilityOutput[visibilityTexel].g = payload.HitDistance * payload.HitDistance;
        }
        
        // Blur irradiance output
        for (int i = 0; i < IRRADIANCE_BLUR_ITERATIONS; ++i)
            BlurIrradianceOutput(GetProbeTopLeftPosition(p, IRRADIANCE_PROBE_SIDE_LENGTH, PROBE_PADDING));
        
        // Blur visibility output
        for (int v = 0; v < VISIBILITY_BLUR_ITERATIONS; ++v)
            BlurVisibilityOutput(GetProbeTopLeftPosition(p, VISIBILITY_PROBE_SIDE_LENGTH, PROBE_PADDING));
    }
}