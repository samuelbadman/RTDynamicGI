#ifndef OCTAHEDRAL
#define OCTAHEDRAL

float2 SignNotZero(float2 v)
{
    return float2((v.x >= 0.0) ? 1.0 : -1.0, (v.y >= 0.0) ? 1.0 : -1.0);
}

// Majercik et al. https://jcgt.org/published/0008/02/01/
/** Assumes that v is a unit vector. The result is an octahedral vector on the [-1, +1] square. */
float2 OctEncode(in float3 v)
{
    float l1norm = abs(v.x) + abs(v.y) + abs(v.z);
    float2 result = v.xy * (1.0 / l1norm);
    if (v.z < 0.0)
    {
        result = (1.0 - abs(result.yx)) * SignNotZero(result.xy);
    }
    return result;
}

// Majercik et al. https://jcgt.org/published/0008/02/01/
/** Returns a unit vector. Argument o is an octahedral vector packed via octEncode,
    on the [-1, +1] square*/
float3 OctDecode(float2 o)
{
    float3 v = float3(o.x, o.y, 1.0 - abs(o.x) - abs(o.y));
    if (v.z < 0.0)
    {
        v.xy = (1.0 - abs(v.yx)) * SignNotZero(v.xy);
    }
    return normalize(v);
}

#endif