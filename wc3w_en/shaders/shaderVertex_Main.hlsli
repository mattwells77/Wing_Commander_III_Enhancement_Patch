#pragma pack_matrix( row_major )
cbuffer ModelViewProjectionConstantBuffer  : register(b0)
{
	float4x4 WorldMatrix;
	float4x4 WorldViewProjectionMatrix;
}



struct VS_INPUT
{
	float4 Position : POSITION;
	float2 TexCoords : TEXCOORD0;
	float3 Normal : NORMAL0;
};


struct VS_OUTPUT
{
	float4 Position : SV_Position;
	float2 TexCoords : TEXCOORD0;
	float3 Normal : NORMAL0;
	float3 WorldPos : POSITION0;
};
