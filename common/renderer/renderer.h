#ifndef __RENDERER__
#define __RENDERER__
#include "color/color.h"

class Renderer
{
public:
	
	typedef void(*KernelFunc)(const unsigned int& x,
		const unsigned int& y,
		const unsigned int& width,
		const unsigned int& height,
		Color& outputColor);

	Renderer(KernelFunc kernelFunc, int width, int height, int bytesPerPixel);
	void Render();
	inline float* GetPixels() { return m_Pixels; }

private:

	KernelFunc m_KernelFunc;
	int m_Width;
	int m_Height;
	int m_BytesPerPixel;
	int m_JobsCompleted;
	float *m_Pixels;
};

#endif