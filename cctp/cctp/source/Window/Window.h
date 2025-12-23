#pragma once

namespace Window
{
	constexpr uint32_t MOUSE_KEYBOARD_PORT = 0;

	constexpr uint32_t STYLE_BORDERLESS_WINDOWED = WS_POPUPWINDOW;
	constexpr uint32_t STYLE_WINDOWED = WS_OVERLAPPEDWINDOW;
	constexpr uint32_t STYLE_NO_RESIZE = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;

	bool Init(const std::wstring& title, const glm::vec2& size, const uint32_t style);
	bool RunOSMessageLoop();
	HWND GetHandle();
	bool GetWindowAreaRect(RECT& rect);
	bool GetClientAreaRect(RECT& rect);
	bool SetFullscreen(bool fullscreen);
	void Show(int flag);
	bool SetSize(const glm::vec2& size);
	bool Close();
	bool CaptureCursor();
	bool ReleaseCursor();
};