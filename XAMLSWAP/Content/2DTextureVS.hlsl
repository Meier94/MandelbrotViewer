struct VertexShaderInput
{
	float3 pos : POSITION;
	float2 uv : TEXCOORD0;
};

cbuffer CompSpec : register(b0)
{
	float xOffset;
	float yOffset;
	float zoom;
	float pad;
};

struct PixelShaderInput
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD0;
};

PixelShaderInput main(VertexShaderInput input)
{
	PixelShaderInput output;
	output.uv = input.uv;

	//convert the 0,1 texture notation to -1 to +1 coordinate notation with z=0 height and w=1
	output.position = float4(2*(input.pos.x*zoom+xOffset) - 1, 2*(input.pos.y*zoom+yOffset) - 1, 0, 1);

	return output;
}