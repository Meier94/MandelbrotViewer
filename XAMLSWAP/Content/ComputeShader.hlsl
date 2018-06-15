//--------------------------------------------------------------------------------------
// File: BasicCompute11.hlsl
//
// This file contains the Compute Shader to perform array A + array B
// 
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
/*
struct BufType
{
	int i;
	float f;
#ifdef TEST_DOUBLE
	double d;
#endif    
};*/

cbuffer SamplerSpec : register(b0)
{
	uint2	strides;
	uint2	pad;
};

Texture2D Buffer0 : register(t0);
RWTexture2D<float4> BufferOut : register(u0);

[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	BufferOut[DTid.xy] = Buffer0[DTid.xy- DTid.xy%strides];
}


