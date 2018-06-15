#pragma once

namespace DX
{
	// Provides an interface for an application that owns DeviceResources to be notified of the device being lost or created.
	interface IDeviceNotify
	{
		virtual void OnDeviceLost() = 0;
		virtual void OnDeviceRestored() = 0;
	};

	// Controls all the DirectX device resources.
	class DeviceResources
	{
	public:
		DeviceResources();
		void ValidateDevice();
		void HandleDeviceLost();
		void RegisterDeviceNotify(IDeviceNotify* deviceNotify);
		void SetDpi(float dpi);
		void SetCompositionScale(float compositionScaleX, float compositionScaleY);
		void Trim();

		float						GetDpi() const							{ return m_dpi; }

		// D3D Accessors.
		ID3D11Device3*				GetD3DDevice() const					{ return m_d3dDevice.Get(); }
		Microsoft::WRL::ComPtr<ID3D11Device3> GetD3DDeviceCom()				{ return m_d3dDevice; }

		ID3D11DeviceContext3*		GetD3DDeviceContext() const				{ return m_d3dContext.Get(); }
		D3D_FEATURE_LEVEL			GetDeviceFeatureLevel() const			{ return m_d3dFeatureLevel; }

		// D2D Accessors.
		ID2D1Factory3*				GetD2DFactory() const					{ return m_d2dFactory.Get(); }
		ID2D1Device2*				GetD2DDevice() const					{ return m_d2dDevice.Get(); }
		ID2D1DeviceContext2*		GetD2DDeviceContext() const				{ return m_d2dContext.Get(); }
		IDWriteFactory3*			GetDWriteFactory() const				{ return m_dwriteFactory.Get(); }
		IWICImagingFactory2*		GetWicImagingFactory() const			{ return m_wicFactory.Get(); }

	private:
		void CreateDeviceIndependentResources();
		void CreateDeviceResources();
		void UpdateDisplayVariables();

		// Direct3D objects.
		Microsoft::WRL::ComPtr<ID3D11Device3>			m_d3dDevice;
		Microsoft::WRL::ComPtr<ID3D11DeviceContext3>	m_d3dContext;

		// Direct2D drawing components.
		Microsoft::WRL::ComPtr<ID2D1Factory3>			m_d2dFactory;
		Microsoft::WRL::ComPtr<ID2D1Device2>			m_d2dDevice;
		Microsoft::WRL::ComPtr<ID2D1DeviceContext2>		m_d2dContext;

		// DirectWrite drawing components.
		Microsoft::WRL::ComPtr<IDWriteFactory3>			m_dwriteFactory;
		Microsoft::WRL::ComPtr<IWICImagingFactory2>		m_wicFactory;

		// Cached device properties.
		D3D_FEATURE_LEVEL								m_d3dFeatureLevel;

		float											m_dpi;


		// The IDeviceNotify can be held directly as it owns the DeviceResources.
		IDeviceNotify* m_deviceNotify;
	};
}