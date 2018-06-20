#pragma once

#include "XAMLSWAPMain.h"
#include "DirectXPage.xaml.h"

class SCPanelCommon {
public:
	SCPanelCommon(XAMLSWAP::DirectXPage^ page, Windows::UI::Xaml::Controls::Canvas ^Canvas, int width, int height, int posx, int posy, int rectWidth, int rectHeight);

	Windows::Foundation::Size						GetLogicalSize() { return m_logicalSize; }
	Windows::UI::Xaml::Controls::Canvas^			GetCanvas() { return m_canvas; }
	Windows::UI::Xaml::Controls::SwapChainPanel^	GetPanel() { return m_panel; }

	void SetLogicalSize(Windows::Foundation::Size size) { m_logicalSize = size; }

	void Place(Windows::UI::Xaml::UIElement^ element, int xOffset, int yOffset);

	int		GetWidth() { return m_width; }
	int		Getheight() { return m_height; }
	int		GetRectWidth() { return m_rectWidth; }
	int		GetRectHeight() { return m_rectHeight; }
	bool	MouseOver() { return m_panelOver || m_surfaceOver; }
	bool	Pressed() { return m_panelPressed || m_surfacePressed; }

	void OnPointerReleased() { m_panelPressed = false; m_surfacePressed = false; }

	void PanelScrlCallback(std::function<void(float, float, float)> func) { m_panelScrlCallback = func; }
	void PanelDragCallback(std::function<void(float, float)> func) { m_panelDragCallback = func; }
	void SurfaceScrlCallback(std::function<void(float, float, float)> func) { m_surfaceScrlCallback = func; }
	void SurfaceDragCallback(std::function<void(float, float)> func) { m_surfaceDragCallback = func; }

	void MouseWheelCallback(float delta, Windows::UI::Xaml::Input::PointerRoutedEventArgs^ e);

	void PointerMovedCallback(float xDelta, float yDelta) {
		if (m_surfacePressed)
			m_surfaceDragCallback(xDelta, yDelta);
		else if (m_panelPressed)
			m_panelDragCallback(xDelta, yDelta);
	}

	int AddTextField(int xOffset, int yOffset) {
		auto block = ref new Windows::UI::Xaml::Controls::TextBlock();
		m_textFields.push_back(block);
		Place(block, xOffset, yOffset);
		return m_textFields.size()-1;
		
	}

	void Write(int textField, const wchar_t* text);

protected:

	XAMLSWAP::DirectXPage^									m_page;
	Windows::Foundation::Size								m_logicalSize;
	Windows::UI::Xaml::Controls::Canvas^					m_canvas;
	Windows::UI::Xaml::Shapes::Rectangle^					m_surface;
	Windows::UI::Xaml::Controls::SwapChainPanel^			m_panel;
	std::vector<Windows::UI::Xaml::Controls::TextBlock^>	m_textFields;

	bool m_panelOver = false;
	bool m_panelPressed = false;
	bool m_surfaceOver = false;
	bool m_surfacePressed = false;
	int m_xpos;
	int m_ypos;
	int m_width;
	int m_height;
	int m_rectWidth;
	int m_rectHeight;

	std::function<void(float, float, float)>		m_panelScrlCallback;
	std::function<void(float, float)>				m_panelDragCallback;
	std::function<void(float, float, float)>		m_surfaceScrlCallback;
	std::function<void(float, float)>				m_surfaceDragCallback;
};

template<class T>
class SCPanel : public SCPanelCommon{
public:
	//SCPanel() = delete;


	SCPanel(XAMLSWAP::DirectXPage^ page, Windows::UI::Xaml::Controls::Canvas ^Canvas, int width, int height, int posx, int posy, int rectWidth, int rectHeight):
		SCPanelCommon(page, Canvas, width, height, posx, posy, rectWidth, rectHeight)
	{
		
		

	}

private:

};
