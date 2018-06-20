#include "pch.h"
#include "TextRenderer.h"

#include "Common/DirectXHelper.h"

using namespace Microsoft::WRL;

// Initializes D2D resources used for text rendering.
TextRenderer::TextRenderer(const std::shared_ptr<DX::DeviceResources>& deviceResources) :
	m_deviceResources(deviceResources)
{
	ZeroMemory(&m_textMetrics, sizeof(DWRITE_TEXT_METRICS));

	// Create device independent resources
	ComPtr<IDWriteTextFormat> textFormat;
	DX::Try(m_deviceResources->GetDWriteFactory()->CreateTextFormat(
			L"Segoe UI",
			nullptr,
			DWRITE_FONT_WEIGHT_LIGHT,
			DWRITE_FONT_STYLE_NORMAL,
			DWRITE_FONT_STRETCH_NORMAL,
			32.0f,
			L"en-US",
			&textFormat));

	DX::Try(textFormat.As(&m_textFormat));

	DX::Try(m_textFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR));

	DX::Try(m_deviceResources->GetD2DFactory()->CreateDrawingStateBlock(&m_stateBlock));

	CreateDeviceDependentResources();
}


// Renders a frame to the screen.
void TextRenderer::Render()
{
	ID2D1DeviceContext* context = m_deviceResources->GetD2DDeviceContext();

	context->SaveDrawingState(m_stateBlock.Get());
	context->BeginDraw();

	// Position on the bottom right corner
	for (std::vector<TextBlok>::iterator b = m_textBlocks.begin(); b != m_textBlocks.end(); ++b) {
		auto textBlock = *b;
		int stringIndex = 0;
		for (std::vector<std::wstring*>::iterator s = textBlock.strings.begin(); s != textBlock.strings.end(); ++s) {
			stringIndex++;
			//auto string = *s;
			auto string = std::wstring(L"test");
			ComPtr<IDWriteTextLayout> textLayout;
			DX::Try(m_deviceResources->GetDWriteFactory()->CreateTextLayout(string.c_str(), (uint32)string.length(), m_textFormat.Get(),
				240.0f, // Max width of the input text.
				50.0f, // Max height of the input text.
				&textLayout));

			DX::Try(textLayout.As(&m_textLayout));

			DX::Try(m_textLayout->GetMetrics(&m_textMetrics));

			D2D1::Matrix3x2F screenTranslation = D2D1::Matrix3x2F::Translation(
				textBlock.xPos,
				textBlock.yPos + textBlock.lineHeight * stringIndex
			);

			context->SetTransform(screenTranslation);

			DX::Try(m_textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING));

			context->DrawTextLayout(
				D2D1::Point2F(0.f, 0.f),
				m_textLayout.Get(),
				m_whiteBrush.Get()
			);
		}
	}

	// Ignore D2DERR_RECREATE_TARGET here. This error indicates that the device
	// is lost. It will be handled during the next call to Present.
	HRESULT hr = context->EndDraw();
	if (hr != D2DERR_RECREATE_TARGET){
		DX::Try(hr);
	}

	context->RestoreDrawingState(m_stateBlock.Get());
}

void TextRenderer::CreateDeviceDependentResources()
{
	DX::Try(m_deviceResources->GetD2DDeviceContext()->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &m_whiteBrush));
}
void TextRenderer::ReleaseDeviceDependentResources(){
	m_whiteBrush.Reset();
}