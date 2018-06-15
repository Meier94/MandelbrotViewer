Texture2D shaderTexture : register(t0);
SamplerState SampleType : register(s0);

struct PixelShaderInput
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD0;
};

float4 main(PixelShaderInput input) : SV_TARGET
{
	return shaderTexture.Sample(SampleType, input.uv);
}