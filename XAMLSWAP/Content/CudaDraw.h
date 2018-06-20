#pragma once

#include "..\Common\DeviceResources.h"
#include "ShaderStructures.h"
#include "..\Common\StepTimer.h"
#include "swapChainPanel.h"
#include "XAMLSWAPMain.h"
#include "DirectXPage.xaml.h"
#include <stdlib.h>
#include "TextRenderer.h"

namespace XAMLSWAP
{
	// This sample renderer instantiates a basic rendering pipeline.
	class CudaDraw : public Renderer
	{
	public:

		struct DrawSpec {
			int width;
			int height;
			int passWidth;
			int passHeight;
		};

		CudaDraw(const std::shared_ptr<DX::DeviceResources>& deviceResources, SCPanel<CudaDraw>* panel, int width, int height);
		virtual void CreateDeviceDependentResources();
		virtual void CreateWindowSizeDependentResources();
		virtual void ReleaseDeviceDependentResources();
		virtual void Update(DX::StepTimer const& timer);
		virtual void Render();
		void ZoomUpdate(float delta, float xPos, float yPos);
		void ParameterZoomUpdate(float delta, float xPos, float yPos);
		void PanningUpdate(float xDelta, float yDelta);
		void ParameterPanningUpdate(float xDelta, float yDelta);
		void SetSwapChainPanel(SCPanel<CudaDraw>* scpanel);
		void Draw(DrawSpec spec);
		void DrawIteration();
		void CudaDraw::DrawPass();


	private:
		struct VERTEX{
			float X, Y, Z, U, V;    // vertex position
		};

		void UpdateRenderTargetSize();
		// Cached pointer to device resources.
		std::shared_ptr<DX::DeviceResources>				m_deviceResources;

		// Direct3D resources
		Microsoft::WRL::ComPtr<ID3D11InputLayout>			m_inputLayout;
		Microsoft::WRL::ComPtr<ID3D11Buffer>				m_vertexBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer>				m_indexBuffer;
		uint32												m_indexCount;

		Microsoft::WRL::ComPtr<ID3D11PixelShader>			m_pixelShader;
		
		Microsoft::WRL::ComPtr<ID3D11VertexShader>			m_vertexShader;
		Microsoft::WRL::ComPtr<ID3D11Buffer>				m_CBTexOffsetZoom;
		XAMLSWAP::CBOffsetZoom								m_CBDTexOffsetZoom;

		Microsoft::WRL::ComPtr<ID3D11Buffer>				m_CBParamOffsetZoom;
		XAMLSWAP::CBOffsetZoomDouble						m_CBDParamOffsetZoom;
		
		Microsoft::WRL::ComPtr<ID3D11ComputeShader>			m_CSSampler;
		Microsoft::WRL::ComPtr<ID3D11Buffer>				m_CBSamplerSpec;
		XAMLSWAP::CBSamplerSpec								m_CBDSamplerSpec;
		
		Microsoft::WRL::ComPtr<ID3D11ComputeShader>			m_CSFunction;
		Microsoft::WRL::ComPtr<ID3D11Buffer>				m_CBComputePass;
		XAMLSWAP::CBComputePassSpec							m_CBDComputePass;

		Microsoft::WRL::ComPtr<ID3D11SamplerState>			m_samplerState;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>	m_nullTextureView;
		Microsoft::WRL::ComPtr<ID3D11Texture2D>				m_nullTexture;

		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>	m_boundSamplerTextureView;
		Microsoft::WRL::ComPtr<ID3D11Texture2D>				m_boundSamplerTexture;
		Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView>	m_UAVSamplerTexture;

		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>	m_boundComputeTextureView;
		Microsoft::WRL::ComPtr<ID3D11Texture2D>				m_boundComputeTexture;
		Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView>	m_UAVComputeTexture;

		Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView>	m_UAVPerlin;

		Microsoft::WRL::ComPtr<IDXGISwapChain3>				m_swapChain;
		Microsoft::WRL::ComPtr<ID3D11RenderTargetView1>		m_d3dRenderTargetView;
		D3D11_VIEWPORT										m_screenViewport;

		float*												DummyBuffer;
		// Cached reference to the XAML panel.
		SCPanel<CudaDraw>*									m_SCPanel;
		Windows::Foundation::Size							m_d3dRenderTargetSize;
		Windows::Foundation::Size							m_outputSize;
		Windows::Foundation::Size							m_logicalSize;

		// Variables that take into account whether the app supports high resolution screens or not.
		float				m_effectiveDpi;
		float				m_effectiveCompositionScaleX;
		float				m_effectiveCompositionScaleY;

		// Variables used with the rendering loop.
		bool				m_loadingComplete;

		bool m_paramCompUpdate = true;
		bool m_texCompUpdate = true;

		//Drawing variables
		DrawSpec			m_drawSpec;
		int					m_groupWidth;
		int					m_groupHeight;
		int					m_width;
		int					m_height;
		int					m_xStride;
		int					m_yStride;
		int					m_passWidth;
		int					m_passHeight;
		int					m_Factors[2][10];
		int					m_NumFactors[2];
		bool				m_firstPass;
		bool				m_drawing = false;
		bool				m_drawUpdate = false;

		bool m_controlRectOver = false;
		bool m_controlRectPressed = false;
		
		Windows::UI::Xaml::Controls::ComboBox^			comboBox;
		Windows::UI::Xaml::Controls::TextBox^			bufferWidthField;
		Windows::UI::Xaml::Controls::TextBox^			bufferHeightField;
		Windows::UI::Xaml::Controls::TextBox^			blockWidthField;
		Windows::UI::Xaml::Controls::TextBox^			blockHeightField;
		Windows::UI::Xaml::Controls::TextBlock^			m_iterationField;

		std::mutex							m_paramCompMutex;
		std::mutex							m_texCompMutex;
		std::mutex							m_renderMutex;
		std::mutex							m_drawMutex;

		TextRenderer						m_textRenderer;
	};
}

