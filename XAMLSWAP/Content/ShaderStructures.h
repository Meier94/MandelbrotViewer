#pragma once

namespace XAMLSWAP
{
	// Constant buffer used to send MVP matrices to the vertex shader.
	struct ModelViewProjectionConstantBuffer
	{
		DirectX::XMFLOAT4X4 model;
		DirectX::XMFLOAT4X4 view;
		DirectX::XMFLOAT4X4 projection;
	};

	struct CBOffsetZoom {
		float xOffset;
		float yOffset;
		float zoom;
		float padding;
	};

	struct CBOffsetZoomDouble {
		double xOffset;
		double yOffset;
		double zoom;
		double offset;
	};

	struct CBComputePassSpec{
		uint32	Offset[2];
		uint32	stride[2];
		float	f_Offset[2];
		float	f_stride[2];
		float	width;
		float	height;
		float	pad1;
		float	pad2;
	};

	struct CBSamplerSpec{
		uint32	strides[2];
		uint32	pad[2];
	};

	// Used to send per-vertex data to the vertex shader.
	struct VertexPositionColor
	{
		DirectX::XMFLOAT3 pos;
		DirectX::XMFLOAT3 color;
	};
}