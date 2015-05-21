
#include "../core/system.h"
#define IMPL_MODULE
#include "module.h"

YU_GLOBAL YuCore* gYuCore;
namespace yum
{
#define FR(func, proto) inline proto(func) { return gYuCore->func proto##_PARAM; }
#define F(func, proto) inline proto(func) { gYuCore->func proto##_PARAM; }
#define FV(func, proto)	inline proto(func) { va_list args; va_start(args, fmt); gYuCore->V##func (fmt, args); va_end(args); }
	#include "module_interface.h"
#undef FV	
#undef F
#undef FR	
}


#define MAX_INPUT 256
#define MAX_KEYBOARD_STATE 256

using namespace yu;

struct KeyState
{
	yu::u32 pressCount;
	yu::u32 pressed;
};

struct MouseState
{
	float x, y;
	float dx, dy;
	bool	 captured;
	KeyState leftButton;
	KeyState rightButton;
};

struct JoyState
{
	yu::u32 buttonState;
	float leftX;
	float leftY;

	float rightX;
	float rightY;
};

struct InputSource : public yu::InputData
{
	yu::EventQueue* eventQueue;
};

struct InputResult : public yu::OutputData
{
	yu::InputEvent	frameEvent[MAX_INPUT];
	KeyState	keyboardState[MAX_KEYBOARD_STATE];
	MouseState	mouseState;
	JoyState	joyState[4];
	int			numEvent;
};

YU_INTERNAL void ProcessInput(const InputSource& source, InputResult& output)
{
//	int	threadIdx =	GetWorkerThreadIdx();
//	yum::Log("thread id:%d\n", threadIdx);

//	yum::Log("lala\n");

//	yum::Log("test:%d\n", 10);
	output.numEvent = 0;
	InputEvent event;

	for (int i = 0; i < MAX_KEYBOARD_STATE; i++)
	{
		output.keyboardState[i].pressCount = 0;
	}

	output.mouseState.leftButton.pressCount = 0;
	output.mouseState.rightButton.pressCount = 0;
	output.mouseState.dx = output.mouseState.dy = 0.f;

	while (yum::DequeueEvent(source.eventQueue, event) && output.numEvent < MAX_INPUT)
	{
		Time sysInitTime = yum::SysStartTime();
		Time eventTime;
		eventTime.time = event.timeStamp;

		output.frameEvent[output.numEvent++] = event;

		switch (event.type)
		{
		case InputEvent::UNKNOWN:
		break;
		case InputEvent::KILL_FOCUS:
		{
			yu::memset(&output, 0, sizeof(output));
		}
		break;
		case InputEvent::KEYBOARD:
		{
			//Log("event time: %f ", ConvertToMs(eventTime - sysInitTime) / 1000.f);
			if (event.keyboardEvent.type == InputEvent::KeyboardEvent::DOWN)
			{
				//yum::Log("%c key down\n", event.keyboardEvent.key);
				output.keyboardState[event.keyboardEvent.key].pressCount++;
				output.keyboardState[event.keyboardEvent.key].pressed = 1;
			}
			else if (event.keyboardEvent.type == InputEvent::KeyboardEvent::UP)
			{
				//yum::Log("%c key up\n", event.keyboardEvent.key);
				output.keyboardState[event.keyboardEvent.key].pressed = 0;
			}
		}break;
		case InputEvent::MOUSE:
		{
			output.mouseState.captured = event.window.captured;
			if (event.mouseEvent.type == InputEvent::MouseEvent::L_BUTTON_DOWN)
			{
				output.mouseState.leftButton.pressCount++;
				output.mouseState.leftButton.pressed = 1;
				//Log("event time: %f ", ConvertToMs(eventTime - sysInitTime) / 1000.f);
				//Log("left button down: %f, %f\n", event.mouseEvent.x, event.mouseEvent.y);
			}
			else if (event.mouseEvent.type == InputEvent::MouseEvent::L_BUTTON_UP)
			{
				output.mouseState.leftButton.pressed = 0;
			}

			if (event.mouseEvent.type == InputEvent::MouseEvent::R_BUTTON_DOWN)
			{
				output.mouseState.rightButton.pressCount++;
				output.mouseState.rightButton.pressed = 1;
				//Log("event time: %f ", ConvertToMs(eventTime - sysInitTime) / 1000.f);
				//Log("right button down: %f, %f\n", event.mouseEvent.x, event.mouseEvent.y);
			}
			else if (event.mouseEvent.type == InputEvent::MouseEvent::R_BUTTON_UP)
			{
				output.mouseState.rightButton.pressed = 0;
			}


			if (event.mouseEvent.type == InputEvent::MouseEvent::MOVE)
			{
				if (event.window.mode == Window::MOUSE_FREE)
				{
					output.mouseState.dx += (event.mouseEvent.x - output.mouseState.x);
					output.mouseState.dy += (event.mouseEvent.y - output.mouseState.y);
				}
				else if (event.window.mode == Window::MOUSE_CAPTURE)
				{
					output.mouseState.dx += event.mouseEvent.x;
					output.mouseState.x = 0;

					output.mouseState.dy += event.mouseEvent.y;
					output.mouseState.y = 0;
				}

				//Log("event time: %f ", ConvertToMs(eventTime - sysInitTime) / 1000.f);
				//Log("mouse location: %f, %f\n", event.mouseEvent.x, event.mouseEvent.y);
				//Log("mouse dx dy: %f, %f\n", output.mouseState.dx, output.mouseState.dy);
			}
			if (event.mouseEvent.type == InputEvent::MouseEvent::WHEEL)
			{
				//Log("event time: %f ", ConvertToMs(eventTime - sysInitTime) / 1000.f);
				//Log("scroll: %f\n", event.mouseEvent.scroll);
			}
			
			output.mouseState.x = event.mouseEvent.x;
			output.mouseState.y = event.mouseEvent.y;

		}break;
		case InputEvent::JOY:
		{
			if (event.joyEvent.type & InputEvent::JoyEvent::AXIS)
			{
				int controllerIdx = event.joyEvent.controllerIdx;
				output.joyState[controllerIdx].leftX = event.joyEvent.leftX;
				output.joyState[controllerIdx].leftY = event.joyEvent.leftY;
				output.joyState[controllerIdx].rightX = event.joyEvent.rightX;
				output.joyState[controllerIdx].rightY = event.joyEvent.rightY;
			}

			if (event.joyEvent.type & InputEvent::JoyEvent::BUTTON)
			{
				int controllerIdx = event.joyEvent.controllerIdx;
				output.joyState[controllerIdx].buttonState = event.joyEvent.buttonState;
			}

		}break;
		}
	}
}

//-----glue code------
void InputWorkFunc(WorkItem* item)
{
	WorkData inputWorkData = yum::GetWorkData(item);
	const InputSource* source = (const InputSource*) inputWorkData.inputData;
	InputResult* result = (InputResult*) inputWorkData.outputData;
	ProcessInput(*source, *result);
}

WorkItemHandle InputWorkItem(WindowManager* winMgr,Allocator* allocator)
{
	WorkItemHandle itemHandle = yum::NewWorkItem();
	WorkItem* item = yum::GetWorkItem(itemHandle);

	InputSource* source = New<InputSource>(allocator);
	source->eventQueue = yum::CreateEventQueue(winMgr, allocator);
	InputResult* result = New<InputResult>(allocator);
	yu::memset(result, 0, sizeof(*result));

	WorkData inputWorkData = {source, result};
	yum::SetWorkData(item, inputWorkData);
	return itemHandle;
}


struct TestModuleData
{
	yu::WorkItemHandle inputWorkItem;
};

extern "C" MODULE_UPDATE(ModuleUpdate)
{
	TestModuleData* moduleData = (TestModuleData*) context->moduleData;
	gYuCore = core;
	if(!context->initialized)
	{
		context->moduleData = moduleData = yu::New<TestModuleData>(core->sysAllocator->sysAllocator);

		moduleData->inputWorkItem = InputWorkItem(core->windowManager, core->sysAllocator->sysAllocator);
		context->initialized = true;
	}

	yu::WorkItem* inputWorkItem = yum::GetWorkItem(moduleData->inputWorkItem);
	yum::SetWorkFunc(inputWorkItem, InputWorkFunc, nullptr);
	
	yum::ResetWorkItem(inputWorkItem);

	yum::SubmitWorkItem(inputWorkItem, nullptr, 0);
	
}
