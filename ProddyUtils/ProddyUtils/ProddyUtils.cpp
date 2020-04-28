#include <windows.h>
#include <string>
#include <experimental/filesystem>
#include <sstream>
#include <chrono>
#include "lua.hpp"

#pragma region Version
static const int Major = 1;
static const int Minor = 4;
static const int Build = 0;

static int lua_checkversion(lua_State* L)
{
	auto major = luaL_checkinteger(L, 1);
	auto minor = luaL_optinteger(L, 2, 0);
	auto build = luaL_optinteger(L, 3, 0);
	if (Major > major)
		lua_pushboolean(L, true);
	else if (Major < major)
		lua_pushboolean(L, false);
	else
		if (Minor > minor)
			lua_pushboolean(L, true);
		else if (Minor < minor)
			lua_pushboolean(L, false);
		else
			if (Build >= build)
				lua_pushboolean(L, true);
			else
				lua_pushboolean(L, false);
	return 1;
}

static int lua_getversion(lua_State* L)
{
	lua_newtable(L);
	lua_pushinteger(L, 1);
	lua_pushinteger(L, Major);
	lua_settable(L, -3);
	lua_pushinteger(L, 2);
	lua_pushinteger(L, Minor);
	lua_settable(L, -3);
	lua_pushinteger(L, 3);
	lua_pushinteger(L, Build);
	lua_settable(L, -3);
	return 1;
}
#pragma endregion

#pragma region UTFUtils
#ifdef _WIN64
#pragma warning(push)
#pragma warning(disable : 4267)
#endif
std::string UTF16ToUTF8(const wchar_t* pszString, size_t Length)
{
	if (Length == 0)
		return "";
	int iLength = WideCharToMultiByte(CP_UTF8, 0, pszString, Length, nullptr, 0, nullptr, nullptr);
	if (iLength == 0)
		abort();
	char* pszString2 = new char[iLength];
	int iResult = WideCharToMultiByte(CP_UTF8, 0, pszString, Length, pszString2, iLength, nullptr, nullptr);
	if (iResult == 0)
	{
		delete[] pszString2;
		abort();
	}
	std::string strString(pszString2, iLength);
	delete[] pszString2;
	return strString;
}
#ifdef _WIN64
#pragma warning(pop)
#endif

inline std::string UTF16ToUTF8(const std::wstring& strString)
{
	return UTF16ToUTF8(strString.c_str(), strString.size());
}

#ifdef _WIN64
#pragma warning(push)
#pragma warning(disable : 4267)
#endif
std::wstring UTF8ToUTF16(const char* pszString, size_t Length)
{
	if (Length == 0)
		return L"";
	int iLength = MultiByteToWideChar(CP_UTF8, 0, pszString, Length, nullptr, 0);
	if (iLength == 0)
		abort();
	wchar_t* pszString2 = new wchar_t[iLength];
	int iResult = MultiByteToWideChar(CP_UTF8, 0, pszString, Length, pszString2, iLength);
	if (iResult == 0)
	{
		delete[] pszString2;
		abort();
	}
	std::wstring strString(pszString2, iLength);
	delete[] pszString2;
	return strString;
}
#ifdef _WIN64
#pragma warning(pop)
#endif
inline void lua_pushlstring(lua_State* L, const std::string& str)
{
	lua_pushlstring(L, str.c_str(), str.size());
}

inline void lua_pushlstring(lua_State* L, const std::wstring& str)
{
	lua_pushlstring(L, UTF16ToUTF8(str));
}
#pragma endregion

#pragma region Clipboard
bool SetClipboard(const std::wstring& str)
{
	if (!OpenClipboard(nullptr))
		return false;
	size_t Length = str.size() + 1;
	HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, Length * sizeof(wchar_t));
	if (hMem == nullptr)
	{
		CloseClipboard();
		return false;
	}
	void* pMem = GlobalLock(hMem);
	if (pMem == nullptr)
	{
		GlobalFree(hMem);
		CloseClipboard();
		return false;
	}
	memcpy(pMem, str.c_str(), Length * sizeof(wchar_t));
	if (!GlobalUnlock(hMem))
	{
		HRESULT hr = HRESULT_FROM_WIN32(GetLastError());
		if (FAILED(hr))
		{
			GlobalFree(hMem);
			CloseClipboard();
			return false;
		}
	}
	if (!EmptyClipboard())
	{
		GlobalFree(hMem);
		CloseClipboard();
		return false;
	}
	if (SetClipboardData(CF_UNICODETEXT, hMem) == nullptr)
	{
		GlobalFree(hMem);
		CloseClipboard();
		return false;
	}
	CloseClipboard();
	return true;
}

bool ClipboardContainsText(void)
{
	return !!IsClipboardFormatAvailable(CF_UNICODETEXT);
}

bool GetClipboardText(std::wstring* pstr)
{
	if (!OpenClipboard(nullptr))
		return false;
	if (!ClipboardContainsText())
	{
		CloseClipboard();
		return false;
	}
	HGLOBAL hMem = GetClipboardData(CF_UNICODETEXT);
	if (hMem == nullptr)
	{
		CloseClipboard();
		return false;
	}
	void* pMem = GlobalLock(hMem);
	if (pMem == nullptr)
	{
		CloseClipboard();
		return false;
	}
	*pstr = (wchar_t*)pMem;
	GlobalUnlock(hMem);
	CloseClipboard();
	return true;
}

static int lua_setclipboard(lua_State* L)
{
	size_t len;
	auto text = luaL_checklstring(L, 1, &len);
	lua_pushboolean(L, SetClipboard(UTF8ToUTF16(text, len)));
	return 1;
}

static int lua_getclipboard(lua_State* L)
{
	std::wstring str;
	if (GetClipboardText(&str)) {
		lua_pushlstring(L, str);
	}
	else {
		lua_pushnil(L);
	}
	return 1;
}
#pragma endregion

#pragma region MessageBox
static int lua_msgbox(lua_State* L)
{
	size_t msgLen;
	size_t capLen;
	auto message = luaL_checklstring(L, 1, &msgLen);
	auto caption = luaL_optlstring(L, 2, "2Take1Menu - ProddyUtils", &capLen);
	auto uType = MB_TASKMODAL;
	auto type = luaL_optinteger(L, 3, 1);
	switch (type) {
	default:
		uType = uType | MB_OK;
		break;
	case 2:
		uType = uType | MB_OKCANCEL;
		break;
	case 3:
		uType = uType | MB_RETRYCANCEL;
		break;
	case 4:
		uType = uType | MB_YESNO;
		break;
	case 5:
		uType = uType | MB_YESNOCANCEL;
		break;
	}
	int result = MessageBox(NULL, UTF8ToUTF16(message, msgLen).c_str(), UTF8ToUTF16(caption, capLen).c_str(), uType);
	switch (result) {
	default:
		lua_pushinteger(L, 1);
		break;
	case 2:
		lua_pushinteger(L, 2);
		break;
	case 4:
		lua_pushinteger(L, 3);
		break;
	case 6:
		lua_pushinteger(L, 4);
		break;
	case 7:
		lua_pushinteger(L, 5);
		break;
	}
	return 1;
}
#pragma endregion

#pragma region IO
bool Exists(const std::wstring& strPath, bool& isDir)
{
	DWORD dw = GetFileAttributesW(strPath.c_str());
	if (dw != INVALID_FILE_ATTRIBUTES)
	{
		auto bDirectory = (dw & FILE_ATTRIBUTE_DIRECTORY) != 0;
		isDir = bDirectory;
		return true;
	}
	else
		return false;
}
std::wstring GetExtension(const std::wstring& strName) {
	if (empty(strName))
		return L"";
	auto period = strName.find_last_of(L'.');
	if (period == std::wstring::npos)
		return L"";
	auto strExt = strName.substr(period);
	std::transform(strExt.begin(), strExt.end(), strExt.begin(), towlower);
	return strExt;
}

static int lua_createdirectory(lua_State* L)
{
	size_t len;
	auto text = luaL_checklstring(L, 1, &len);
	if (len == 0)
	{
		lua_pushboolean(L, false);
		return 1;
	}
	auto strPath = UTF8ToUTF16(text, len);
	auto isDir = false;
	if (Exists(strPath, isDir) && isDir)
	{
		lua_pushboolean(L, true);
		return 1;
	}
	lua_pushboolean(L, std::experimental::filesystem::create_directories(strPath));
	return 1;
}

static int lua_exists(lua_State* L)
{
	size_t len;
	auto text = luaL_checklstring(L, 1, &len);
	auto strPath = UTF8ToUTF16(text, len);
	auto bDirectory = false;
	auto bExists = Exists(strPath, bDirectory);
	lua_pushboolean(L, bExists);
	lua_pushboolean(L, bDirectory);
	return 2;
}

static int lua_fileexists(lua_State* L) {
	size_t len;
	auto text = luaL_checklstring(L, 1, &len);
	auto strPath = UTF8ToUTF16(text, len);
	auto bDirectory = false;
	auto bExists = Exists(strPath, bDirectory);
	lua_pushboolean(L, bExists && !bDirectory);
	return 1;
}

static int lua_direxists(lua_State* L) {
	size_t len;
	auto text = luaL_checklstring(L, 1, &len);
	auto strPath = UTF8ToUTF16(text, len);
	auto bDirectory = false;
	auto bExists = Exists(strPath, bDirectory);
	lua_pushboolean(L, bExists && bDirectory);
	return 1;
}

static int lua_iteratedirectory(lua_State* L)
{
	size_t len;
	auto text = luaL_checklstring(L, 1, &len);
	auto strPath = UTF8ToUTF16(text, len) + L"\\*";
	WIN32_FIND_DATAW FindFileData;
	HANDLE hFind = FindFirstFileW(strPath.c_str(), &FindFileData);
	auto bCompleted = true;
	if (hFind != INVALID_HANDLE_VALUE)
	{
		for (;;)
		{
			std::wstring strName = FindFileData.cFileName;
			if (strName != L".." && strName != L".")
			{
				auto bDirectory = (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;

				lua_pushvalue(L, 2);
				lua_pushlstring(L, strName);
				lua_pushboolean(L, bDirectory);
				lua_call(L, 2, 1);
				auto bResult = lua_toboolean(L, -1) != 0;
				lua_pop(L, 1);
				if (!bResult)
				{
					bCompleted = false;
					break;
				}
			}
			if (!FindNextFileW(hFind, &FindFileData))
				break;
		}
		FindClose(hFind);
	}
	lua_pushboolean(L, bCompleted);
	return 1;
}

static int lua_getfiles(lua_State* L) {
	size_t len;
	auto text = luaL_checklstring(L, 1, &len);
	auto strPath = UTF8ToUTF16(text, len) + L"\\*";
	auto size = lua_gettop(L);
	std::wstring* exts = nullptr;
	if (size > 0)
	{
		exts = new std::wstring[size];
		for (int i = 0; i < size; i++)
		{
			size_t eLen;
			auto ext = luaL_checklstring(L, i + 1, &eLen);
			auto strExt = UTF8ToUTF16(ext, eLen);
			std::transform(strExt.begin(), strExt.end(), strExt.begin(), towlower);
			exts[i] = strExt;
		}
	}
	WIN32_FIND_DATAW FindFileData;
	HANDLE hFind = FindFirstFileW(strPath.c_str(), &FindFileData);
	lua_newtable(L);
	auto i = 0;
	if (hFind != INVALID_HANDLE_VALUE)
	{
		for (;;)
		{
			std::wstring strName = FindFileData.cFileName;
			if ((FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0 && strName != L".." && strName != L"." && (size == 0 || std::find(exts, exts+size, GetExtension(strName)) != exts+size))
			{
				i++;
				lua_pushinteger(L, i);
				lua_pushlstring(L, strName);
				lua_settable(L, -3);
			}
			if (!FindNextFileW(hFind, &FindFileData))
				break;
		}
		FindClose(hFind);
	}
	delete[] exts;
	return 1;
}
#pragma endregion

#pragma region Keyboard
static int lua_iskeypressed(lua_State* L)
{
	auto size = lua_gettop(L);
	if (size == 0)
		return 0;
	for (int i = 0; i < size; i++) {
		int key = (int)luaL_checkinteger(L, i + 1);
		if (!(GetKeyState(key) & 0x8000))
		{
			lua_pushboolean(L, false);
			return 1;
		}
	}
	lua_pushboolean(L, true);
	return 1;
}
static int lua_keydown(lua_State* L)
{
	auto size = lua_gettop(L);
	if (size == 0)
		return 0;
	for (int i = 0; i < size; i++) {
		int key = (int)luaL_checkinteger(L, i + 1);
		INPUT ip;
		ip.type = INPUT_KEYBOARD;
		ip.ki.wScan = key;
		ip.ki.dwFlags = 0;
		ip.ki.time = 0;
		ip.ki.dwExtraInfo = 0;
		SendInput(1, &ip, sizeof(INPUT));
	}
	return 0;
}
static int lua_keyup(lua_State* L)
{
	auto size = lua_gettop(L);
	if (size == 0)
		return 0;
	for (int i = 0; i < size; i++) {
		int key = (int)luaL_checkinteger(L, i + 1);
		INPUT ip;
		ip.type = INPUT_KEYBOARD;
		ip.ki.wScan = key;
		ip.ki.dwFlags = KEYEVENTF_KEYUP;
		ip.ki.time = 0;
		ip.ki.dwExtraInfo = 0;
		SendInput(1, &ip, sizeof(INPUT));
	}
	return 0;
}
#pragma endregion

#pragma region Time
static int lua_gettimemillis(lua_State* L)
{
	lua_pushinteger(L, std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count());
	return 1;
}
static int lua_gettimemicro(lua_State* L)
{
	lua_pushinteger(L, std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count());
	return 1;
}
static int lua_gettimenano(lua_State* L)
{
	lua_pushinteger(L, std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count());
	return 1;
}
#pragma endregion

#pragma region LuaOpen
static const struct luaL_Reg ProddyUtils[] = {
	{"CheckVersion", lua_checkversion},
	{"GetVersion", lua_getversion},
	{NULL, NULL}
};
static const struct luaL_Reg Clipboard[] = {
	{"GetText", lua_getclipboard},
	{"SetText", lua_setclipboard},
	{NULL, NULL}
};
static const struct luaL_Reg Keyboard[] = {
	{"IsKeyPressed", lua_iskeypressed},
	{"KeyDown", lua_keydown},
	{"KeyUp", lua_keyup},
	{NULL, NULL}
};
static const struct luaL_Reg MsgBox[] = {
	{"Show", lua_msgbox},
	{NULL, NULL}
};
static const struct luaL_Reg IO[] = {
	{"CreateDirectory", lua_createdirectory},
	{"DirExists", lua_direxists},
	{"Exists", lua_exists},
	{"FileExists", lua_fileexists},
	{"GetFiles", lua_getfiles},
	{"IterateDirectory", lua_iteratedirectory},
	{NULL, NULL}
};
static const struct luaL_Reg OS[] = {
	{"GetTimeMillis", lua_gettimemillis},
	{"GetTimeMicro", lua_gettimemicro},
	{"GetTimeNano", lua_gettimenano},
	{NULL, NULL}
};

extern "C" __declspec(dllexport) int luaopen_ProddyUtils(lua_State * L)
{
	luaL_newlib(L, ProddyUtils);

	luaL_newlib(L, Clipboard);
	lua_setfield(L, -2, "Clipboard");

	luaL_newlib(L, IO);
	lua_setfield(L, -2, "IO");

	luaL_newlib(L, OS);
	lua_setfield(L, -2, "OS");

	luaL_newlib(L, Keyboard);
	lua_newtable(L);
	std::string keystring;
	char keybuffer;
	for (int i = 0; i < 256; ++i) {
		if (i == 3 || i == 8 || i == 9 || i == 13 || i == 27 || i == 32)
			continue;
		if (keybuffer = MapVirtualKey(UINT(i), 2)) {
			keystring += keybuffer;
			lua_pushinteger(L, i);
			lua_setfield(L, -2, keystring.c_str());
			keystring = "";
		}
	}
	lua_pushinteger(L, 0x1B);
	lua_setfield(L, -2, "Escape");
	lua_pushinteger(L, 0x70);
	lua_setfield(L, -2, "F1");
	lua_pushinteger(L, 0x71);
	lua_setfield(L, -2, "F2");
	lua_pushinteger(L, 0x72);
	lua_setfield(L, -2, "F3");
	lua_pushinteger(L, 0x73);
	lua_setfield(L, -2, "F4");
	lua_pushinteger(L, 0x74);
	lua_setfield(L, -2, "F5");
	lua_pushinteger(L, 0x75);
	lua_setfield(L, -2, "F6");
	lua_pushinteger(L, 0x76);
	lua_setfield(L, -2, "F7");
	lua_pushinteger(L, 0x77);
	lua_setfield(L, -2, "F8");
	lua_pushinteger(L, 0x78);
	lua_setfield(L, -2, "F9");
	lua_pushinteger(L, 0x79);
	lua_setfield(L, -2, "F10");
	lua_pushinteger(L, 0x7A);
	lua_setfield(L, -2, "F11");
	lua_pushinteger(L, 0x7B);
	lua_setfield(L, -2, "F12");
	lua_pushinteger(L, 0x2C);
	lua_setfield(L, -2, "PrintScreen");
	lua_pushinteger(L, 0x91);
	lua_setfield(L, -2, "ScrollLock");
	lua_pushinteger(L, 0x08);
	lua_setfield(L, -2, "Backspace");
	lua_pushinteger(L, 0x20);
	lua_setfield(L, -2, "Space");
	lua_pushinteger(L, 0x2D);
	lua_setfield(L, -2, "Insert");
	lua_pushinteger(L, 0x24);
	lua_setfield(L, -2, "Home");
	lua_pushinteger(L, 0x22);
	lua_setfield(L, -2, "PageDown");
	lua_pushinteger(L, 0x21);
	lua_setfield(L, -2, "PageUp");
	lua_pushinteger(L, 0x2E);
	lua_setfield(L, -2, "Delete");
	lua_pushinteger(L, 0x90);
	lua_setfield(L, -2, "Numlock");
	lua_pushinteger(L, 0x6F);
	lua_setfield(L, -2, "Numpad /");
	lua_pushinteger(L, 0x6A);
	lua_setfield(L, -2, "Numpad*");
	lua_pushinteger(L, 0x6D);
	lua_setfield(L, -2, "Numpad-");
	lua_pushinteger(L, 0x6B);
	lua_setfield(L, -2, "Numpad+");
	lua_pushinteger(L, 0x6E);
	lua_setfield(L, -2, "Numpad.");
	lua_pushinteger(L, 0x60);
	lua_setfield(L, -2, "Numpad0");
	lua_pushinteger(L, 0x61);
	lua_setfield(L, -2, "Numpad1");
	lua_pushinteger(L, 0x62);
	lua_setfield(L, -2, "Numpad2");
	lua_pushinteger(L, 0x63);
	lua_setfield(L, -2, "Numpad3");
	lua_pushinteger(L, 0x64);
	lua_setfield(L, -2, "Numpad4");
	lua_pushinteger(L, 0x65);
	lua_setfield(L, -2, "Numpad5");
	lua_pushinteger(L, 0x66);
	lua_setfield(L, -2, "Numpad6");
	lua_pushinteger(L, 0x67);
	lua_setfield(L, -2, "Numpad7");
	lua_pushinteger(L, 0x68);
	lua_setfield(L, -2, "Numpad8");
	lua_pushinteger(L, 0x68);
	lua_setfield(L, -2, "Numpad9");

	lua_pushinteger(L, 0x26);
	lua_setfield(L, -2, "ArrowUp");
	lua_pushinteger(L, 0x28);
	lua_setfield(L, -2, "ArrowDown");
	lua_pushinteger(L, 0x25);
	lua_setfield(L, -2, "ArrowLeft");
	lua_pushinteger(L, 0x27);
	lua_setfield(L, -2, "ArrowRight");

	lua_pushinteger(L, 0x0D);
	lua_setfield(L, -2, "Enter");
	lua_pushinteger(L, 0x10);
	lua_setfield(L, -2, "Shift");
	lua_pushinteger(L, 0xA0);
	lua_setfield(L, -2, "LShift");
	lua_pushinteger(L, 0xA1);
	lua_setfield(L, -2, "RShift");
	lua_pushinteger(L, 0x09);
	lua_setfield(L, -2, "Tab");
	lua_pushinteger(L, 0x14);
	lua_setfield(L, -2, "CapsLock");
	lua_pushinteger(L, 0x11);
	lua_setfield(L, -2, "Control");
	lua_pushinteger(L, 0xA2);
	lua_setfield(L, -2, "LControl");
	lua_pushinteger(L, 0xA3);
	lua_setfield(L, -2, "RControl");
	lua_pushinteger(L, 0x12);
	lua_setfield(L, -2, "Alt");
	lua_pushinteger(L, 0xA4);
	lua_setfield(L, -2, "LAlt");
	lua_pushinteger(L, 0xA5);
	lua_setfield(L, -2, "RAlt");
	lua_pushinteger(L, 0x5B);
	lua_setfield(L, -2, "LWin");
	lua_pushinteger(L, 0x5C);
	lua_setfield(L, -2, "RWin");
	lua_setfield(L, -2, "Keys");
	lua_newtable(L);
	lua_pushinteger(L, 1);
	lua_setfield(L, -2, "Escape");
	lua_pushinteger(L, 2);
	lua_setfield(L, -2, "1");
	lua_pushinteger(L, 3);
	lua_setfield(L, -2, "2");
	lua_pushinteger(L, 4);
	lua_setfield(L, -2, "3");
	lua_pushinteger(L, 6);
	lua_setfield(L, -2, "5");
	lua_pushinteger(L, 8);
	lua_setfield(L, -2, "7");
	lua_pushinteger(L, 10);
	lua_setfield(L, -2, "9");
	lua_pushinteger(L, 11);
	lua_setfield(L, -2, "0");
	lua_pushinteger(L, 12);
	lua_setfield(L, -2, "Minus");
	lua_pushinteger(L, 13);
	lua_setfield(L, -2, "Equals");
	lua_pushinteger(L, 14);
	lua_setfield(L, -2, "Backspace");
	lua_pushinteger(L, 15);
	lua_setfield(L, -2, "Tab");
	lua_pushinteger(L, 16);
	lua_setfield(L, -2, "Q");
	lua_pushinteger(L, 17);
	lua_setfield(L, -2, "W");
	lua_pushinteger(L, 18);
	lua_setfield(L, -2, "E");
	lua_pushinteger(L, 19);
	lua_setfield(L, -2, "R");
	lua_pushinteger(L, 20);
	lua_setfield(L, -2, "T");
	lua_pushinteger(L, 21);
	lua_setfield(L, -2, "Y");
	lua_pushinteger(L, 22);
	lua_setfield(L, -2, "U");
	lua_pushinteger(L, 23);
	lua_setfield(L, -2, "I");
	lua_pushinteger(L, 24);
	lua_setfield(L, -2, "O");
	lua_pushinteger(L, 25);
	lua_setfield(L, -2, "P");
	lua_pushinteger(L, 26);
	lua_setfield(L, -2, "LeftBracket");
	lua_pushinteger(L, 27);
	lua_setfield(L, -2, "RightBracket");
	lua_pushinteger(L, 28);
	lua_setfield(L, -2, "Enter");
	lua_pushinteger(L, 29);
	lua_setfield(L, -2, "LeftControl");
	lua_pushinteger(L, 30);
	lua_setfield(L, -2, "A");
	lua_pushinteger(L, 31);
	lua_setfield(L, -2, "S");
	lua_pushinteger(L, 32);
	lua_setfield(L, -2, "D");
	lua_pushinteger(L, 33);
	lua_setfield(L, -2, "F");
	lua_pushinteger(L, 34);
	lua_setfield(L, -2, "G");
	lua_pushinteger(L, 35);
	lua_setfield(L, -2, "H");
	lua_pushinteger(L, 36);
	lua_setfield(L, -2, "J");
	lua_pushinteger(L, 37);
	lua_setfield(L, -2, "K");
	lua_pushinteger(L, 38);
	lua_setfield(L, -2, "L");
	lua_pushinteger(L, 39);
	lua_setfield(L, -2, "Semicolon");
	lua_pushinteger(L, 40);
	lua_setfield(L, -2, "Apostrophe");
	lua_pushinteger(L, 41);
	lua_setfield(L, -2, "Tilde");
	lua_pushinteger(L, 42);
	lua_setfield(L, -2, "LeftShift");
	lua_pushinteger(L, 43);
	lua_setfield(L, -2, "BackSlash");
	lua_pushinteger(L, 44);
	lua_setfield(L, -2, "Z");
	lua_pushinteger(L, 45);
	lua_setfield(L, -2, "X");
	lua_pushinteger(L, 46);
	lua_setfield(L, -2, "C");
	lua_pushinteger(L, 47);
	lua_setfield(L, -2, "V");
	lua_pushinteger(L, 48);
	lua_setfield(L, -2, "B");
	lua_pushinteger(L, 49);
	lua_setfield(L, -2, "N");
	lua_pushinteger(L, 50);
	lua_setfield(L, -2, "M");
	lua_pushinteger(L, 51);
	lua_setfield(L, -2, "Comma");
	lua_pushinteger(L, 52);
	lua_setfield(L, -2, "Period");
	lua_pushinteger(L, 53);
	lua_setfield(L, -2, "ForwardSlash");
	lua_pushinteger(L, 54);
	lua_setfield(L, -2, "RightShift");
	lua_pushinteger(L, 55);
	lua_setfield(L, -2, "NumpadMultiply");
	lua_pushinteger(L, 56);
	lua_setfield(L, -2, "LeftAlt");
	lua_pushinteger(L, 57);
	lua_setfield(L, -2, "Spacebar");
	lua_pushinteger(L, 58);
	lua_setfield(L, -2, "CapsLock");
	lua_pushinteger(L, 59);
	lua_setfield(L, -2, "F1");
	lua_pushinteger(L, 60);
	lua_setfield(L, -2, "F2");
	lua_pushinteger(L, 61);
	lua_setfield(L, -2, "F3");
	lua_pushinteger(L, 62);
	lua_setfield(L, -2, "F4");
	lua_pushinteger(L, 63);
	lua_setfield(L, -2, "F5");
	lua_pushinteger(L, 64);
	lua_setfield(L, -2, "F6");
	lua_pushinteger(L, 65);
	lua_setfield(L, -2, "F7");
	lua_pushinteger(L, 66);
	lua_setfield(L, -2, "F8");
	lua_pushinteger(L, 67);
	lua_setfield(L, -2, "F9");
	lua_pushinteger(L, 68);
	lua_setfield(L, -2, "F10");
	lua_pushinteger(L, 69);
	lua_setfield(L, -2, "NumLock");
	lua_pushinteger(L, 70);
	lua_setfield(L, -2, "ScrollLock");
	lua_pushinteger(L, 71);
	lua_setfield(L, -2, "Numpad7");
	lua_pushinteger(L, 72);
	lua_setfield(L, -2, "Numpad8");
	lua_pushinteger(L, 73);
	lua_setfield(L, -2, "Numpad9");
	lua_pushinteger(L, 74);
	lua_setfield(L, -2, "NumpadMinus");
	lua_pushinteger(L, 75);
	lua_setfield(L, -2, "Numpad4");
	lua_pushinteger(L, 76);
	lua_setfield(L, -2, "Numpad5");
	lua_pushinteger(L, 77);
	lua_setfield(L, -2, "Numpad6");
	lua_pushinteger(L, 78);
	lua_setfield(L, -2, "NumpadPlus");
	lua_pushinteger(L, 79);
	lua_setfield(L, -2, "Numpad1");
	lua_pushinteger(L, 80);
	lua_setfield(L, -2, "Numpad2");
	lua_pushinteger(L, 81);
	lua_setfield(L, -2, "Numpad3");
	lua_pushinteger(L, 82);
	lua_setfield(L, -2, "Numpad0");
	lua_pushinteger(L, 83);
	lua_setfield(L, -2, "NumpadPeriod");
	lua_pushinteger(L, 87);
	lua_setfield(L, -2, "F11");
	lua_pushinteger(L, 88);
	lua_setfield(L, -2, "F12");
	lua_pushinteger(L, 156);
	lua_setfield(L, -2, "NumpadEnter");
	lua_pushinteger(L, 157);
	lua_setfield(L, -2, "RightControl");
	lua_pushinteger(L, 181);
	lua_setfield(L, -2, "NumpadSlash");
	lua_pushinteger(L, 184);
	lua_setfield(L, -2, "RightAlt");
	lua_pushinteger(L, 199);
	lua_setfield(L, -2, "Home");
	lua_pushinteger(L, 200);
	lua_setfield(L, -2, "UpArrow");
	lua_pushinteger(L, 201);
	lua_setfield(L, -2, "PageUp");
	lua_pushinteger(L, 203);
	lua_setfield(L, -2, "LeftArrow");
	lua_pushinteger(L, 205);
	lua_setfield(L, -2, "RightArrow");
	lua_pushinteger(L, 207);
	lua_setfield(L, -2, "End");
	lua_pushinteger(L, 208);
	lua_setfield(L, -2, "DownArrow");
	lua_pushinteger(L, 209);
	lua_setfield(L, -2, "PageDown");
	lua_pushinteger(L, 210);
	lua_setfield(L, -2, "Insert");
	lua_pushinteger(L, 211);
	lua_setfield(L, -2, "Delete");
	lua_setfield(L, -2, "DXKeys");
	lua_setfield(L, -2, "Keyboard");

	luaL_newlib(L, MsgBox);
	lua_newtable(L);
	lua_pushinteger(L, 1);
	lua_setfield(L, -2, "OK");
	lua_pushinteger(L, 2);
	lua_setfield(L, -2, "OKCancel");
	lua_pushinteger(L, 3);
	lua_setfield(L, -2, "RetryCancel");
	lua_pushinteger(L, 4);
	lua_setfield(L, -2, "YesNo");
	lua_pushinteger(L, 5);
	lua_setfield(L, -2, "YesNoCancel");
	lua_setfield(L, -2, "Buttons");
	lua_newtable(L);
	lua_pushinteger(L, 1);
	lua_setfield(L, -2, "OK");
	lua_pushinteger(L, 2);
	lua_setfield(L, -2, "Cancel");
	lua_pushinteger(L, 3);
	lua_setfield(L, -2, "Retry");
	lua_pushinteger(L, 4);
	lua_setfield(L, -2, "Yes");
	lua_pushinteger(L, 5);
	lua_setfield(L, -2, "No");
	lua_setfield(L, -2, "DialogResult");
	lua_setfield(L, -2, "MessageBox");

#if DEBUG
	lua_getglobal(L, "ui");
	lua_getfield(L, -1, "notify_above_map");
	std::stringstream ss;
	ss << "ProddyUtils v" << Major << "." << Minor << "." << Build << " loaded.";
	lua_pushstring(L, ss.str().c_str());
	lua_pushstring(L, "ProddyUtils");
	lua_pushinteger(L, 140);
	if (lua_pcall(L, 3, 0, 0) != 0) {
		lua_pop(L, 1);
	}
	lua_pop(L, 1);
#endif

	return 1;
}
#pragma endregion