#include "Pch.h"
#include "Window.h"
#include "Events/EventSystem.h"

HWND Handle = nullptr;
bool Fullscreen = false;
RECT WindowRectAtEnterFullscreen = {};

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
	{
		return true;
	}

	switch (msg)
	{
	case WM_MOUSEWHEEL:
	{
		const auto delta = GET_WHEEL_DELTA_WPARAM(wParam);
		if (delta > 0)
		{
			// Mouse wheel up
			EventSystem::SendEventImmediate(InputEvent{ false, InputCodes::Mouse_Wheel_Up, Window::MOUSE_KEYBOARD_PORT, 1.0f });
		}
		else if (delta < 0)
		{
			// Mouse wheel down
			EventSystem::SendEventImmediate(InputEvent{ false, InputCodes::Mouse_Wheel_Down, Window::MOUSE_KEYBOARD_PORT, 1.0f });
		}

		return 0;
	}

	case WM_INPUT:
	{
		// Initialize raw data size
		UINT dataSize = 0;

		// Retrieve the size of the raw data
		::GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, NULL, &dataSize, sizeof(RAWINPUTHEADER));

		// Return if there was no raw data
		if (dataSize == 0)
		{
			return 0;
		}

		// Initialize raw data storage
		std::unique_ptr<BYTE[]> rawData = std::make_unique<BYTE[]>(dataSize);

		// Retreive raw data and store it. Return if the retreived data is the not same size
		if (::GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, rawData.get(), &dataSize, sizeof(RAWINPUTHEADER)) != dataSize)
		{
			return 0;
		}

		// Cast byte to raw input
		RAWINPUT* raw = reinterpret_cast<RAWINPUT*>(rawData.get());

		// Return if the raw input is not from the mouse
		if (raw->header.dwType != RIM_TYPEMOUSE)
		{
			return 0;
		}

		// Raw mouse delta
		EventSystem::SendEventImmediate(InputEvent{ false, InputCodes::Mouse_X, Window::MOUSE_KEYBOARD_PORT, static_cast<float>(raw->data.mouse.lLastX) });
		EventSystem::SendEventImmediate(InputEvent{ false, InputCodes::Mouse_Y, Window::MOUSE_KEYBOARD_PORT, static_cast<float>(raw->data.mouse.lLastY) });

		return 0;
	}

	case WM_LBUTTONDOWN:
		// Mouse left button down
		EventSystem::SendEventImmediate(InputEvent{ false, InputCodes::Left_Mouse_Button, Window::MOUSE_KEYBOARD_PORT, 1.0f });

		return 0;

	case WM_RBUTTONDOWN:
		// Mouse right button down
		EventSystem::SendEventImmediate(InputEvent{ false, InputCodes::Right_Mouse_Button, Window::MOUSE_KEYBOARD_PORT, 1.0f });

		return 0;

	case WM_MBUTTONDOWN:
		// Mouse middle button down
		EventSystem::SendEventImmediate(InputEvent{ false, InputCodes::Middle_Mouse_Button, Window::MOUSE_KEYBOARD_PORT, 1.0f });

		return 0;

	case WM_LBUTTONUP:
		// Mouse left button up
		EventSystem::SendEventImmediate(InputEvent{ false, InputCodes::Left_Mouse_Button, Window::MOUSE_KEYBOARD_PORT, 0.0f });

		return 0;

	case WM_RBUTTONUP:
		// Mouse right button up
		EventSystem::SendEventImmediate(InputEvent{ false, InputCodes::Right_Mouse_Button, Window::MOUSE_KEYBOARD_PORT, 0.0f });

		return 0;

	case WM_MBUTTONUP:
		// Mouse middle button up
		EventSystem::SendEventImmediate(InputEvent{ false, InputCodes::Middle_Mouse_Button, Window::MOUSE_KEYBOARD_PORT, 0.0f });

		return 0;

	case WM_KEYDOWN:
		// Key down
		EventSystem::SendEventImmediate(InputEvent{ static_cast<bool>(lParam & 0x40000000), static_cast<int16_t>(wParam), Window::MOUSE_KEYBOARD_PORT, 1.0f });

		return 0;

	case WM_KEYUP:
		// Key up
		EventSystem::SendEventImmediate(InputEvent{ false, static_cast<int16_t>(wParam), Window::MOUSE_KEYBOARD_PORT, 0.0f });

		return 0;

	case WM_ENTERSIZEMOVE:
		// GameWindow move started
		EventSystem::SendEventImmediate(WindowMoveEvent{});

		return 0;

	case WM_EXITSIZEMOVE:
		// GameWindow move finished
		EventSystem::SendEventImmediate(WindowEndMoveEvent{});

		return 0;

	case WM_ACTIVATEAPP:
		if (wParam == TRUE)
		{
			// Received focus
			EventSystem::SendEventImmediate(WindowReceivedFocusEvent{});

			return 0;
		}
		else if (wParam == FALSE)
		{
			// Lost focus
			EventSystem::SendEventImmediate(WindowLostFocusEvent{});

			return 0;
		}

		return 0;

	case WM_SIZE:
		switch (wParam)
		{
		case SIZE_MAXIMIZED:
			// GameWindow maximized
			EventSystem::SendEventImmediate(WindowResizedEvent{});
			EventSystem::SendEventImmediate(WindowMaximizedEvent{});

			return 0;

		case SIZE_MINIMIZED:
			// GameWindow minimized
			EventSystem::SendEventImmediate(WindowResizedEvent{});
			EventSystem::SendEventImmediate(WindowMinimizedEvent{});

			return 0;

		case SIZE_RESTORED:
			// GameWindow restored
			EventSystem::SendEventImmediate(WindowRestoredEvent{});
			EventSystem::SendEventImmediate(WindowResizedEvent{});

			return 0;
		}

		return 0;

	case WM_CLOSE:
		// GameWindow closed
		EventSystem::SendEventImmediate(WindowClosedEvent{});
		DestroyWindow(hWnd);

		return 0;

	case WM_DESTROY:
		// GameWindow destroyed
		EventSystem::SendEventImmediate(WindowDestroyedEvent{});
		PostQuitMessage(0);

		return 0;

	default:
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}
}

bool EnterFullscreen()
{
	auto hwnd = static_cast<HWND>(Handle);

	// Store the current window area rect
	if (!Window::GetWindowAreaRect(WindowRectAtEnterFullscreen))
	{
		return false;
	}

	// Retreive info about the monitor the window is on
	HMONITOR hMon = ::MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
	MONITORINFO monitorInfo = { sizeof(monitorInfo) };
	if (!::GetMonitorInfo(hMon, &monitorInfo))
	{
		return false;
	}

	// Calculate width and height of the monitor
	auto fWidth = monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left;
	auto fHeight = monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top;

	// Update position and size of the window
	if (::SetWindowPos(hwnd,
		HWND_TOP, monitorInfo.rcMonitor.left, monitorInfo.rcMonitor.top, fWidth, fHeight,
		SWP_FRAMECHANGED | SWP_NOACTIVATE) == 0)
	{
		return false;
	}

	// Update window style
	if (::SetWindowLong(hwnd, GWL_STYLE, 0) == 0)
	{
		return false;
	}

	// Show the window maximized
	::ShowWindow(hwnd, SW_MAXIMIZE);

	Fullscreen = true;

	return true;
}

bool EnterWindowed()
{
	auto hwnd = static_cast<HWND>(Handle);

	// Update position and size of the window
	if (::SetWindowPos(hwnd,
		HWND_TOP, WindowRectAtEnterFullscreen.left, WindowRectAtEnterFullscreen.top,
		WindowRectAtEnterFullscreen.right - WindowRectAtEnterFullscreen.left, WindowRectAtEnterFullscreen.bottom - WindowRectAtEnterFullscreen.top,
		SWP_FRAMECHANGED | SWP_NOACTIVATE) == 0)
	{
		return false;
	}

	// Update window style
	if (::SetWindowLong(hwnd, GWL_STYLE, Window::STYLE_WINDOWED) == 0)
	{
		return false;
	}

	// Show the window
	::ShowWindow(hwnd, SW_SHOW);

	Fullscreen = false;

	return true;
}

bool Window::Init(const std::wstring& title, const glm::vec2& size, const uint32_t style)
{
	auto hInstance = GetModuleHandle(NULL);
	auto windowClassName = L"WindowClass";

	// Register window class
	WNDCLASSEX windowClass = {};
	windowClass.cbSize = sizeof(WNDCLASSEX);
	windowClass.style = CS_HREDRAW | CS_VREDRAW;
	windowClass.lpfnWndProc = &WindowProc;
	windowClass.cbClsExtra = 0;
	windowClass.cbWndExtra = 0;
	windowClass.hInstance = hInstance;
	windowClass.hIcon = ::LoadIcon(hInstance, IDI_APPLICATION);
	windowClass.hCursor = ::LoadCursor(NULL, IDC_ARROW);
	windowClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	windowClass.lpszMenuName = NULL;
	windowClass.lpszClassName = windowClassName;
	windowClass.hIconSm = ::LoadIcon(hInstance, IDI_APPLICATION);

	if (!(::RegisterClassExW(&windowClass) > 0))
	{
		return false;
	}

	// Create window
	Handle = CreateWindowExW(NULL, windowClassName, title.c_str(), style,
		CW_USEDEFAULT, CW_USEDEFAULT, static_cast<int>(size.x), static_cast<int>(size.y), NULL, NULL, hInstance, nullptr);
	if (Handle == NULL)
	{
		return false;
	}

	Fullscreen = false;

	if (!SetSize(size))
	{
		return false;
	}

	// Register raw input devices
	RAWINPUTDEVICE rid;
	rid.usUsagePage = 0x01;
	rid.usUsage = 0x02;
	rid.dwFlags = 0;
	rid.hwndTarget = NULL;
	if (!RegisterRawInputDevices(&rid, 1, sizeof(rid)))
	{
		return false;
	}

	return true;
}

bool Window::RunOSMessageLoop()
{
	MSG msg = {};
	while (::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
	{
		::TranslateMessage(&msg);
		::DispatchMessage(&msg);

		if (msg.message == WM_QUIT)
		{
			return true;
		}
	}

	return false;
}

HWND Window::GetHandle()
{
	return Handle;
}

bool Window::GetWindowAreaRect(RECT& rect)
{
	return ::GetWindowRect(Handle, &rect);
}

bool Window::GetClientAreaRect(RECT& rect)
{
	return ::GetClientRect(Handle, &rect);
}

bool Window::SetFullscreen(bool fullscreen)
{
	if (fullscreen)
	{
		if (!Fullscreen)
		{
			if (!EnterFullscreen())
			{
				return false;
			}
		}
	}
	else
	{
		if (Fullscreen)
		{
			if (!EnterWindowed())
			{
				return false;
			}
		}
	}

	return true;
}

void Window::Show(int flag)
{
	::ShowWindow(Handle, flag);
}

bool Window::SetSize(const glm::vec2& size)
{
	// Retreive info about the monitor the window is on
	HMONITOR hMon = ::MonitorFromWindow(static_cast<HWND>(Handle), MONITOR_DEFAULTTONEAREST);
	MONITORINFO monitorInfo = { sizeof(monitorInfo) };
	if (!::GetMonitorInfo(hMon, &monitorInfo))
	{
		return false;
	}

	// Calculate width and height of the monitor
	auto monitorWidth = monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left;
	auto monitorHeight = monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top;

	// Position the window in the center of the monitor
	return ::SetWindowPos(static_cast<HWND>(Handle),
		HWND_TOP,
		(monitorWidth / 2) - (static_cast<long>(size.x) / 2), (monitorHeight / 2) - (static_cast<long>(size.y) / 2),
		static_cast<int>(size.x), static_cast<int>(size.y),
		SWP_FRAMECHANGED | SWP_NOACTIVATE) != 0;
}

bool Window::Close()
{
	return ::DestroyWindow(Handle) != 0;
}

bool Window::CaptureCursor()
{
	RECT clientRect;
	if (!Window::GetClientAreaRect(clientRect))
	{
		return false;
	}

	POINT ul;
	ul.x = clientRect.left;
	ul.y = clientRect.top;

	POINT lr;
	lr.x = clientRect.right;
	lr.y = clientRect.bottom;

	MapWindowPoints(Handle, nullptr, &ul, 1);
	MapWindowPoints(Handle, nullptr, &lr, 1);

	clientRect.left = ul.x;
	clientRect.top = ul.y;

	clientRect.right = lr.x;
	clientRect.bottom = lr.y;

	return ClipCursor(&clientRect) != 0;
}

bool Window::ReleaseCursor()
{
	return ClipCursor(NULL) != 0;
}
