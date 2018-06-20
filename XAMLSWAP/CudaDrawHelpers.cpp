#include "pch.h"
#include "Content\CudaDraw.h"

using namespace XAMLSWAP;


//Zoom ----------------------------------

void CudaDraw::ZoomUpdate(float delta, float xPos, float yPos) {
	m_texCompMutex.lock();
	CBOffsetZoom* cbd = &m_CBDTexOffsetZoom;
	float xRatio = xPos / m_SCPanel->GetRectWidth();
	float yRatio = 1.0f - yPos / m_SCPanel->GetRectHeight();
	cbd->xOffset = min(0.0f, cbd->xOffset - 0.0004f*delta*xRatio);
	cbd->yOffset = min(0.0f, cbd->yOffset - 0.0004f*delta*yRatio);
	cbd->zoom = min(100.0f, max(1.0f, cbd->zoom + 0.0004f*delta));
	cbd->xOffset = cbd->zoom + cbd->xOffset < 1.0f ? 1.0f - cbd->zoom : cbd->xOffset;
	cbd->yOffset = cbd->zoom + cbd->yOffset < 1.0f ? 1.0f - cbd->zoom : cbd->yOffset;
	m_texCompUpdate = true;
	m_texCompMutex.unlock();
}

//Panning -------------------------------

void CudaDraw::PanningUpdate(float xDelta, float yDelta) {
	m_texCompMutex.lock();
	CBOffsetZoom* cbd = &m_CBDTexOffsetZoom;
	cbd->xOffset = min(0.0f, cbd->xOffset + 0.005f*xDelta);
	cbd->xOffset = cbd->zoom + cbd->xOffset < 1.0f ? 1.0f - cbd->zoom : cbd->xOffset;
	cbd->yOffset = min(0.0f, cbd->yOffset - 0.005f*yDelta);
	cbd->yOffset = cbd->zoom + cbd->yOffset < 1.0f ? 1.0f - cbd->zoom : cbd->yOffset;
	m_texCompUpdate = true;
	m_texCompMutex.unlock();
}

//Surface Scroll Update-----------------

void CudaDraw::ParameterZoomUpdate(float delta, float xPos, float yPos) {
	m_paramCompMutex.lock();
	CBOffsetZoomDouble* cbd = &m_CBDParamOffsetZoom;
	double xRatio = xPos / m_logicalSize.Width;
	double yRatio = 1.0 - yPos / m_logicalSize.Height;
	cbd->xOffset = cbd->xOffset + 0.0006*delta*cbd->zoom*xRatio;
	cbd->yOffset = cbd->yOffset + 0.0006*delta*cbd->zoom*yRatio;
	cbd->zoom = cbd->zoom * ( 1 - 0.0006*delta);
	m_paramCompUpdate = true;
	m_paramCompMutex.unlock();
}

//Surface Panning Update------------------

void CudaDraw::ParameterPanningUpdate(float xDelta, float yDelta) {
	m_paramCompMutex.lock();
	CBOffsetZoomDouble* cbd = &m_CBDParamOffsetZoom;
	cbd->xOffset = cbd->xOffset - 0.001*xDelta*cbd->zoom;
	cbd->yOffset = cbd->yOffset + 0.001*yDelta*cbd->zoom;
	m_paramCompUpdate = true;
	m_paramCompMutex.unlock();
}


int factor(int n, int* primeFactors) {
	int i = 0;
	int z = 2;
	while (z * z <= n) {
		if (n % z == 0) {
			primeFactors[i] = z;
			i++;
			n /= z;
		}
		else
			z++;
	}
	if (n > 1)
		primeFactors[i] = n;
	return i + 1;
}

void CudaDraw::Draw(DrawSpec d) {
	m_drawMutex.lock();

	m_width = d.width;
	m_height = d.height;
	m_passWidth = d.passWidth;
	m_passHeight = d.passHeight;
	m_xStride = m_width / (d.passWidth*m_groupWidth);
	m_yStride = m_height / (d.passHeight*m_groupHeight);
	m_NumFactors[0] = factor(m_width / m_passWidth / m_groupWidth, m_Factors[0]);
	m_NumFactors[1] = factor(m_height / m_passHeight / m_groupHeight, m_Factors[1]);
	m_firstPass = true;

	//recreateTextureDependentResources();


	m_drawing = true;
	m_drawMutex.unlock();
}

void CudaDraw::DrawIteration() {
	m_drawMutex.lock();
	static int effStride[2], dim, factorIndex[2], f, count, jumpIdx, passIdx, jumpNum[2], numPasses[2];
	static int numIterations = 0, totalIter;
	static wchar_t iterationText[80];

	if (m_firstPass) {
		jumpNum[0] = 1;
		jumpNum[1] = 1;
		jumpIdx = 0;
		factorIndex[0] = 0;
		factorIndex[1] = 0;
		numPasses[0] = 1;
		numPasses[1] = 1;
		passIdx = 0;
		effStride[0] = m_xStride;
		effStride[1] = m_yStride;
		count = 0;
		f = 1;
		dim = 0;
		numIterations = 0;
		totalIter = m_width*m_height/(m_yStride*m_xStride);
		m_CBDComputePass.stride[0] = m_xStride;
		m_CBDComputePass.stride[1] = m_yStride;
		m_CBDComputePass.f_stride[0] = (float)m_xStride;
		m_CBDComputePass.f_stride[1] = (float)m_yStride;
		m_CBDComputePass.width = (float)m_width;
		m_CBDComputePass.height = (float)m_height;
	}
	m_CBDComputePass.Offset[dim]   = jumpIdx * effStride[dim] + count * effStride[dim] / f;
	m_CBDComputePass.Offset[dim ^ 1] = passIdx * effStride[dim ^ 1];
	m_CBDComputePass.f_Offset[0] = (float)m_CBDComputePass.Offset[0];
	m_CBDComputePass.f_Offset[1] = (float)m_CBDComputePass.Offset[1];
	//doPass
	DrawPass();
	numIterations++;
	/*if (numIterations % 100 == 0) {
		swprintf_s(iterationText, L"%d / %d iterations:", numIterations, totalIter);
		m_SCPanel->Write(m_iterationField, iterationText);
	}*/
	passIdx++;
	//Is it done with sweep of passive dim with current active dim offset?
	if (passIdx == numPasses[dim ^ 1]) {
		passIdx = 0;
		count++;
		//Is it done with one section of passive dim passes?
		if (count == f) {
			count = 1;
			jumpIdx++;
			//Is it done with all jumps?
			if (jumpIdx == jumpNum[dim]) {
				jumpIdx = 0;
				m_drawUpdate = true;
				//if (!m_firstPass)
				//	Sleep(500);
				effStride[dim] /= f;
				numPasses[dim] *= f;
				jumpNum[dim] *= f;

				m_CBDSamplerSpec.strides[0] = effStride[0];
				m_CBDSamplerSpec.strides[1] = effStride[1];
				m_deviceResources->GetD3DDeviceContext()->UpdateSubresource1(m_CBSamplerSpec.Get(), 0, NULL, &m_CBDSamplerSpec, 0, 0, 0);

				if (!m_firstPass)
					factorIndex[dim]++;
				if (effStride[0] == 1 && effStride[1] == 1) {
					//done
					m_drawing = false;
					wchar_t string[100];
					swprintf_s(string, L"Final iterations %d\n", numIterations);
					OutputDebugString(string);
					m_drawMutex.unlock();
					return;
				}

				if (effStride[0] < effStride[1])
					dim = 1;
				else
					dim = 0;
				f = m_Factors[dim][factorIndex[dim]];
			}
		}
	}
	if(m_firstPass)
		m_firstPass = false;
	m_drawMutex.unlock();
}

void CudaDraw::DrawPass() {
	auto context = m_deviceResources->GetD3DDeviceContext();
	//Setup and run compute shader
	UINT initCounts = 0;

	context->CSSetShader(m_CSFunction.Get(), nullptr, 0);
	context->UpdateSubresource1(m_CBComputePass.Get(), 0, NULL, &m_CBDComputePass, 0, 0, 0);
	context->CSSetConstantBuffers1(0, 1, m_CBComputePass.GetAddressOf(), nullptr, nullptr);
	context->CSSetConstantBuffers1(1, 1, m_CBParamOffsetZoom.GetAddressOf(), nullptr, nullptr);
	ID3D11UnorderedAccessView* uavs[2] = { m_UAVComputeTexture.Get(), m_UAVPerlin.Get() };
	context->CSSetUnorderedAccessViews(0, 2, uavs, &initCounts);


	context->Dispatch(m_passWidth, m_passHeight, 1);
	uavs[0] = nullptr;
	uavs[1] = nullptr;
	context->CSSetUnorderedAccessViews(0, 2, uavs, &initCounts); // This is what is important
}
