cbuffer CameraConstant : register(b0)
{
	float4x4 viewMatrix;
	float4x4 projectionMatrix;
};

struct VertexInput 
{
	float3 position : POSITION;
	float4 color : COLOR;
};
struct VS_OUTPUT 
{
	float4 Position : SV_POSITION; 
};

VS_OUTPUT main(VertexInput IN) 
{
	VS_OUTPUT Out;
	float4x4 viewProjMatrix = mul(viewMatrix, projectionMatrix);
	Out.Position = mul(float4(IN.position, 1), viewProjMatrix);
	return Out;
}

																																																																																		