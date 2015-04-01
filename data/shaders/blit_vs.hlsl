cbuffer CameraConstant : register(b0)
{
	float4x4 viewMatrix;
	float4x4 projectionMatrix;
};

struct VertexInput 
{
	float3 position : POSITION;
	float2 texcoord : TEXCOORD0;
	float4 color : COLOR;
};
struct VS_OUTPUT 
{
	float4 position : SV_POSITION; 
	float2 texcoord : TEXCOORD1;
};

VS_OUTPUT main(VertexInput IN) 
{
	VS_OUTPUT Out;
	Out.position = float4(IN.position, 1);
	Out.texcoord = IN.texcoord;
	return Out;
}

		