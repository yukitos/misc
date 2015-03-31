cbuffer cbNeverChanges : register(b0)
{
    matrix View;
};

struct VS_INPUT {
    float4 v4Position : POSITION;
    float4 v4Color : COLOR;
};

struct VS_OUTPUT {
    float4 v4Position : SV_POSITION;
    float4 v4Color : COLOR;
};

VS_OUTPUT VS(VS_INPUT Input)
{
    VS_OUTPUT Output;

    Output.v4Position = mul(Input.v4Position, View);
    Output.v4Color = Input.v4Color;

    return Output;
}

float4 PS(VS_OUTPUT Input) : SV_TARGET
{
    return Input.v4Color;
}
