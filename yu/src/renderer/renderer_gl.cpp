#include "renderer_impl.h"

namespace yu
{

struct RendererGL : public Renderer
{
};

void InitRenderThread(const Window& win, const FrameBufferDesc& desc)
{

}

Renderer* CreateRenderer()
{
	return new RendererGL;
}

void FreeRenderer(Renderer* renderer)
{
	RendererGL* rendererGL = (RendererGL*) renderer;
	delete rendererGL;
}

}
