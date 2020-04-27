#include <windows.h>
#include <string>
#include <experimental/filesystem>
#include "lua.hpp"

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
bool exists(const std::wstring& strPath, bool& isDir)
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
	if (exists(strPath, isDir) && isDir)
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
	auto bExists = exists(strPath, bDirectory);
	lua_pushboolean(L, bExists);
	lua_pushboolean(L, bDirectory);
	return 2;
}

static int lua_fileexists(lua_State* L) {
	size_t len;
	auto text = luaL_checklstring(L, 1, &len);
	auto strPath = UTF8ToUTF16(text, len);
	auto bDirectory = false;
	auto bExists = exists(strPath, bDirectory);
	lua_pushboolean(L, bExists && !bDirectory);
	return 1;
}

static int lua_direxists(lua_State* L) {
	size_t len;
	auto text = luaL_checklstring(L, 1, &len);
	auto strPath = UTF8ToUTF16(text, len);
	auto bDirectory = false;
	auto bExists = exists(strPath, bDirectory);
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
#pragma endregion

#pragma region LuaOpen
static const struct luaL_Reg Clipboard[] = {
	{"GetText", lua_getclipboard},
	{"SetText", lua_setclipboard},
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
	{"IterateDirectory", lua_iteratedirectory},
	{NULL, NULL}
};

extern "C" __declspec(dllexport) int luaopen_ProddyUtils(lua_State * L)
{
	lua_newtable(L);
	luaL_newlib(L, Clipboard);
	lua_setfield(L, -2, "Clipboard");
	luaL_newlib(L, IO);
	lua_setfield(L, -2, "IO");
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
	return 1;
}
#pragma endregion