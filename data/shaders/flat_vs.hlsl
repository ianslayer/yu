cbuffer CameraConstant : register(b0)
{
	float4x4 viewMatrix;
	float4x4 projectionMatrix;
};

cbuffer TransformConstant : register(b1)
{
	float4x4 modelMatrix;
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
	float4x4 viewProjMatrix = mul(viewMatrix, projectionMatrix);
	Out.position = mul(float4(IN.position, 1), viewProjMatrix);
	Out.texcoord = IN.texcoord;
	return Out;
}

																																																																																		