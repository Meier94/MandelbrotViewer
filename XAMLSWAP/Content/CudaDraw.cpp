#include "pch.h"
#include "CudaDraw.h"
#include <stdio.h>
#include <windows.ui.xaml.media.dxinterop.h>

#include "..\Common\DirectXHelper.h"

using namespace XAMLSWAP;

using namespace DirectX;
using namespace Windows::Foundation;

using namespace D2D1;
using namespace Microsoft::WRL;
using namespace Windows::Foundation;
using namespace Windows::Graphics::Display;
using namespace Windows::UI::Core;
using namespace Windows::UI::Xaml::Controls;
using namespace Platform;



namespace DisplayMetrics
{
	static const bool SupportHighResolutions = false; //Don't turn on, swapchain does not allow for no scaling on composition?? Didn't bother looking into it.

	static const float DpiThreshold = 192.0f;		// 200% of standard desktop display.
	static const float WidthThreshold = 1920.0f;	// 1080p width.
	static const float HeightThreshold = 1080.0f;	// 1080p height.
};

// Loads vertex and pixel shaders from files and instantiates the cube geometry.
CudaDraw::CudaDraw(const std::shared_ptr<DX::DeviceResources>& deviceResources, SCPanel<CudaDraw>* panel, int width, int height) :
	m_loadingComplete(false),
	m_indexCount(0),
	m_deviceResources(deviceResources),
	m_width(width),
	m_height(height),
	m_groupWidth(8),
	m_groupHeight(8),
	m_CBDTexOffsetZoom({0.0f, 0.0f, 1.0f, 0.0f}),
	//m_CBDParamOffsetZoom({ -0.75112049050233554, -0.025753832194144760, 1.2e-9, 0.0 }),
	m_CBDParamOffsetZoom({ -2.5, -2, 4, 0.0 }),
	m_drawSpec({ width, height , 16 , 18 })
{
	SetSwapChainPanel(panel);

	m_SCPanel->PanelScrlCallback([this](float delta, float xpos, float ypos) { ParameterZoomUpdate(delta, xpos, ypos); });
	m_SCPanel->PanelDragCallback([this](float dx, float dy) { ParameterPanningUpdate(dx, dy); });
	m_SCPanel->SurfaceScrlCallback([this](float delta, float xpos, float ypos) { ZoomUpdate(delta, xpos, ypos); });
	m_SCPanel->SurfaceDragCallback([this](float dx, float dy) { PanningUpdate(dx, dy); });

	/*
	//CS selector
	comboBox = ref new Windows::UI::Xaml::Controls::ComboBox();
	comboBox->Width = 300;
	comboBox->PlaceholderText = "Select Compute Shader";
	m_SCPanel->Place(comboBox, 10, 0);

	//Widthfield
	bufferWidthField = ref new Windows::UI::Xaml::Controls::TextBox();
	bufferWidthField->PlaceholderText = "Image Width";
	bufferWidthField->Width = 300;
	m_SCPanel->Place(bufferWidthField, 10, 40);

	//Heightfield
	bufferHeightField = ref new Windows::UI::Xaml::Controls::TextBox();
	bufferHeightField->PlaceholderText = "Image Height";
	bufferHeightField->Width = 300;
	m_SCPanel->Place(bufferHeightField, 10, 80);

	//BlockWidth
	blockWidthField = ref new Windows::UI::Xaml::Controls::TextBox();
	blockWidthField->PlaceholderText = "Block Width";
	blockWidthField->Width = 300;
	m_SCPanel->Place(blockWidthField, 10, 120);

	//BlockHeight
	blockHeightField = ref new Windows::UI::Xaml::Controls::TextBox();
	blockHeightField->PlaceholderText = "Block Height";
	blockHeightField->Width = 300;
	m_SCPanel->Place(blockHeightField, 10, 160);

	//TextField
	m_iterationField = m_SCPanel->AddTextField(10, 200);
	*/


	CreateDeviceDependentResources();
	CreateWindowSizeDependentResources();
}

// This method is called when the XAML control is created (or re-created).
void CudaDraw::SetSwapChainPanel(SCPanel<CudaDraw>* scpanel)
{
	DisplayInformation^ currentDisplayInformation = DisplayInformation::GetForCurrentView();
	SwapChainPanel^ panel = scpanel->GetPanel();
	m_SCPanel = scpanel;
	m_logicalSize = m_SCPanel->GetLogicalSize();
}

// Determine the dimensions of the render target and whether it will be scaled down.
void CudaDraw::UpdateRenderTargetSize()
{
	m_effectiveDpi = DisplayInformation::GetForCurrentView()->LogicalDpi;
	m_effectiveCompositionScaleX = m_SCPanel->GetPanel()->CompositionScaleX;
	m_effectiveCompositionScaleY = m_SCPanel->GetPanel()->CompositionScaleY;

	m_logicalSize = m_SCPanel->GetLogicalSize();

	// To improve battery life on high resolution devices, render to a smaller render target
	// and allow the GPU to scale the output when it is presented.
	if (!DisplayMetrics::SupportHighResolutions && m_effectiveDpi > DisplayMetrics::DpiThreshold)
	{
		float width = DX::ConvertDipsToPixels(m_logicalSize.Width, m_effectiveDpi);
		float height = DX::ConvertDipsToPixels(m_logicalSize.Height, m_effectiveDpi);

		// When the device is in portrait orientation, height > width. Compare the
		// larger dimension against the width threshold and the smaller dimension
		// against the height threshold.
		if (max(width, height) > DisplayMetrics::WidthThreshold && min(width, height) > DisplayMetrics::HeightThreshold)
		{
			// To scale the app we change the effective DPI. Logical size does not change.
			m_effectiveDpi /= 2.0f;
			m_effectiveCompositionScaleX /= 2.0f;
			m_effectiveCompositionScaleY /= 2.0f;
		}
	}

	// Calculate the necessary render target size in pixels.
	m_outputSize.Width = DX::ConvertDipsToPixels(m_logicalSize.Width, m_effectiveDpi);
	m_outputSize.Height = DX::ConvertDipsToPixels(m_logicalSize.Height, m_effectiveDpi);

	// Prevent zero size DirectX content from being created.
	m_outputSize.Width = max(m_outputSize.Width, 1);
	m_outputSize.Height = max(m_outputSize.Height, 1);
}

// Initializes view parameters when the window size changes.
void CudaDraw::CreateWindowSizeDependentResources()
{
	//---------------------------------------------------------------
	// Clear the previous window size specific context.
	ID3D11DeviceContext3* devCon = m_deviceResources->GetD3DDeviceContext();
	ID3D11RenderTargetView* nullViews[] = { nullptr };
	devCon->OMSetRenderTargets(ARRAYSIZE(nullViews), nullViews, nullptr);
	m_d3dRenderTargetView = nullptr;
	//m_d2dTargetBitmap = nullptr;
	devCon->Flush1(D3D11_CONTEXT_TYPE_ALL, nullptr);

	UpdateRenderTargetSize();

	m_d3dRenderTargetSize.Width = m_outputSize.Width;
	m_d3dRenderTargetSize.Height = m_outputSize.Height;

	if (m_swapChain != nullptr){
		// If the swap chain already exists, resize it.
		HRESULT hr = m_swapChain->ResizeBuffers(2, lround(m_d3dRenderTargetSize.Width), lround(m_d3dRenderTargetSize.Height), DXGI_FORMAT_B8G8R8A8_UNORM, 0);

		if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET){
			// If the device was removed for any reason, a new device and swap chain will need to be created.
			m_deviceResources->HandleDeviceLost();
			// Everything is set up now. Do not continue execution of this method. HandleDeviceLost will reenter this method 
			// and correctly set up the new device.
			return;
		}
		else
			DX::Try(hr);
	}
	else
	{
		// Otherwise, create a new one using the same adapter as the existing Direct3D device.
		DXGI_SCALING scaling = DisplayMetrics::SupportHighResolutions ? DXGI_SCALING_NONE : DXGI_SCALING_STRETCH;
		DXGI_SWAP_CHAIN_DESC1 swapChainDesc = { 0 };

		swapChainDesc.Width = lround(m_d3dRenderTargetSize.Width);		// Match the size of the window.
		swapChainDesc.Height = lround(m_d3dRenderTargetSize.Height);
		swapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;				// This is the most common swap chain format.
		swapChainDesc.Stereo = false;
		swapChainDesc.SampleDesc.Count = 1;								// Don't use multi-sampling.
		swapChainDesc.SampleDesc.Quality = 0;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.BufferCount = 2;									// Use double-buffering to minimize latency.
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;	// All Windows Store apps must use _FLIP_ SwapEffects.
		swapChainDesc.Flags = 0;
		swapChainDesc.Scaling = scaling;
		swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;

		// This sequence obtains the DXGI factory that was used to create the Direct3D device above.
		ComPtr<IDXGIDevice3> dxgiDevice;
		DX::Try(m_deviceResources->GetD3DDeviceCom().As(&dxgiDevice));

		ComPtr<IDXGIAdapter> dxgiAdapter;
		DX::Try(dxgiDevice->GetAdapter(&dxgiAdapter));

		ComPtr<IDXGIFactory4> dxgiFactory;
		DX::Try(dxgiAdapter->GetParent(IID_PPV_ARGS(&dxgiFactory)));

		// When using XAML interop, the swap chain must be created for composition.
		ComPtr<IDXGISwapChain1> swapChain;
		DX::Try(dxgiFactory->CreateSwapChainForComposition(m_deviceResources->GetD3DDevice(), &swapChainDesc, nullptr, &swapChain));

		DX::Try(swapChain.As(&m_swapChain));

		// Associate swap chain with SwapChainPanel
		// UI changes will need to be dispatched back to the UI thread
		m_SCPanel->GetPanel()->Dispatcher->RunAsync(CoreDispatcherPriority::High, ref new DispatchedHandler([=](){
			// Get backing native interface for SwapChainPanel
			ComPtr<ISwapChainPanelNative> panelNative;
			DX::Try(reinterpret_cast<IUnknown*>(m_SCPanel->GetPanel())->QueryInterface(IID_PPV_ARGS(&panelNative)));

			DX::Try(panelNative->SetSwapChain(m_swapChain.Get()));
		}, CallbackContext::Any));

		// Ensure that DXGI does not queue more than one frame at a time. This both reduces latency and
		// ensures that the application will only render after each VSync, minimizing power consumption.
		DX::Try(dxgiDevice->SetMaximumFrameLatency(1));
	}

	
	// Setup inverse scale on the swap chain
	DXGI_MATRIX_3X2_F inverseScale = { 0 };
	inverseScale._11 = 1.0f / m_effectiveCompositionScaleX;
	inverseScale._22 = 1.0f / m_effectiveCompositionScaleY;
	ComPtr<IDXGISwapChain2> spSwapChain2;
	DX::Try(m_swapChain.As<IDXGISwapChain2>(&spSwapChain2));

	DX::Try(spSwapChain2->SetMatrixTransform(&inverseScale));

	// Create a render target view of the swap chain back buffer.
	ComPtr<ID3D11Texture2D1> backBuffer;
	DX::Try(m_swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer)));

	DX::Try(m_deviceResources->GetD3DDevice()->CreateRenderTargetView1(backBuffer.Get(), nullptr, &m_d3dRenderTargetView));

	// Set the 3D rendering viewport to target the entire window.
	m_screenViewport = CD3D11_VIEWPORT(0.0f, 0.0f, m_d3dRenderTargetSize.Width, m_d3dRenderTargetSize.Height);

	devCon->RSSetViewports(1, &m_screenViewport);

}

// Called once per frame, rotates the cube and calculates the model and view matrices.
void CudaDraw::Update(DX::StepTimer const& timer)
{
	if (!m_loadingComplete) {
		return;
	}
	auto context = m_deviceResources->GetD3DDeviceContext();
	m_paramCompMutex.lock();
	if (m_paramCompUpdate) {
		context->UpdateSubresource1(m_CBParamOffsetZoom.Get(), 0, NULL, &m_CBDParamOffsetZoom, 0, 0, 0);
		m_paramCompUpdate = false;
		Draw(m_drawSpec);
	}
	m_paramCompMutex.unlock();

	if (m_drawing)
		DrawIteration();

}


// Renders one frame using the vertex and pixel shaders.
void CudaDraw::Render()
{
	m_renderMutex.lock();
	// Loading is asynchronous. Only draw geometry after it's loaded.
	if (!m_loadingComplete){
		m_renderMutex.unlock();
		return;
	}

	auto context = m_deviceResources->GetD3DDeviceContext();
	//Setup and run compute shader
	UINT initCounts = 0;

	ID3D11ShaderResourceView *resourceViews;
	if (m_drawUpdate) {
		
		context->CSSetShader(m_CSSampler.Get(), nullptr, 0);
		context->CSSetConstantBuffers1(0, 1, m_CBSamplerSpec.GetAddressOf(), nullptr, nullptr);
		ID3D11UnorderedAccessView* uavs = m_UAVSamplerTexture.Get();
		context->CSSetUnorderedAccessViews(0, 1, &uavs, &initCounts);
		resourceViews = m_boundComputeTextureView.Get();
		context->CSSetShaderResources(0, 1, &resourceViews);

		context->Dispatch(m_width / 8, m_height / 8, 1);
		uavs = nullptr;
		resourceViews = nullptr;
		context->CSSetShaderResources(0, 1, &resourceViews);
		context->CSSetUnorderedAccessViews(0, 1, &uavs, &initCounts);
	}

	//Setup rendertarget
	context->RSSetViewports(1, &m_screenViewport);
	ID3D11RenderTargetView *const targets[1] = { m_d3dRenderTargetView.Get() };
	context->OMSetRenderTargets(1, targets, nullptr);

	context->ClearRenderTargetView(m_d3dRenderTargetView.Get(), DirectX::Colors::CornflowerBlue);

	// Prepare and set shader resources
	m_texCompMutex.lock();
	if (m_texCompUpdate) {
		context->UpdateSubresource1(m_CBTexOffsetZoom.Get(), 0, NULL, &m_CBDTexOffsetZoom, 0, 0, 0);
		m_texCompUpdate = false;
	}
	m_texCompMutex.unlock();

	context->VSSetConstantBuffers1(0, 1, m_CBTexOffsetZoom.GetAddressOf(), nullptr, nullptr);

	ID3D11SamplerState *sampler = m_samplerState.Get();
	context->PSSetSamplers(0, 1, &sampler);

	resourceViews = m_boundSamplerTextureView.Get();
	context->PSSetShaderResources(0, 1, &resourceViews);
	context->PSSetShader(m_pixelShader.Get(), nullptr, 0);
	context->VSSetShader(m_vertexShader.Get(), nullptr, 0);

	// Each vertex is one instance of the VertexPositionColor struct.
	UINT stride = sizeof(VERTEX);
	UINT offset = 0;
	context->IASetVertexBuffers(0, 1, m_vertexBuffer.GetAddressOf(), &stride, &offset);
	context->IASetIndexBuffer(m_indexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	context->IASetInputLayout(m_inputLayout.Get());

	// Draw the objects.
	context->DrawIndexed(m_indexCount, 0, 0);

	//Release the sampler texture to aquisition by the sampler CS
	resourceViews = nullptr;
	context->PSSetShaderResources(0, 1, &resourceViews);
	
	DXGI_PRESENT_PARAMETERS parameters = { 0 };
	HRESULT hr = m_swapChain->Present1(1, 0, &parameters);
	DX::Try(hr);
	m_renderMutex.unlock();
}

void CudaDraw::CreateDeviceDependentResources()
{
	// Load shaders asynchronously.
	auto loadVSTask = DX::ReadDataAsync(L"2DTextureVS.cso");
	auto loadPSTask = DX::ReadDataAsync(L"2DTexturePS.cso");
	auto loadCSTask = DX::ReadDataAsync(L"ComputeShader.cso");
	auto loadCFTask = DX::ReadDataAsync(L"ComputeFunction.cso");

	// After the vertex shader file is loaded, create the shader and input layout.
	auto createVSTask = loadVSTask.then([this](const std::vector<byte>& fileData) {
		auto dev = m_deviceResources->GetD3DDevice();
		DX::Try(dev->CreateVertexShader(&fileData[0], fileData.size(), nullptr, &m_vertexShader));

		static const D3D11_INPUT_ELEMENT_DESC vertexDesc [] = {
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT   , 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }, };

		DX::Try(dev->CreateInputLayout(vertexDesc, ARRAYSIZE(vertexDesc), &fileData[0], fileData.size(), &m_inputLayout));
	});

	// After the pixel shader file is loaded, create the shader and constant buffer.
	auto createPSTask = loadPSTask.then([this](const std::vector<byte>& fileData) {
		auto dev = m_deviceResources->GetD3DDevice();
		DX::Try(dev->CreatePixelShader(&fileData[0], fileData.size(), nullptr, &m_pixelShader));

		CD3D11_BUFFER_DESC constantBufferDesc(sizeof(CBOffsetZoom) , D3D11_BIND_CONSTANT_BUFFER);
		D3D11_SUBRESOURCE_DATA constantBufferData = { 0 };
		constantBufferData.pSysMem = &m_CBDTexOffsetZoom;
		DX::Try(dev->CreateBuffer(&constantBufferDesc, &constantBufferData, &m_CBTexOffsetZoom));
	});

	auto createCSTask = loadCSTask.then([this](const std::vector<byte>& fileData) {
		auto dev = m_deviceResources->GetD3DDevice();
		DX::Try(dev->CreateComputeShader(&fileData[0], fileData.size(), nullptr, &m_CSSampler));

		D3D11_TEXTURE2D_DESC desc;
		desc.Width = m_width;
		desc.Height = m_height;
		desc.MipLevels = 1;
		desc.ArraySize = 1;
		desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;;
		desc.CPUAccessFlags = 0;
		desc.MiscFlags = 0;
		HRESULT hr = m_deviceResources->GetD3DDevice()->CreateTexture2D(&desc, nullptr, &m_boundSamplerTexture);


		D3D11_UNORDERED_ACCESS_VIEW_DESC UAVDesc;
		ZeroMemory(&UAVDesc, sizeof(UAVDesc));
		UAVDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		UAVDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
		UAVDesc.Texture2D.MipSlice = 0;
		DX::Try(dev->CreateUnorderedAccessView(m_boundSamplerTexture.Get(),&UAVDesc, &m_UAVSamplerTexture));


		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
		ZeroMemory(&srvDesc, sizeof(srvDesc));
		srvDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;
		DX::Try(dev->CreateShaderResourceView(m_boundSamplerTexture.Get(), &srvDesc, &m_boundSamplerTextureView));

		CD3D11_BUFFER_DESC constantBufferDesc(sizeof(CBSamplerSpec), D3D11_BIND_CONSTANT_BUFFER);
		m_CBDSamplerSpec.strides[0] = 1;
		m_CBDSamplerSpec.strides[1] = 1;
		m_CBDSamplerSpec.pad[0] = 1;
		m_CBDSamplerSpec.pad[1] = 1;
		D3D11_SUBRESOURCE_DATA constantBufferData = { 0 };
		constantBufferData.pSysMem = &m_CBDSamplerSpec;
		DX::Try(dev->CreateBuffer(&constantBufferDesc, &constantBufferData, &m_CBSamplerSpec));
	
	});

	auto createCFTask = loadCFTask.then([this](const std::vector<byte>& fileData) {
		auto dev = m_deviceResources->GetD3DDevice();
		DX::Try(dev->CreateComputeShader(&fileData[0], fileData.size(), nullptr, &m_CSFunction));

		D3D11_TEXTURE2D_DESC desc;
		desc.Width = m_width;
		desc.Height = m_height;
		desc.MipLevels = 1;
		desc.ArraySize = 1;
		desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;;
		desc.CPUAccessFlags = 0;
		desc.MiscFlags = 0;
		HRESULT hr = m_deviceResources->GetD3DDevice()->CreateTexture2D(&desc, nullptr, &m_boundComputeTexture);


		D3D11_UNORDERED_ACCESS_VIEW_DESC UAVDesc;
		ZeroMemory(&UAVDesc, sizeof(UAVDesc));
		UAVDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		UAVDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
		UAVDesc.Texture2D.MipSlice = 0;
		DX::Try(dev->CreateUnorderedAccessView(m_boundComputeTexture.Get(), &UAVDesc, &m_UAVComputeTexture));


		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
		ZeroMemory(&srvDesc, sizeof(srvDesc));
		srvDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;
		DX::Try(dev->CreateShaderResourceView(m_boundComputeTexture.Get(), &srvDesc, &m_boundComputeTextureView));


		CD3D11_BUFFER_DESC constantBufferDesc(sizeof(CBComputePassSpec), D3D11_BIND_CONSTANT_BUFFER);
		DX::Try(dev->CreateBuffer(&constantBufferDesc, nullptr, &m_CBComputePass));
		constantBufferDesc.ByteWidth = sizeof(CBOffsetZoomDouble);
		D3D11_SUBRESOURCE_DATA constantBufferData = { 0 };
		constantBufferData.pSysMem = &m_CBDParamOffsetZoom;
		DX::Try(dev->CreateBuffer(&constantBufferDesc, &constantBufferData, &m_CBParamOffsetZoom));
	});

	// Once both shaders are loaded, create the mesh.
	auto createCubeTask = (createPSTask && createVSTask).then([this] () {
		auto dev = m_deviceResources->GetD3DDevice();
		VERTEX OurVertices[] ={
		 	{ 0.0f, 1.0f, 0.1f, 0.0f, 1.0f }, { 1.0f, 1.0f, 0.1f, 1.0f, 1.0f },
			{ 0.0f, 0.0f, 0.1f, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.1f, 1.0f, 0.0f }};

		D3D11_SUBRESOURCE_DATA vertexBufferData = {0};
		vertexBufferData.pSysMem = OurVertices;
		CD3D11_BUFFER_DESC vertexBufferDesc(sizeof(OurVertices), D3D11_BIND_VERTEX_BUFFER);
		DX::Try(dev->CreateBuffer(&vertexBufferDesc, &vertexBufferData, &m_vertexBuffer));

		static const unsigned short OurIndices[] = {0,1,2,2,1,3};

		m_indexCount = ARRAYSIZE(OurIndices);
		D3D11_SUBRESOURCE_DATA indexBufferData = {0};
		indexBufferData.pSysMem = OurIndices;
		CD3D11_BUFFER_DESC indexBufferDesc(sizeof(OurIndices), D3D11_BIND_INDEX_BUFFER);
		DX::Try(dev->CreateBuffer(&indexBufferDesc, &indexBufferData, &m_indexBuffer));

		
		D3D11_SAMPLER_DESC samplerDesc;
		//samplerDesc.Filter = D3D11_FILTER_ANISOTROPIC;
		samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
		samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.MipLODBias = 0.0f;
		samplerDesc.MaxAnisotropy = 2;// featureLevel <= D3D_FEATURE_LEVEL_9_1 ? 2 : 4;
		samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
		samplerDesc.BorderColor[0] = 0.0f;
		samplerDesc.BorderColor[1] = 0.0f;
		samplerDesc.BorderColor[2] = 0.0f;
		samplerDesc.BorderColor[3] = 0.0f;
		samplerDesc.MinLOD = 0;
		samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
		dev->CreateSamplerState(&samplerDesc, &m_samplerState);


		D3D11_TEXTURE2D_DESC desc;
		desc.Width = m_width;
		desc.Height = m_height;
		desc.MipLevels = 0;
		desc.ArraySize = 1;
		desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		desc.CPUAccessFlags = 0;
		desc.MiscFlags = 0;
		HRESULT hr = dev->CreateTexture2D(&desc, nullptr, &m_nullTexture);

		
		DummyBuffer = new float[m_width*m_height*4];
		ZeroMemory(DummyBuffer, sizeof(float)*4*m_width*m_height);
		for (int x = 0; x < m_width; x++) {
			for (int y = 0; y < m_height; y++) {
				float* color = &DummyBuffer[y*m_width * 4 + x * 4];
				float xf = float(x) / m_width;
				float yf = float(y) / m_height;
				if ((x + y) % 2) {
					color[0] = xf;
					color[1] = yf;
					color[2] = 1.0f - xf;
					color[3] = 1.0f;
				}
				else {
					color[0] = 1.0f;
					color[1] = 1.0f;
					color[2] = 1.0f;
					color[3] = 1.0f;
				}
			}
		}

		m_deviceResources->GetD3DDeviceContext()->UpdateSubresource(m_nullTexture.Get(), 0, nullptr, DummyBuffer, m_width * 4 * sizeof(float), 0);
		dev->CreateShaderResourceView(m_nullTexture.Get(), nullptr, &m_nullTextureView);


		int permutation[] = { 151,160,137,91,90,15,131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,8,99,37,240,21,10,23,190, 6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,35,11,32,57,177,33,88,237,149,56,87,174,20,125,136,171,168, 68,175,74,165,71,134,139,48,27,166,77,146,158,231,83,111,229,122,60,211,133,230,220,105,92,41,55,46,245,40,244,102,143,54, 65,25,63,161, 1,216,80,73,209,76,132,187,208, 89,18,169,200,196,135,130,116,188,159,86,164,100,109,198,173,186, 3,64,52,217,226,250,124,123,5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,189,28,42,223,183,170,213,119,248,152, 2,44,154,163, 70,221,153,101,155,167, 43,172,9,129,22,39,253, 19,98,108,110,79,113,224,232,178,185, 112,104,218,246,97,228,251,34,242,193,238,210,144,12,191,179,162,241, 81,51,145,235,249,14,239,107,49,192,214, 31,181,199,106,157,184, 84,204,176,115,121,50,45,127, 4,150,254,138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180 };
		int p[512];
		for (int i = 0; i < 256; i++) {
			p[256 + i] = p[i] = permutation[i];
		}
		ID3D11Buffer *pStructuredBuffer;
		// Create Structured Buffer
		D3D11_BUFFER_DESC sbDesc;
		sbDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS |
			D3D11_BIND_SHADER_RESOURCE;
		sbDesc.CPUAccessFlags = 0;
		sbDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
		sbDesc.StructureByteStride = sizeof(int);
		sbDesc.ByteWidth = sizeof(int) * 512;
		sbDesc.Usage = D3D11_USAGE_DEFAULT;
		D3D11_SUBRESOURCE_DATA perlinDataDesc = { 0 };
		perlinDataDesc.pSysMem = p;

		dev->CreateBuffer(&sbDesc, &perlinDataDesc, &pStructuredBuffer);

		D3D11_UNORDERED_ACCESS_VIEW_DESC UAVDesc;

		ZeroMemory(&UAVDesc, sizeof(UAVDesc));
		UAVDesc.Buffer.NumElements = 512;
		UAVDesc.Format = DXGI_FORMAT_UNKNOWN;
		UAVDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
		UAVDesc.Texture2D.MipSlice = 0;
		DX::Try(dev->CreateUnorderedAccessView(pStructuredBuffer, &UAVDesc, &m_UAVPerlin));
		
	});



	// Once the cube is loaded, the object is ready to be rendered.
	(createCubeTask && createCSTask && createCFTask).then([this] () {
		m_loadingComplete = true;
		Draw(m_drawSpec);
	});

}


void CudaDraw::ReleaseDeviceDependentResources()
{
	m_loadingComplete = false;
	m_vertexShader.Reset();
	m_inputLayout.Reset();
	m_pixelShader.Reset();
	m_CBTexOffsetZoom.Reset();
	m_vertexBuffer.Reset();
	m_indexBuffer.Reset();
}