#include "Common.hlsl"

cbuffer PerObjectConstants : register(b0)
{
    float4x4 WorldMatrix;
    float4 Color;
    float4x4 NormalMatrix;
    uint Lit;
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

struct VertexIn
{
    float3 LocalSpacePosition : LOCAL_SPACE_POSITION;
    float2 UV : UV;
    float3 VertexNormal : VERTEX_NORMAL;
};

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

VertexOut main(VertexIn input)
{
    float4 worldSpacePosition = mul(WorldMatrix, float4(input.LocalSpacePosition, 1.0f));
    float4 viewSpacePosition = mul(ViewMatrix, worldSpacePosition);

    VertexOut output;
    output.ProjectionSpacePosition = mul(ProjectionMatrix, viewSpacePosition);
    output.TextureCoordinate = input.UV;
    output.NormalWS = normalize(mul(NormalMatrix, float4(input.VertexNormal, 0.0f)).xyz);
    output.LightVectorWS = -normalize(LightDirectionWS.xyz);
    output.CameraVectorWS = normalize(CameraPositionWS.xyz - worldSpacePosition.xyz);
    output.BaseColor = Color;
    output.Lit = Lit;
    output.LightSpacePosition = mul(LightMatrix, worldSpacePosition);
    output.WorldPosition = worldSpacePosition.xyz;
    return output;
}