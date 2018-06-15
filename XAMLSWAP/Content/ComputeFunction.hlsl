

//x*x threads per block
//x1*x2	pass size (this * 16*16 will be the lowest sample accuracy)
//x3^2	number of passes (must be power of 2)

cbuffer ComputeAreaSpec : register(b0)
{
	uint2	Offset;
	uint2	stride;
	float2	f_Offset;
	float2	f_Stride;
	float	width;
	float	height;
	float	pad1;
	float	pad2;
};

cbuffer ParamSpec : register(b1)
{
	double2 pOffset;
	double pZoom;
};


RWTexture2D<float4> BufferOut : register(u0);
RWStructuredBuffer<int> p : register(u1);
float lerp(float t, float a, float b) {
	return a + t*(b - a);
}

float grad(int hash, float x, float y, float z) {
	int h = hash & 15;
	float u = h<8 ? x : y, v = h<4 ? y : h == 12 || h == 15 ? x : z;
	return((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
}

float fade(float t) {
	return t * t * t * (t * (t * 6 - 15) + 10);
}

float noise(float x, float y, float z) {
	int X = (int)floor(x) & 255;                 // FIND UNIT CUBE THAT
	int Y = (int)floor(y) & 255;                 // CONTAINS POdouble.
	int Z = (int)floor(z) & 255;
	x -= floor(x);                               // FIND RELATIVE X,Y,Z
	y -= floor(y);                               // OF POdouble IN CUBE.
	z -= floor(z);
	float u = fade(x), v = fade(y), w = fade(z);     // COMPUTE FADE CURVES




	int A = p[X] + Y, AA = p[A] + Z, AB = p[A + 1] + Z,
		B = p[X + 1] + Y, BA = p[B] + Z, BB = p[B + 1] + Z;

	return (lerp(w, lerp(v, lerp(u, grad(p[AA], x, y, z),           // AND ADD
		grad(p[BA], x - 1, y, z)),          // BLENDED
		lerp(u, grad(p[AB], x, y - 1, z),           // RESULTS
			grad(p[BB], x - 1, y - 1, z))),         // FROM  8
		lerp(v, lerp(u, grad(p[AA + 1], x, y, z - 1),           // CORNERS
			grad(p[BA + 1], x - 1, y, z - 1)),          // OF CUBE
			lerp(u, grad(p[AB + 1], x, y - 1, z - 1),
				grad(p[BB + 1], x - 1, y - 1, z - 1)))));
}


float4 pinkShit(float fx, float fy) {
	float z = 0;
	float nf = 0;
	float4 color;
	for (int i = 0; i<8; i++) { //amplitude(7)=0.996*255, 
		float amplitude = pow(0.5f, i + 1);
		float frekvens = 5.0f*pow(2.0f, i + 1);
		nf += ((amplitude)*(noise(fx *frekvens, fy *frekvens, z*frekvens)));
	}
	nf = 0.5f * nf + 0.5f;
	float step = 0.012;
	if (fmod(nf, step) < (0.95*step)) {
		color.r = fmod(0.13f + 13.0f * nf, 1.0f);
		color.g = 0;
		color.b = fmod(0.04f + 13.0f * nf, 1.0f);
	}
	else if (fmod(nf, step) < (0.87*step)) {
		color.r = 0;// 133 + 12000 * (fmod(nf, step) - 0.75*step);
		color.g = fmod(0.2f + 47.0f * (fmod(nf, step) - 0.75f*step), 1.0f);
		color.b = 0;
	}
	else {
		color.r = 0;// 133 + 12000 * (step - fmod(nf, step));
		color.g = fmod(0.2f + 47.0f * (step - fmod(nf, step)), 1.0f);
		color.b = 0;
	}
	color.a = 1.0f;
	return color;
}

float4 planet(float fx, float fy) {
	float z = 0;
	float nf = 0;
	float4 color;
	float r = sqrt((0.5f - fx)*(0.5f - fx) + (0.5f - fy)*(0.5f - fy));
	if (r < 0.4) {
		z = sqrt(0.09 - (r*r));
		if (r > 0.3f)
			z = 0;
		for (int i = 0; i < 8; i++) { //amplitude(7)=0.996*255, 
			float amplitude = pow(0.5f, i + 1);
			float frekvens = 5.0f*pow(2.0f, i + 1);
			nf += ((amplitude)*(noise(fx *frekvens, fy *frekvens, z*frekvens)));
		}
		nf = 0.5f * nf + 0.5f;
	}
	if (r < 0.3){
		color.r = 0.8f * (1.0f + sin((0.5f + (0.1*fx) + nf*2.0f*3.14f))) * 0.5f;
		color.g = 0;
		color.b = 0;
	}
	else if (r<0.4){
	color.r = ((0.4f - r) / 0.1f) * 0.8f * (1.0f + sin((0.5f + (0.1*fx) + nf*2.0f*3.14f)))*0.5f;
	color.g = 0;
	color.b = 0;
	}
	else {
		color.r = 0;
		color.g = 0;
		color.b = 0;
	}
	color.a = 1.0f;
	return color;
}

float2 c_mul(float2 c1, float2 c2)
{
	float a = c1.x;
	float b = c1.y;
	float c = c2.x;
	float d = c2.y;
	return float2(a*c - b*d, b*c + a*d);
}

float4 mandelbrot(int iter, double2 pos) {
	double2 z = double2(0,0);
	int i = -1;
	double zrsqr = z.x * z.x;
	double zisqr = z.y * z.y;
	while (zrsqr + zisqr <= 4.0 && i < iter)
	{
		z.y = (z.x + z.y);
		z.y *= z.y;
		z.y += - zrsqr - zisqr;
		z.y += pos.y;
		z.x = zrsqr - zisqr + pos.x;
		zrsqr = z.x * z.x;
		zisqr = z.y * z.y;
		i++;
	}
	/*return float4(
		float(i) / iter,
		float(i) / iter,
		float(i) / iter,
		1.0f);*/
	
	float step = 1.0f;
	return float4(
		-cos(i*0.10f * step)*0.5f + 0.5f,
		-cos(i*0.20f * step)*0.5f + 0.5f,
		-cos(i*0.30f * step)*0.5f + 0.5f,
		1.0f);
}

[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	uint2 idx = Offset + stride*DTid.xy;
	float fx = (float)idx.x / width;
	float fy = (float)idx.y / height;
	double2 pos = double2(fx, fy);

	float4 color = mandelbrot(7000, pos*pZoom + pOffset);
	//float4 color = planet(fx, fy);

	BufferOut[idx] = color;
}


