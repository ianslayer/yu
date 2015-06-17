
#if !defined YU_MODULE_INTERFACE
#if defined IMPL_MODULE
#include "../core/platform.h"
#include "../core/worker.h"
#include "../core/timer.h"
#include "../core/system.h"
#include "../core/thread.h"
#include "../core/log.h"
#include "../core/file.h"
#include "../core/log.h"
#include <new>
#include "../core/allocator.h"
#define YU_LIB_IMPL
#include "../core/yu_lib.h"
#include "../renderer/renderer.h"
#endif

#include <stdarg.h>
#define VLOG(name) void name(const char* fmt, va_list args)
#define VLOG_PARAM (fmt, args)
#define LOG(name) void name(char const* fmt, ...)
//#define LOG_PARAM (fmt)
#define VFILTER_LOG(name) void name(int filter, const char* fmt, va_list args)
#define VFILTER_LOG_PARAM (filter, fmt, args)
#define FILTER_LOG(name) void name(int filter, const char* fmt, ...)
#define SET_LOG_FILTER(name) void  name(int filter)
#define SET_LOG_FILTER_PARAM (filter)

//thread runtime

//allocator
#define GET_CURRENT_ALLOCATOR(name) yu::Allocator* name()
#define GET_CURRENT_ALLOCATOR_PARAM ()
#define PUSH_ALLOCATOR(name) void name(yu::Allocator* allocator)
#define PUSH_ALLOCATOR_PARAM (allocator)
#define POP_ALLOCATOR(name) yu::Allocator* name()
#define POP_ALLOCATOR_PARAM ()
#define CREATE_ARENA_ALLOCATOR(name) yu::ArenaAllocator* name(size_t blockSize, yu::Allocator* baseAllocator)
#define CREATE_ARENA_ALLOCATOR_PARAM (blockSize, baseAllocator)
#define FREE_ARENA_ALLOCATOR(name) void name(yu::ArenaAllocator* arena)
#define FREE_ARENA_ALLOCATOR_PARAM (arena)
#define CREATE_STACK_ALLOCATOR(name) yu::StackAllocator* name(size_t bufferSize, yu::Allocator* baseAllocator)
#define CREATE_STACK_ALLOCATOR_PARAM (bufferSize, baseAllocator)
#define FREE_STACK_ALLOCATOR(name) void name(yu::StackAllocator* stack)
#define FREE_STACK_ALLOCATOR_PARAM (stack)

#define WORKING_DIR(name) const char* name()
#define WORKING_DIR_PARAM ()
#define EXE_PATH(name) const char* name()
#define EXE_PATH_PARAM ()
#define DATA_PATH(name) const char* name()
#define DATA_PATH_PARAM ()
#define BUILD_DATA_PATH(name) char* name(const char* relPath, char* buffer, size_t bufferLen)
#define BUILD_DATA_PATH_PARAM (relPath, buffer, bufferLen)
#define READ_DATA_BLOB(name) yu::DataBlob name(const char* path)
#define READ_DATA_BLOB_PARAM (path)
#define FREE_DATA_BLOB(name) void name(yu::DataBlob blob)
#define FREE_DATA_BLOB_PARAM (blob)

#define CREATE_EVENT_QUEUE(name) yu::EventQueue* name(yu::WindowManager* winMgr)
#define CREATE_EVENT_QUEUE_PARAM (winMgr)
#define DEQUEUE_EVENT(name) bool name(yu::EventQueue* queue, yu::InputEvent& ev)
#define DEQUEUE_EVENT_PARAM (queue, ev)


//worker system
#define NEW_WORK_ITEM(name) yu::WorkHandle name()
#define NEW_WORK_ITEM_PARAM ()
#define FREE_WORK_ITEM(name) void name(yu::WorkHandle item)
#define FREE_WORK_ITEM_PARAM (item)
#define GET_WORK_DATA(name) yu::WorkData name(yu::WorkHandle item)
#define GET_WORK_DATA_PARAM (item)
#define SET_WORK_DATA(name) void name(yu::WorkHandle item, yu::WorkData data)
#define SET_WORK_DATA_PARAM (item, data)
#define SET_WORK_FUNC(name) void name(yu::WorkHandle item, yu::WorkFunc* func, yu::Finalizer* finalizer)
#define SET_WORK_FUNC_PARAM (item, func, finalizer)
#define SUBMIT_WORK_ITEM(name) void name(yu::WorkHandle item, yu::WorkHandle dep[], int numDep)
#define SUBMIT_WORK_ITEM_PARAM (item, dep, numDep)
#define IS_DONE(name) bool name(yu::WorkHandle item)
#define IS_DONE_PARAM (item)
#define RESET_WORK_ITEM(name) void name(yu::WorkHandle item)
#define RESET_WORK_ITEM_PARAM (item)
#define GET_WORKER_THREAD_INDEX(name) int name()
#define GET_WORKER_THREAD_INDEX_PARAM ()
#define ADD_END_FRAME_DEPENDENCY(name) void name(yu::WorkHandle item)
#define ADD_END_FRAME_DEPENDENCY_PARAM (item)
#define FRAME_COUNT(name) yu::u64 name()
#define FRAME_COUNT_PARAM ()

//timer
#define SAMPLE_TIME(name) yu::Time name()
#define SAMPLE_TIME_PARAM ()
#define SYS_START_TIME(name) yu::Time name()
#define SYS_START_TIME_PARAM ()


//renderer
#define GET_THREAD_LOCAL_RENDER_QUEUE(name) yu::RenderQueue* name()
#define GET_THREAD_LOCAL_RENDER_QUEUE_PARAM ()

//debug
#define GET_RENDER_QUEUE(name) yu::RenderQueue* name(int index)
#define GET_RENDER_QUEUE_PARAM (index)
#define GET_RENDER_QUEUE_INDEX(name) int name(yu::RenderQueue* queue)
#define GET_RENDER_QUEUE_INDEX_PARAM (queue)
#define RENDER_QUEUE_EMPTY(name) bool name(yu::RenderQueue* queue)
#define RENDER_QUEUE_EMPTY_PARAM (queue)
//debug
#define GET_RENDERER(name) yu::Renderer* name()
#define GET_RENDERER_PARAM ()
#define GET_FRAMEBUFFER_RENDER_TEXTURE(name) yu::RenderTextureHandle name(yu::Renderer* renderer)
#define GET_FRAMEBUFFER_RENDER_TEXTURE_PARAM (renderer)

#define CREATE_MESH(name) yu::MeshHandle name(yu::RenderQueue* queue, yu::u32 numVertices, yu::u32 numIndices, yu::u32 vertChannelMask)
#define CREATE_MESH_PARAM (queue, numVertices, numIndices, vertChannelMask)
#define UPDATE_MESH(name) void name(yu::RenderQueue* queue, yu::MeshHandle mesh, yu::u32 startVert, yu::u32 startIndex, yu::MeshData* subMeshData)
#define UPDATE_MESH_PARAM (queue, mesh, startVert, startIndex, subMeshData)
#define CREATE_CONST_BUFFER(name) yu::ConstBufferHandle name(yu::RenderQueue* queue, size_t bufferSize, void* initData)
#define CREATE_CONST_BUFFER_PARAM (queue, bufferSize, initData)
#define UPDATE_CONST_BUFFER(name) void name(yu::RenderQueue* queue, yu::ConstBufferHandle buffer, size_t startOffset, size_t updateSize, void* updateData)
#define UPDATE_CONST_BUFFER_PARAM (queue, buffer, startOffset, updateSize, updateData)
#define CREATE_VERTEX_SHADER(name) yu::VertexShaderHandle name(yu::RenderQueue* queue, const yu::DataBlob& shaderData)
#define CREATE_VERTEX_SHADER_PARAM (queue, shaderData)
#define FREE_VERTEX_SHADER(name) void name(yu::RenderQueue* queue, yu::VertexShaderHandle vertexShader)
#define FREE_VERTEX_SHADER_PARAM (queue, vertexShader)
#define CREATE_PIXEL_SHADER(name) yu::PixelShaderHandle name(yu::RenderQueue* queue, const yu::DataBlob& shaderData)
#define CREATE_PIXEL_SHADER_PARAM (queue, shaderData)
#define FREE_PIXEL_SHADER(name) void name(yu::RenderQueue* queue, yu::PixelShaderHandle pixelShader)
#define FREE_PIXEL_SHADER_PARAM (queue, pixelShader)
#define CREATE_PIPELINE(name) yu::PipelineHandle name(yu::RenderQueue* queue,const yu::PipelineData&  pipelineData)
#define CREATE_PIPELINE_PARAM (queue, pipelineData)
#define FREE_PIPELINE(name) void name(yu::RenderQueue* queue, yu::PipelineHandle pipeline)
#define FREE_PIPELINE_PARAM (queue, pipeline)
#define CREATE_TEXTURE(name) yu::TextureHandle name(yu::RenderQueue* queue, const yu::TextureDesc& desc, yu::TextureMipData* initData)
#define CREATE_TEXTURE_PARAM (queue, desc, initData)
#define CREATE_SAMPLER(name) yu::SamplerHandle name(yu::RenderQueue* queue, const yu::SamplerStateDesc& desc)
#define CREATE_SAMPLER_PARAM (queue, desc)
#define TEXTURE_SIZE(name) size_t name(yu::TextureFormat format, int width, int height, int depth, int mipLevels)
#define TEXTURE_SIZE_PARAM (format, width, height, depth, mipLevels)
#define TEXTURE_LEVEL_SIZE(name) size_t name(yu::TextureFormat format, int width, int height, int depth, int mipSlice)
#define TEXTURE_LEVEL_SIZE_PARAM (format, width, height, depth, mipSlice)
#define CREATE_FENCE(name) yu::FenceHandle name(yu::RenderQueue* queue)
#define CREATE_FENCE_PARAM (queue)
#define INSERT_FENCE(name) void name(yu::RenderQueue* queue, yu::FenceHandle fence)
#define INSERT_FENCE_PARAM (queue, fence)
#define WAIT_FENCE(name) void name(yu::RenderQueue* queue, yu::FenceHandle fence)
#define WAIT_FENCE_PARAM (queue, fence)
#define WAIT_AND_RESET_FENCE(name) void name(yu::RenderQueue* queue, yu::FenceHandle fence)
#define WAIT_AND_RESET_FENCE_PARAM (queue, fence)
#define RESET_FENCE(name) void name(yu::RenderQueue* queue, yu::FenceHandle fence)
#define RESET_FENCE_PARAM (queue, fence)
#define RENDER(name) void name(yu::RenderQueue* queue, yu::RenderTextureHandle renderTexture, yu::ConstBufferHandle cameraConstBuffer, yu::MeshHandle mesh, yu::ConstBufferHandle transformConstBuffer, yu::PipelineHandle pipeline, const yu::RenderResource& resource)
#define RENDER_PARAM (queue, renderTexture, cameraConstBuffer, mesh, transformConstBuffer, pipeline, resource)
#define FLUSH(name) void name(yu::RenderQueue* queue)
#define FLUSH_PARAM (queue)

#define YU_MODULE_INTERFACE
#endif

#if !defined F
#define F(a, b)
#endif

#if !defined FR
#define FR F
#endif

#if !defined FV
#define FV F
#endif

F(VLog, VLOG)
FV(Log, LOG)
F(VFilterLog, VFILTER_LOG)
FV(FilterLog, FILTER_LOG)
F(SetLogFilter, SET_LOG_FILTER)

FR(GetCurrentAllocator, GET_CURRENT_ALLOCATOR)
F(PushAllocator, PUSH_ALLOCATOR)
FR(PopAllocator, POP_ALLOCATOR)
FR(CreateArenaAllocator, CREATE_ARENA_ALLOCATOR)
F(FreeArenaAllocator, FREE_ARENA_ALLOCATOR)
FR(CreateStackAllocator, CREATE_STACK_ALLOCATOR)
F(FreeStackAllocator, FREE_STACK_ALLOCATOR)

FR(ReadDataBlob, READ_DATA_BLOB)
F(FreeDataBlob, FREE_DATA_BLOB)
FR(WorkingDir, WORKING_DIR)
FR(ExePath, EXE_PATH)
FR(DataPath, DATA_PATH)
FR(BuildDataPath, BUILD_DATA_PATH)

FR(CreateEventQueue, CREATE_EVENT_QUEUE)
FR(DequeueEvent, DEQUEUE_EVENT)
	
FR(NewWorkItem, NEW_WORK_ITEM)
F(FreeWorkItem, FREE_WORK_ITEM)
FR(GetWorkData, GET_WORK_DATA)
F(SetWorkData, SET_WORK_DATA)
F(SetWorkFunc, SET_WORK_FUNC)
F(SubmitWorkItem, SUBMIT_WORK_ITEM)
FR(IsDone, IS_DONE)
FR(GetWorkerThreadIndex, GET_WORKER_THREAD_INDEX)
F(ResetWorkItem, RESET_WORK_ITEM)
F(AddEndFrameDependency, ADD_END_FRAME_DEPENDENCY)
FR(FrameCount, FRAME_COUNT)

FR(SampleTime, SAMPLE_TIME)
FR(SysStartTime, SYS_START_TIME)

FR(GetThreadLocalRenderQueue, GET_THREAD_LOCAL_RENDER_QUEUE)

#if defined YU_DEBUG
FR(GetRenderQueue, GET_RENDER_QUEUE)
FR(GetRenderQueueIndex, GET_RENDER_QUEUE_INDEX)
FR(RenderQueueEmpty, RENDER_QUEUE_EMPTY)
#endif


FR(GetRenderer, GET_RENDERER)
FR(GetFrameBufferRenderTexture, GET_FRAMEBUFFER_RENDER_TEXTURE)

FR(CreateMesh, CREATE_MESH)
F(UpdateMesh, UPDATE_MESH)
FR(CreateConstBuffer, CREATE_CONST_BUFFER)
F(UpdateConstBuffer, UPDATE_CONST_BUFFER)
FR(CreateVertexShader, CREATE_VERTEX_SHADER)
F(FreeVertexShader, FREE_VERTEX_SHADER)
FR(CreatePixelShader, CREATE_PIXEL_SHADER)
F(FreePixelShader, FREE_PIXEL_SHADER)
FR(CreatePipeline, CREATE_PIPELINE)
F(FreePipeline, FREE_PIPELINE)
FR(CreateTexture, CREATE_TEXTURE)
FR(CreateSampler, CREATE_SAMPLER)
FR(TextureSize, TEXTURE_SIZE)
FR(TextureLevelSize, TEXTURE_LEVEL_SIZE)
FR(CreateFence, CREATE_FENCE)
F(InsertFence, INSERT_FENCE)
F(WaitFence, WAIT_FENCE)
F(WaitAndResetFence, WAIT_AND_RESET_FENCE)
F(ResetFence, RESET_FENCE)
F(Render, RENDER)
F(Flush, FLUSH)

#if defined FV
#undef FV
#endif

#if defined FR
#undef FR
#endif

#if defined F
#undef F
#endif


