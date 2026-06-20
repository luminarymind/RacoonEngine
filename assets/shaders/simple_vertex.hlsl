cbuffer cbPerObject : register(b0)
{
    float4x4 gMVP;
}

void VS(float3 inPos : POSITION,
        float4 inColor : COLOR,
        out float4 outPos : SV_POSITION,
        out float4 outColor : COLOR)
{
    outPos = mul(float4(inPos, 1.0f), gMVP);
    outColor = inColor;
}