#pragma once

#include "Events.h"

namespace EventSystem
{
	template <typename T>
	void SendEventImmediate(T&& event);

	template <typename T>
	void SubscribeToEvent(std::function<void(T&&)>&& subscription);
}