Texture2D Tex2D : register(t0);

SamplerState MeshTextureSampler : register(s0)
{
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = Wrap;
    AddressV = Wrap;
};

cbuffer cbNeverChanges : register(b0)
{
    matrix View;
    float4 v4AddColor;
};

struct VS_INPUT {
    float4 v4Position : POSITION;
    float2 v2Tex : TEXTURE;
};

struct VS_OUTPUT {
    float4 v4Position : SV_POSITION;
    float4 v4Color : COLOR;
    float2 v2Tex : TEXTURE;
};

VS_OUTPUT VS(VS_INPUT Input) {
    VS_OUTPUT Output;

    Output.v4Position = mul(Input.v4Position, View);
    Output.v4Color = float4(1.0, 1.0, 1.0, 1.0);
    Output.v2Tex = Input.v2Tex;

    return Output;
}

float4 PS(VS_OUTPUT Input) : SV_TARGET {
    return Tex2D.Sample(MeshTextureSampler, Input.v2Tex) * Input.v4Color + v4AddColor;
}