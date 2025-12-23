struct VertexIn
{
    float3 LocalSpacePosition : LOCAL_SPACE_POSITION;
    float2 UV : UV;
    float3 VertexNormal : VERTEX_NORMAL;
};

struct VertexOut
{
    float4 FinalPosition : SV_POSITION;
    float2 TextureCoordinate : TEXTURE_COORDINATE;
};

VertexOut main( VertexIn input )
{
    VertexOut output;
    output.FinalPosition = float4(input.LocalSpacePosition, 1.0);
    output.TextureCoordinate = float2(input.UV.x, -input.UV.y);
    return output;
}