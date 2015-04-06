cbuffer cbNeverChanges : register(b0)
{
    matrix View;
    matrix World;
    float3 v3Light;
    float Ambient;
    float Directional;
};

struct VS_INPUT {
    float4 v4Position : POSITION;
    float3 v3Normal : NORMAL;
};

struct VS_OUTPUT {
    float4 v4Position : SV_POSITION;
    float4 v4Color : COLOR;
};

VS_OUTPUT VS(VS_INPUT Input) {
    VS_OUTPUT Output;
    float3 v3WorldNormal;
    float fPlusDot;
    float fBright;

    Output.v4Position = mul(Input.v4Position, View);
    v3WorldNormal = mul(Input.v3Normal, World);
    fPlusDot = max(dot(v3WorldNormal, v3Light), 0.0);
    fBright = Directional * fPlusDot + Ambient;
    Output.v4Color = float4(fBright, fBright, fBright, 1.0);

    return Output;
}

float4 PS(VS_OUTPUT Input) : SV_TARGET{
    return Input.v4Color;
}