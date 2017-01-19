#include "renderer.h"
#include <thread>

Renderer::Renderer(KernelFunc kernelFunc, int width, int height, int bytesPerPixel) 
	: m_KernelFunc(kernelFunc)
	, m_Width(width)
	, m_Height(height)
	, m_BytesPerPixel(bytesPerPixel)
	, m_JobsCompleted(0)
	, m_Pixels(0)
{
	m_Pixels = new float[width * height * bytesPerPixel];
}

void Render_Thread(Renderer::KernelFunc kernelFunc, int* jobsCompleted, int startingX, int startingY, int endingX, int endingY, int width, int height, int bytesPerPixel, float* pixels)
{
	Color outputColor;
	for (int y = startingY; y < endingY; ++y)
	{
		for (int x = startingX; x < endingX; ++x)
		{
			kernelFunc(x, y, width, height, outputColor);

			int redIndex = (y*width*bytesPerPixel) + (x*bytesPerPixel + 0);
			memcpy(&pixels[redIndex], outputColor.GetValues(), sizeof(float) * bytesPerPixel);
		}
	}

	++(*jobsCompleted);
}

void Renderer::Render()
{
	int d = m_Width / 5;

	std::thread t1(Render_Thread, m_KernelFunc, &m_JobsCompleted, 0, 0, d, m_Height, m_Width, m_Height, m_BytesPerPixel, m_Pixels);
	t1.detach();

	std::thread t2(Render_Thread, m_KernelFunc, &m_JobsCompleted, d, 0, d * 2, m_Height, m_Width, m_Height, m_BytesPerPixel, m_Pixels);
	t2.detach();

	std::thread t3(Render_Thread, m_KernelFunc, &m_JobsCompleted, d * 2, 0, d * 3, m_Height, m_Width, m_Height, m_BytesPerPixel, m_Pixels);
	t3.detach();

	std::thread t4(Render_Thread, m_KernelFunc, &m_JobsCompleted, d * 3, 0, d * 4, m_Height, m_Width, m_Height, m_BytesPerPixel, m_Pixels);
	t4.detach();

	std::thread t5(Render_Thread, m_KernelFunc, &m_JobsCompleted, d * 4, 0, m_Width, m_Height, m_Width, m_Height, m_BytesPerPixel, m_Pixels);
	t5.detach();

	while (m_JobsCompleted < 5);
}