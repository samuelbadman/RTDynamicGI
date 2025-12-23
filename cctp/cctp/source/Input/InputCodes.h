#pragma once

using InputCode = int16_t;

namespace InputCodes
{
	constexpr InputCode Backspace{ VK_BACK };
	constexpr InputCode Tab{ VK_TAB };
	constexpr InputCode Enter{ VK_RETURN };
	constexpr InputCode Left_Shift{ VK_LSHIFT };
	constexpr InputCode Right_Shift{ VK_RSHIFT };
	constexpr InputCode Caps_Lock{ VK_CAPITAL };
	constexpr InputCode Escape{ VK_ESCAPE };
	constexpr InputCode Space_Bar{ VK_SPACE };
	constexpr InputCode Page_Up{ VK_PRIOR };
	constexpr InputCode Page_Down{ VK_NEXT };
	constexpr InputCode End{ VK_END };
	constexpr InputCode Home{ VK_HOME };
	constexpr InputCode Insert{ VK_INSERT };
	constexpr InputCode Delete{ VK_DELETE };
	constexpr InputCode Left{ VK_LEFT };
	constexpr InputCode Right{ VK_RIGHT };
	constexpr InputCode Up{ VK_UP };
	constexpr InputCode Down{ VK_DOWN };
	constexpr InputCode Zero{ 0x30 };
	constexpr InputCode One{ 0x31 };
	constexpr InputCode Two{ 0x32 };
	constexpr InputCode Three{ 0x33 };
	constexpr InputCode Four{ 0x34 };
	constexpr InputCode Five{ 0x35 };
	constexpr InputCode Six{ 0x36 };
	constexpr InputCode Seven{ 0x37 };
	constexpr InputCode Eight{ 0x38 };
	constexpr InputCode Nine{ 0x39 };
	constexpr InputCode A{ 0x41 };
	constexpr InputCode B{ 0x42 };
	constexpr InputCode C{ 0x43 };
	constexpr InputCode D{ 0x44 };
	constexpr InputCode E{ 0x45 };
	constexpr InputCode F{ 0x46 };
	constexpr InputCode G{ 0x47 };
	constexpr InputCode H{ 0x48 };
	constexpr InputCode I{ 0x49 };
	constexpr InputCode J{ 0x4A };
	constexpr InputCode K{ 0x4B };
	constexpr InputCode L{ 0x4C };
	constexpr InputCode M{ 0x4D };
	constexpr InputCode N{ 0x4E };
	constexpr InputCode O{ 0x4F };
	constexpr InputCode P{ 0x50 };
	constexpr InputCode Q{ 0x51 };
	constexpr InputCode R{ 0x52 };
	constexpr InputCode S{ 0x53 };
	constexpr InputCode T{ 0x54 };
	constexpr InputCode U{ 0x55 };
	constexpr InputCode V{ 0x56 };
	constexpr InputCode W{ 0x57 };
	constexpr InputCode X{ 0x58 };
	constexpr InputCode Y{ 0x59 };
	constexpr InputCode Z{ 0x5A };
	constexpr InputCode Num_0{ VK_NUMPAD0 };
	constexpr InputCode Num_1{ VK_NUMPAD1 };
	constexpr InputCode Num_2{ VK_NUMPAD2 };
	constexpr InputCode Num_3{ VK_NUMPAD3 };
	constexpr InputCode Num_4{ VK_NUMPAD4 };
	constexpr InputCode Num_5{ VK_NUMPAD5 };
	constexpr InputCode Num_6{ VK_NUMPAD6 };
	constexpr InputCode Num_7{ VK_NUMPAD7 };
	constexpr InputCode Num_8{ VK_NUMPAD8 };
	constexpr InputCode Num_9{ VK_NUMPAD9 };
	constexpr InputCode F1{ VK_F1 };
	constexpr InputCode F2{ VK_F2 };
	constexpr InputCode F3{ VK_F3 };
	constexpr InputCode F4{ VK_F4 };
	constexpr InputCode F5{ VK_F5 };
	constexpr InputCode F6{ VK_F6 };
	constexpr InputCode F7{ VK_F7 };
	constexpr InputCode F8{ VK_F8 };
	constexpr InputCode F9{ VK_F9 };
	constexpr InputCode F10{ VK_F10 };
	constexpr InputCode F11{ VK_F11 };
	constexpr InputCode F12{ VK_F12 };
	constexpr InputCode Left_Ctrl{ VK_LCONTROL };
	constexpr InputCode Right_Ctrl{ VK_RCONTROL };
	constexpr InputCode Alt{ VK_MENU };
	constexpr InputCode Tilde{ 223 };

	constexpr InputCode Left_Mouse_Button{ MK_LBUTTON };
	constexpr InputCode Right_Mouse_Button{ MK_RBUTTON };
	constexpr InputCode Middle_Mouse_Button{ MK_MBUTTON };

	constexpr InputCode Mouse_Wheel_Up{ 399 };
	constexpr InputCode Mouse_Wheel_Down{ 400 };

	constexpr InputCode Mouse_X{ 401 };
	constexpr InputCode Mouse_Y{ 402 };
}