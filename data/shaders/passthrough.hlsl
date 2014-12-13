struct VertexInput
{
	float3 position : POSITION;
	float4 color : COLOR;
};
struct VS_OUTPUT
{
	float4 Position : SV_POSITION;
};

VS_OUTPUT VS(VertexInput IN)
{
	VS_OUTPUT Out;
	Out.Position = float4(IN.position, 1);
	return Out;
}

struct PS_OUTPUT {
	float4 color: SV_TARGET0;
};

PS_OUTPUT PS(VS_OUTPUT IN)
{
	PS_OUTPUT Out;
	Out.color = float4(1, 0, 0, 1);
	return Out;
}
