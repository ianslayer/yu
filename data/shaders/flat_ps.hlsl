struct PS_INPUT
{
	float4 Position : SV_POSITION;
};

struct PS_OUTPUT {
	float4 color: SV_TARGET0;
};

PS_OUTPUT main(PS_INPUT IN)
{
	PS_OUTPUT Out;
	Out.color = float4(1, 0, 0, 1);
	return Out;
}
