//
// DirectXPage.xaml.cpp
// Implementation of the DirectXPage class.
//

#include "pch.h"
#include "DirectXPage.xaml.h"

using namespace XAMLSWAP;

using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::Graphics::Display;
using namespace Windows::System::Threading;
using namespace Windows::UI::Core;
using namespace Windows::UI::Input;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Navigation;
using namespace concurrency;





DirectXPage::DirectXPage():
	m_windowVisible(true),
	m_coreInput(nullptr)
{
	InitializeComponent();

	Scroller->Width = this->Width*2;
	Scroller->Height = this->Height;
	mainCanvas->Height = this->Height;
	mainCanvas->Width = this->Width*2;


	// Register event handlers for page lifecycle.
	CoreWindow^ window = Window::Current->CoreWindow;

	window->VisibilityChanged +=
		ref new TypedEventHandler<CoreWindow^, VisibilityChangedEventArgs^>(this, &DirectXPage::OnVisibilityChanged);

	DisplayInformation^ currentDisplayInformation = DisplayInformation::GetForCurrentView();

	currentDisplayInformation->DpiChanged += ref new TypedEventHandler<DisplayInformation^, Object^>(this, &DirectXPage::OnDpiChanged);
	
	DisplayInformation::DisplayContentsInvalidated += ref new TypedEventHandler<DisplayInformation^, Object^>(this, &DirectXPage::OnDisplayContentsInvalidated);


	// At this point we have access to the device. 
	// We can create the device-dependent resources.
	m_deviceResources = std::make_shared<DX::DeviceResources>();
	//m_deviceResources->SetSwapChainPanel(Panel->GetPanel());
	

	this->PointerPressed  += ref new PointerEventHandler(this, &DirectXPage::OnPointerPressed);
	this->PointerMoved    += ref new PointerEventHandler(this, &DirectXPage::OnPointerMoved);
	this->PointerReleased += ref new PointerEventHandler(this, &DirectXPage::OnPointerReleased);
	this->PointerWheelChanged += ref new PointerEventHandler(this, &DirectXPage::OnPointerWheel);

	m_main = std::unique_ptr<XAMLSWAPMain>(new XAMLSWAPMain(this, m_deviceResources));

}

DirectXPage::~DirectXPage()
{
	// Stop rendering and processing events on destruction.
	m_main->StopRenderLoop();
	m_coreInput->Dispatcher->StopProcessEvents();
}

// Saves the current state of the app for suspend and terminate events.
void DirectXPage::SaveInternalState(IPropertySet^ state)
{
	critical_section::scoped_lock lock(m_main->GetCriticalSection());
	m_deviceResources->Trim();

	// Stop rendering when the app is suspended.
	m_main->StopRenderLoop();

	// Put code to save app state here.
}

// Loads the current state of the app for resume events.
void DirectXPage::LoadInternalState(IPropertySet^ state)
{
	// Put code to load app state here.

	// Start rendering when the app is resumed.
	m_main->StartRenderLoop();
}

// Window event handlers.

void DirectXPage::OnVisibilityChanged(CoreWindow^ sender, VisibilityChangedEventArgs^ args)
{
	m_windowVisible = args->Visible;
	if (m_windowVisible)
	{
		m_main->StartRenderLoop();
	}
	else
	{
		m_main->StopRenderLoop();
	}
}

// DisplayInformation event handlers.

void DirectXPage::OnDpiChanged(DisplayInformation^ sender, Object^ args)
{
	critical_section::scoped_lock lock(m_main->GetCriticalSection());
	// Note: The value for LogicalDpi retrieved here may not match the effective DPI of the app
	// if it is being scaled for high resolution devices. Once the DPI is set on DeviceResources,
	// you should always retrieve it using the GetDpi method.
	// See DeviceResources.cpp for more details.

	m_deviceResources->SetDpi(sender->LogicalDpi);
	m_main->CreateWindowSizeDependentResources();
}


void DirectXPage::OnDisplayContentsInvalidated(DisplayInformation^ sender, Object^ args)
{
	critical_section::scoped_lock lock(m_main->GetCriticalSection());
	m_deviceResources->ValidateDevice();
}

// Called when the app bar button is clicked.
void DirectXPage::AppBarButton_Click(Object^ sender, RoutedEventArgs^ e)
{
	// Use the app bar if it is appropriate for your app. Design the app bar, 
	// then fill in event handlers (like this one).
}
bool down = false;
static float lastMeasure[2];
void DirectXPage::OnPointerPressed(Object^ sender, PointerRoutedEventArgs^ e)
{
	this->CapturePointer(e->Pointer);
	down = true;
	lastMeasure[0] = e->GetCurrentPoint((UIElement^) nullptr)->Position.X;
	lastMeasure[1] = e->GetCurrentPoint((UIElement^) nullptr)->Position.Y;
	OutputDebugString(L"Pressed master");
}


void DirectXPage::OnPointerMoved(Object^ sender, PointerRoutedEventArgs^ e)
{
	// Update the pointer tracking code.
	if (down) {

		SCPanelCommon* pressed = nullptr;
		for (std::vector<SCPanelCommon*>::iterator p = panels.begin(); p != panels.end(); ++p) {
			auto panel = *p;
			if (panel->Pressed())
				pressed = panel;
		}

		auto x = e->GetCurrentPoint((UIElement^) nullptr)->Position.X;
		auto y = e->GetCurrentPoint((UIElement^) nullptr)->Position.Y;
		double movedX = x - lastMeasure[0];
		double movedY = y - lastMeasure[1];
		lastMeasure[0] = x;
		lastMeasure[1] = y;

		if (pressed == nullptr)
			Scroller->ChangeView(Scroller->HorizontalOffset - 4.0*movedX, Scroller->VerticalOffset - 4.0*movedY, nullptr);
		else
			pressed->PointerMovedCallback(movedX, movedY);
	}
}

void DirectXPage::OnPointerReleased(Object^ sender, PointerRoutedEventArgs^ e)
{
	this->ReleasePointerCapture(e->Pointer);
	down = false;
	for (std::vector<SCPanelCommon*>::iterator p = panels.begin(); p != panels.end(); ++p) {
		auto panel = *p;
		panel->OnPointerReleased();
	}
	OutputDebugString(L"Released master\n");
}

void DirectXPage::OnPointerWheel(Object^ sender, PointerRoutedEventArgs^ e)
{

	auto delta = e->GetCurrentPoint((UIElement^) nullptr)->Properties->MouseWheelDelta;
	//Only zooms the canvas, not the SwapChainPanel

	//Scroller->ChangeView(nullptr, Scroller->VerticalOffset - 0.5f*delta, nullptr);
	bool mouseOver = false;
	for (std::vector<SCPanelCommon*>::iterator p = panels.begin(); p != panels.end(); ++p) {
		auto panel = *p;
		if (panel->MouseOver()) {
			panel->MouseWheelCallback(delta, e);
			mouseOver = true;
			break;
		}
	}
	if (!mouseOver) {
		Scroller->ChangeView(nullptr, Scroller->VerticalOffset - 0.5f*delta, nullptr);
	}
	
}

void DirectXPage::OnCompositionScaleChanged(SwapChainPanel^ sender, Object^ args)
{
	critical_section::scoped_lock lock(m_main->GetCriticalSection());
	m_main->CreateWindowSizeDependentResources();
}

void DirectXPage::OnSwapChainPanelSizeChanged(Object^ sender, SizeChangedEventArgs^ e)
{
	critical_section::scoped_lock lock(m_main->GetCriticalSection());
	for (std::vector<SCPanelCommon*>::iterator p = panels.begin(); p != panels.end(); ++p) {
		auto panel = *p;
		panel->SetLogicalSize(e->NewSize);
	}
	m_main->CreateWindowSizeDependentResources();
}
