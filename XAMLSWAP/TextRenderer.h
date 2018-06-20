#pragma once

#include <string>
#include "..\Common\DeviceResources.h"
#include "..\Common\StepTimer.h"

struct TextBlok {
public:
	std::vector<std::wstring*> strings;
	int xPos, yPos;
	int lineHeight;
	int width;
	int height;

	TextBlok(int xPos, int yPos, int width, int height, int lineHeight) :
		xPos(xPos),
		yPos(yPos),
		width(width),
		height(height),
		lineHeight(lineHeight)
	{
		for (int i = 0; i < 5; i++) {
			std::wstring string = L"test";
			strings.push_back(&string);
		}
	}

};

// Renders the current FPS value in the bottom right corner of the screen using Direct2D and DirectWrite.
class TextRenderer
{
public:

	TextRenderer(const std::shared_ptr<DX::DeviceResources>& deviceResources);
	void CreateDeviceDependentResources();
	void ReleaseDeviceDependentResources();
	void Render();
	void addBlock(TextBlok& block) {
		m_textBlocks.push_back(block);
	}

	TextRenderer(){}
	

private:
	// Cached pointer to device resources.
	std::shared_ptr<DX::DeviceResources> m_deviceResources;
	
	// Resources related to text rendering.
	std::vector<TextBlok>							m_textBlocks;

	DWRITE_TEXT_METRICS	                            m_textMetrics;
	Microsoft::WRL::ComPtr<ID2D1SolidColorBrush>    m_whiteBrush;
	Microsoft::WRL::ComPtr<ID2D1DrawingStateBlock1> m_stateBlock;
	Microsoft::WRL::ComPtr<IDWriteTextLayout3>      m_textLayout;
	Microsoft::WRL::ComPtr<IDWriteTextFormat2>      m_textFormat;
};
