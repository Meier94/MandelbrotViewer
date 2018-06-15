#include "pch.h"
#include "swapChainPanel.h"

using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Core;
using namespace Platform;


SCPanelCommon::SCPanelCommon(XAMLSWAP::DirectXPage^ page, Canvas ^Canvas, int width, int height, int posx, int posy) :
	m_xpos(posx),
	m_ypos(posy),
	m_page(page),
	m_width(width),
	m_height(height),
	m_logicalSize(Windows::Foundation::Size(0.0f, 0.0f)),
	m_rectWidth(300),
	m_rectHeight(300)
{
	m_canvas = Canvas;
	m_canvas->Width = width + 330;
	m_canvas->Height = height + 20;

	m_panel = ref new SwapChainPanel();
	m_panel->Width = width;
	m_panel->Height = height;
	Canvas::SetLeft(m_panel, posx);
	Canvas::SetTop(m_panel, posy);

	Canvas->Children->Append(m_panel);
	m_panel->PointerEntered += ref new PointerEventHandler(
		[this](Object^ sender, PointerRoutedEventArgs^ e) { m_panelOver = true; });
	m_panel->PointerExited += ref new PointerEventHandler(
		[this](Object^ sender, PointerRoutedEventArgs^ e) { m_panelOver = false; });
	m_panel->PointerPressed += ref new PointerEventHandler(
		[this](Object^ sender, PointerRoutedEventArgs^ e) { m_panelPressed = true; });

	m_panel->SizeChanged += ref new SizeChangedEventHandler(
		m_page, &XAMLSWAP::DirectXPage::OnSwapChainPanelSizeChanged);
	m_panel->CompositionScaleChanged += ref new Windows::Foundation::TypedEventHandler
		<SwapChainPanel^, Object^>(m_page, &XAMLSWAP::DirectXPage::OnCompositionScaleChanged);

	m_surface = ref new Shapes::Rectangle();
	m_surface->Width = m_rectWidth;
	m_surface->Height = m_rectHeight;
	m_surface->Fill = ref new Media::SolidColorBrush(Windows::UI::ColorHelper::FromArgb(255, 250, 250, 250));
	m_surface->Stroke = ref new Media::SolidColorBrush(Windows::UI::ColorHelper::FromArgb(255, 0, 0, 0));
	Place(m_surface, 10, height - m_rectHeight);

	m_surface->PointerEntered += ref new PointerEventHandler([this](Object^ sender, PointerRoutedEventArgs^ e) {m_surfaceOver = true; OutputDebugString(L"Over\n"); });
	m_surface->PointerExited += ref new PointerEventHandler([this](Object^ sender, PointerRoutedEventArgs^ e) {m_surfaceOver = false; OutputDebugString(L"Not over\n"); });
	m_surface->PointerPressed += ref new PointerEventHandler([this](Object^ sender, PointerRoutedEventArgs^ e) {m_surfacePressed = true; OutputDebugString(L"Press\n"); });


}

void SCPanelCommon::Place(UIElement^ element, int xOffset, int yOffset) {
	Canvas::SetLeft(element, m_xpos + xOffset + m_width);
	Canvas::SetTop(element, m_ypos + yOffset);
	m_canvas->Children->Append(element);
}

void SCPanelCommon::MouseWheelCallback(float delta, PointerRoutedEventArgs^ e) {
	if (m_surfaceOver) {
		float xPos = e->GetCurrentPoint((UIElement^)m_surface)->Position.X;
		float yPos = e->GetCurrentPoint((UIElement^)m_surface)->Position.Y;
		m_surfaceScrlCallback(delta, xPos, yPos);
	}
	else if (m_panelOver) {
		float xPos = e->GetCurrentPoint((UIElement^)m_panel)->Position.X;
		float yPos = e->GetCurrentPoint((UIElement^)m_panel)->Position.Y;
		m_panelScrlCallback(delta, xPos, yPos);
	}
}

void SCPanelCommon::Write(int textField, const wchar_t* text) {
	m_page->Dispatcher->RunAsync(CoreDispatcherPriority::Normal, ref new DispatchedHandler([=]() {
			m_textFields.at(textField)->Text = ref new Platform::String(text);
		}, CallbackContext::Any));

}