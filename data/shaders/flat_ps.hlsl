struct PS_INPUT
{
	float4 Position : SV_POSITION;
};

struct PS_OUTPUT {
	float4 color: SV_TARGET0;
};

Texture2D colorTexture : register(t0);
SamplerState pointSampler : register(s0);

PS_OUTPUT main(PS_INPUT IN)
{
	PS_OUTPUT Out;
	Out.color = colorTexture.Sample(pointSampler, float2(0.2, 0.1));
	return Out;
}
