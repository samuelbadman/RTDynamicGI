#pragma once

#include "Input/InputCodes.h"

struct InputEvent
{
	bool RepeatedKey;
	InputCode Input;
	uint32_t Port;
	float Data;
};

struct WindowClosedEvent {};
struct WindowDestroyedEvent {};
struct WindowEndMoveEvent {};
struct WindowEnterFullscreenEvent {};
struct WindowExitFullscreenEvent {};
struct WindowLostFocusEvent {};
struct WindowMaximizedEvent {};
struct WindowMinimizedEvent {};
struct WindowMoveEvent {};
struct WindowReceivedFocusEvent {};
struct WindowResizedEvent {};
struct WindowRestoredEvent {};