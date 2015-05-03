#include "module.h"
#include "../renderer/renderer.h"


extern "C" MODULE_UPDATE(ModuleUpdate)
{
	yu::RenderQueue* queue = yu::GetThreadLocalRenderQueue();
}
