
#if !defined YU_MODULE_INTERFACE
#if defined IMPL_MODULE
#include "../core/platform.h"
#include "../core/worker.h"
#include "../core/timer.h"
#include "../core/system.h"
#include "../core/platform.h"
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

#define CREATE_EVENT_QUEUE(name) yu::EventQueue* name(yu::WindowManager* winMgr, yu::Allocator* allocator)
#define CREATE_EVENT_QUEUE_PARAM (winMgr, allocator)
#define DEQUEUE_EVENT(name) bool name(yu::EventQueue* queue, yu::InputEvent& ev)
#define DEQUEUE_EVENT_PARAM (queue, ev)

#define NEW_WORK_ITEM(name) yu::WorkItemHandle name()
#define NEW_WORK_ITEM_PARAM ()
#define FREE_WORK_ITEM(name) void name(yu::WorkItemHandle item)
#define FREE_WORK_ITEM_PARAM (item)
#define GET_WORK_ITEM(name) yu::WorkItem* name(yu::WorkItemHandle item)
#define GET_WORK_ITEM_PARAM (item)
#define GET_WORK_DATA(name) yu::WorkData name(yu::WorkItem* item)
#define GET_WORK_DATA_PARAM (item)
#define SET_WORK_DATA(name) void name(yu::WorkItem* item, yu::WorkData data)
#define SET_WORK_DATA_PARAM (item, data)
#define SET_WORK_FUNC(name) void name(yu::WorkItem* item, yu::WorkFunc* func, yu::Finalizer* finalizer)
#define SET_WORK_FUNC_PARAM (item, func, finalizer)
#define SUBMIT_WORK_ITEM(name) void name(yu::WorkItem* item, yu::WorkItem* dep[], int numDep)
#define SUBMIT_WORK_ITEM_PARAM (item, dep, numDep)
#define IS_DONE(name) bool name(yu::WorkItem* item)
#define IS_DONE_PARAM (item)
#define RESET_WORK_ITEM(name) void name(yu::WorkItem* item)
#define RESET_WORK_ITEM_PARAM (item)

#define SAMPLE_TIME(name) yu::Time name()
#define SAMPLE_TIME_PARAM ()
#define SYS_START_TIME(name) yu::Time name()
#define SYS_START_TIME_PARAM ()

#define GET_THREAD_LOCAL_RENDER_QUEUE(name) yu::RenderQueue name()
#define GET_THREAD_LOCAL_RENDER_QUEUE_PARAM ()
#define CREATE_CAMERA(name) yu::CameraHandle name(RenderQueue* queue)
#define CREATE_CAMERA_PARAM (queue)


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

FR(CreateEventQueue, CREATE_EVENT_QUEUE)
FR(DequeueEvent, DEQUEUE_EVENT)
	
FR(NewWorkItem, NEW_WORK_ITEM)
F(FreeWorkItem, FREE_WORK_ITEM)
FR(GetWorkItem, GET_WORK_ITEM)
FR(GetWorkData, GET_WORK_DATA)
F(SetWorkData, SET_WORK_DATA)
F(SetWorkFunc, SET_WORK_FUNC)
F(SubmitWorkItem, SUBMIT_WORK_ITEM)
FR(IsDone, IS_DONE)
F(ResetWorkItem, RESET_WORK_ITEM)

FR(SampleTime, SAMPLE_TIME)
FR(SysStartTime, SYS_START_TIME)

#if defined FV
#undef FV
#endif

#if defined FR
#undef FR
#endif

#if defined F
#undef F
#endif
