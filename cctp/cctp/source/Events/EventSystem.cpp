#include "Pch.h"
#include "EventSystem.h"

#define EVENT_IMPLEMENTATION(EventType, EventSubscriptionsName) static std::vector<std::function<void(EventType&&)>> EventSubscriptionsName;\
template<>\
void EventSystem::SendEventImmediate(EventType&& event)\
{\
	for (const auto& subscription : EventSubscriptionsName)\
	{\
		subscription(static_cast<EventType&&>(event));\
	}\
}\
\
template<>\
void EventSystem::SubscribeToEvent(std::function<void(EventType&&)>&& subscription)\
{\
	EventSubscriptionsName.emplace_back(static_cast<std::function<void(EventType&&)>&&>(subscription));\
}\

EVENT_IMPLEMENTATION(InputEvent, InputEventSubscriptions);
EVENT_IMPLEMENTATION(WindowMoveEvent, WindowMoveEventSubscriptions);
EVENT_IMPLEMENTATION(WindowEndMoveEvent, WindowEndMoveEventSubscriptions);
EVENT_IMPLEMENTATION(WindowReceivedFocusEvent, WindowReceivedFocusEventSubscriptions);
EVENT_IMPLEMENTATION(WindowLostFocusEvent, WindowLostFocusEventSubscriptions);
EVENT_IMPLEMENTATION(WindowMaximizedEvent, WindowMaximizedEventSubscriptions);
EVENT_IMPLEMENTATION(WindowMinimizedEvent, WindowMinimizedEventSubscriptions);
EVENT_IMPLEMENTATION(WindowRestoredEvent, WindowRestoredEventSubscriptions);
EVENT_IMPLEMENTATION(WindowResizedEvent, WindowResizedEventSubscriptions);
EVENT_IMPLEMENTATION(WindowEnterFullscreenEvent, WindowEnterFullscreenEventSubscriptions);
EVENT_IMPLEMENTATION(WindowExitFullscreenEvent, WindowExitFullscreenEventSubscriptions);
EVENT_IMPLEMENTATION(WindowClosedEvent, WindowClosedEventSubscriptions);
EVENT_IMPLEMENTATION(WindowDestroyedEvent, WindowDestroyedEventSubscriptions);
