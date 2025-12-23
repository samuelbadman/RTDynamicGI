#include "Common.hlsl"

#define MAX_MATERIALS 7

struct Vertex1Pos1UV1Norm
{
    float3 Position;
    float2 UV;
    float3 Normal;
};

cbuffer MaterialBuffer : register(b0)
{
    float4 Colors[MAX_MATERIALS];
}

cbuffer PerFrameConstants : register(b1)
{
    float4x4 LightMatrix;
    float4 ProbePositionsWS[MAX_PROBE_COUNT];
    float4 LightDirectionWS;
    float4 packedData; // Stores probe count (x), probe spacing (y), light intensity (z)
}

cbuffer PerPassConstants : register(b2)
{
    float4x4 ViewMatrix;
    float4x4 ProjectionMatrix;
    float4 CameraPositionWS;
}

SamplerState pointSampler : register(s0);
Texture2D<float> shadowMap : register(t0);
StructuredBuffer<Vertex1Pos1UV1Norm> cubeVertices : register(t1);

// https://stackoverflow.com/questions/983999/simple-3x3-matrix-inverse-code-c
float3x3 inverse(float3x3 m)
{
    // computes the inverse of matrix m
    float det = m[0][0] * (m[1][1] * m[2][2] - m[2][1] * m[1][2]) -
             m[0][1] * (m[1][0] * m[2][2] - m[1][2] * m[2][0]) +
             m[0][2] * (m[1][0] * m[2][1] - m[1][1] * m[2][0]);

    float invdet = 1 / det;

    float3x3 minv; // inverse of matrix m
    minv[0][0] = (m[1][1] * m[2][2] - m[2][1] * m[1][2]) * invdet;
    minv[0][1] = (m[0][2] * m[2][1] - m[0][1] * m[2][2]) * invdet;
    minv[0][2] = (m[0][1] * m[1][2] - m[0][2] * m[1][1]) * invdet;
    minv[1][0] = (m[1][2] * m[2][0] - m[1][0] * m[2][2]) * invdet;
    minv[1][1] = (m[0][0] * m[2][2] - m[0][2] * m[2][0]) * invdet;
    minv[1][2] = (m[1][0] * m[0][2] - m[0][0] * m[1][2]) * invdet;
    minv[2][0] = (m[1][0] * m[2][1] - m[2][0] * m[1][1]) * invdet;
    minv[2][1] = (m[2][0] * m[0][1] - m[0][0] * m[2][1]) * invdet;
    minv[2][2] = (m[0][0] * m[1][1] - m[1][0] * m[0][1]) * invdet;
    
    return minv;
}

[shader("closesthit")]
void ClosestHit(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attribs)
{
    // Sample color of hit object
    uint hitInstanceID = InstanceID();
    
    // Calculate hit point in world space
    float3 hitPointWS = WorldRayOrigin() + (WorldRayDirection() * RayTCurrent());
    
    // Push the sampled point towards the camera a little way into open space
    float3 hitToCamera = normalize(CameraPositionWS.xyz - hitPointWS);
    float3 shadingPointWS = hitPointWS + (hitToCamera * 0.2f);
    
    // Get the base triangle index in the geometry object
    uint triangleIndex = PrimitiveIndex();

    Vertex1Pos1UV1Norm v0 = cubeVertices[triangleIndex];
    Vertex1Pos1UV1Norm v1 = cubeVertices[triangleIndex + 1];
    Vertex1Pos1UV1Norm v2 = cubeVertices[triangleIndex + 2];
    
    // Interpolate normal attribute
    float3x3 normalMatrix = inverse(transpose((float3x3) ObjectToWorld3x4())); // This is necessary as world matrix contains non uniform scaling
                                                                               // TODO Should be done on CPU and uploaded to GPU
    float3 normalWS = -(
        attribs.barycentrics.x * mul(normalMatrix, v0.Normal.xyz) +
        attribs.barycentrics.y * mul(normalMatrix, v1.Normal.xyz) +
        attribs.barycentrics.g * mul(normalMatrix, v2.Normal.xyz)
    );

    float3 lightVectorWS = -normalize(LightDirectionWS.xyz);
    float3 cameraVectorWS = normalize(CameraPositionWS.xyz - shadingPointWS);
    
    float shadow = CalculateShadow(mul(LightMatrix, float4(shadingPointWS, 1.0)), SHADOW_BIAS, saturate(dot(lightVectorWS, normalWS)), shadowMap, pointSampler);
    float3 lighting = Lighting(
        normalWS,
        lightVectorWS,
        cameraVectorWS,
        shadow,
        packedData.z
    );
    
    // Compute radiance power
    payload.HitIrradiance = Colors[hitInstanceID].xyz * lighting; // TODO Sample probe field
    payload.HitDistance = RayTCurrent();
}