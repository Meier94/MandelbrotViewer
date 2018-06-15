#include "pch.h"
#include "XAMLSWAPMain.h"
#include "Common\DirectXHelper.h"
#include <windowsx.h>
#include <consoleapi.h>

using namespace XAMLSWAP;
using namespace Windows::Foundation;
using namespace Windows::System::Threading;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Input;
using namespace Platform;
using namespace Concurrency;

// Loads and initializes application assets when the application is loaded.
XAMLSWAPMain::XAMLSWAPMain(XAMLSWAP::DirectXPage^ page, const std::shared_ptr<DX::DeviceResources>& deviceResources) :
	m_deviceResources(deviceResources)
{
	// Register to be notified if the Device is lost or recreated
	m_deviceResources->RegisterDeviceNotify(this);

	/*AllocConsole();
	AttachConsole(GetCurrentProcessId());
	freopen_s(&consoleFD, "CON", "w", stdout);*/

	//Panel1
	SCPanel<CudaDraw>* Panel = new SCPanel<CudaDraw>(page, page->GetCanvas(), 1920*2/3, 1080*2/3, 10, 10);
	page->addPanel((int)Panel);
	auto renderer = new CudaDraw(m_deviceResources, Panel, 1920*2/3, 1080*2/3);

	//Panel2
	/*SCPanel^ Panel2 = ref new SCPanel(page, page->GetCanvas(), 800, 600, 100, 800);
	page->addPanel(Panel2);
	auto renderer2 = new CudaDraw(m_deviceResources, Panel2, 120, 120);
	Panel2->RegisterMouseWheelCallback((int)renderer2, (int)&CudaDraw::ZoomUpdateStatic);
	Panel2->RegisterPointerMovedCallback((int)renderer2, (int)&CudaDraw::PanningUpdateStatic);
	*/
	m_renderers.push_back(renderer);
	//m_renderers.push_back(renderer2);
	StartRenderLoop();
	// TODO: Replace this with your app's content initialization.
	//m_sceneRenderer = std::unique_ptr<CudaDraw>(new CudaDraw(m_deviceResources));

	//m_fpsTextRenderer = std::unique_ptr<SampleFpsTextRenderer>(new SampleFpsTextRenderer(m_deviceResources));

	// TODO: Change the timer settings if you want something other than the default variable timestep mode.
	// e.g. for 60 FPS fixed timestep update logic, call:
	/*
	m_timer.SetFixedTimeStep(true);
	m_timer.SetTargetElapsedSeconds(1.0 / 60);
	*/
}

XAMLSWAPMain::~XAMLSWAPMain()
{
	// Deregister device notification
	m_deviceResources->RegisterDeviceNotify(nullptr);
}

// Updates application state when the window size changes (e.g. device orientation change)
void XAMLSWAPMain::CreateWindowSizeDependentResources() 
{
	// TODO: Replace this with the size-dependent initialization of your app's content.
	for (std::vector<Renderer*>::iterator r = m_renderers.begin(); r != m_renderers.end(); ++r) {
		auto renderer = *r;
		renderer->CreateWindowSizeDependentResources();
	}
}

void XAMLSWAPMain::StartRenderLoop()
{
	// If the animation render loop is already running then do not start another thread.
	if (m_renderLoopWorker != nullptr && m_renderLoopWorker->Status == AsyncStatus::Started)
	{
		return;
	}

	// Create a task that will be run on a background thread.
	auto workItemHandler = ref new WorkItemHandler([this](IAsyncAction ^ action)
	{
		// Calculate the updated frame and render once per vertical blanking interval.
		while (action->Status == AsyncStatus::Started)
		{
			critical_section::scoped_lock lock(m_criticalSection);
			Update();
			Render();
		}
	});

	// Run task on a dedicated high priority background thread.
	m_renderLoopWorker = ThreadPool::RunAsync(workItemHandler, WorkItemPriority::High, WorkItemOptions::TimeSliced);
}

void XAMLSWAPMain::StopRenderLoop()
{
	m_renderLoopWorker->Cancel();
}

// Updates the application state once per frame.
void XAMLSWAPMain::Update() 
{
	ProcessInput();

	// Update scene objects.
	m_timer.Tick([&]()
	{
		for (std::vector<Renderer*>::iterator r = m_renderers.begin(); r != m_renderers.end(); ++r) {
			auto renderer = *r;
			renderer->Update(m_timer);
		}
	});
}

// Process all input from the user before updating game state
void XAMLSWAPMain::ProcessInput()
{

}

// Renders the current frame according to the current application state.
// Returns true if the frame was rendered and is ready to be displayed.
bool XAMLSWAPMain::Render() 
{
	// Don't try to render anything before the first Update.
	if (m_timer.GetFrameCount() == 0)
	{
		return false;
	}

	// Render the scene objects.
	// TODO: Replace this with your app's content rendering functions.
	for (std::vector<Renderer*>::iterator r = m_renderers.begin(); r != m_renderers.end(); ++r) {
		auto renderer = *r;
		renderer->Render();
	}
	//m_fpsTextRenderer->Render();

	return true;
}

// Notifies renderers that device resources need to be released.
void XAMLSWAPMain::OnDeviceLost()
{
	for (std::vector<Renderer*>::iterator r = m_renderers.begin(); r != m_renderers.end(); ++r) {
		auto renderer = *r;
		renderer->ReleaseDeviceDependentResources();
	}
}

// Notifies renderers that device resources may now be recreated.
void XAMLSWAPMain::OnDeviceRestored()
{
	for (std::vector<Renderer*>::iterator r = m_renderers.begin(); r != m_renderers.end(); ++r) {
		auto renderer = *r;
		renderer->CreateDeviceDependentResources();
	}
	CreateWindowSizeDependentResources();
}
