struct PS_INPUT
{
	float4 position : SV_POSITION;
	float2 texcoord : TEXCOORD1;
};

struct PS_OUTPUT {
	float4 color: SV_TARGET0;
};

Texture2D colorTexture : register(t0);
SamplerState pointSampler : register(s0);

PS_OUTPUT main(PS_INPUT IN)
{
	PS_OUTPUT Out;
	Out.color = colorTexture.Sample(pointSampler, IN.texcoord);
	return Out;
}
