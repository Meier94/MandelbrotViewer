#pragma once

#include "Common\StepTimer.h"
#include "Common\DeviceResources.h"


interface Renderer
{
	virtual void CreateDeviceDependentResources() = 0;
	virtual void CreateWindowSizeDependentResources() = 0;
	virtual void ReleaseDeviceDependentResources() = 0;
	virtual void Update(DX::StepTimer const& timer) = 0;
	virtual void Render() = 0;
};

#include "Content\CudaDraw.h"
#include "DirectXPage.xaml.h"

// Renders Direct2D and 3D content on the screen.
namespace XAMLSWAP
{
	class XAMLSWAPMain : public DX::IDeviceNotify
	{
	public:
		XAMLSWAPMain(XAMLSWAP::DirectXPage^ page, const std::shared_ptr<DX::DeviceResources>& deviceResources);
		~XAMLSWAPMain();
		void CreateWindowSizeDependentResources();
		void StartRenderLoop();
		void StopRenderLoop();
		Concurrency::critical_section& GetCriticalSection() { return m_criticalSection; }

		// IDeviceNotify
		virtual void OnDeviceLost();
		virtual void OnDeviceRestored();

	private:
		void ProcessInput();
		void Update();
		bool Render();

		std::vector<Renderer*> m_renderers;

		// Cached pointer to device resources.
		std::shared_ptr<DX::DeviceResources> m_deviceResources;

		Windows::Foundation::IAsyncAction^ m_renderLoopWorker;
		Concurrency::critical_section m_criticalSection;

		// Rendering loop timer.
		DX::StepTimer m_timer;
	};
}