#ifndef YU_RENDERER_H
#define YU_RENDERER_H

namespace yu
{
struct DisplayMode;
struct Window;

enum TexFormat
{
	TEX_FORMAT_UNKNOWN,
	TEX_FORMAT_R8G8B8A8_UNORM,
	TEX_FORMAT_R8G8B8A8_UNORM_SRGB,

	NUM_TEX_FORMATS
};

struct FrameBufferDesc
{
	TexFormat	format;
	double		refreshRate;
	int			width;
	int			height;
	int			sampleCount;
	bool		fullScreen;
};

struct FrameBuffer
{

};

void InitDX11Thread(const Window& win, const FrameBufferDesc& desc);
void SwapFrameBuffer();


}


#endif
