if not ProddyUtils then
	if not package.cpath:find(os.getenv("APPDATA") .. "\\PopstarDevs\\2Take1Menu\\scripts\\lib\\?.dll", 1, true) then
		package.cpath = package.cpath .. ";" .. os.getenv("APPDATA") .. "\\PopstarDevs\\2Take1Menu\\scripts\\lib\\?.dll"
	end
	ProddyUtils = require("ProddyUtils")
end
if not (ProddyUtils.CheckVersion and ProddyUtils.CheckVersion(1, 3)) then -- Check function exists (added in 1.3), then call function with minimum required version
	local ver = ProddyUtils.GetVersion and "v" .. table.concat(ProddyUtils.GetVersion(), ".") or "Unknown" -- Get current version or Unknown (added in 1.3)
	-- Some error handling
	return -- Exit because wrong version
end

-- ProddyUtils.Clipboard
local ClipboardText = ProddyUtils.Clipboard.GetText()
if ClipboardText ~= nil then
	-- Do something with the text.
else
	-- Clipboard was either empty, or not text (IE: image).
end

local SomeText = "Something to save"
if ProddyUtils.Clipboard.SetText(SomeText) then
	-- Set clipboard text successfully.
else
	-- There was some error setting the text.
end

-- ProddyUtils.MessageBox
if ProddyUtils.MessageBox.Show("Do you like pie?", "Important question", ProddyUtils.MessageBox.Buttons.YesNo) == ProddyUtils.MessageBox.DialogResult.Yes then
	-- User likes pie.
else
	-- User doesn't like pie.
end
--[[ You do not need to include this in your script, but these are the contents of the two tables (enums):
	ProddyUtils.MessageBox.Buttons = {
		["OK"] = 1,
		["OKCancel"] = 2,
		["RetryCancel"] = 3,
		["YesNo"] = 4,
		["YesNoCancel"] = 5
	}
	ProddyUtils.MessageBox.DialogResult = {
		["OK"] = 1,
		["Cancel"] = 2,
		["Retry"] = 3,
		["Yes"] = 4,
		["No"] = 5
	}
]]

-- ProddyUtils.IO
if ProddyUtils.IO.CreateDirectory(os.getenv("APPDATA") .. "\\PopstarDevs\\2Take1Menu\\scripts\\lib\\ProddyUtils") then
	-- Directory already exists or was created successfully.
else
	-- Directory couldn't be created. Probably invalid path or permissions.
end

if ProddyUtils.IO.DirExists(os.getenv("APPDATA") .. "\\PopstarDevs\\2Take1Menu\\scripts\\lib\\ProddyUtils") then
	-- Exists and is directory
end

local Exists, IsDir = ProddyUtils.IO.Exists(os.getenv("APPDATA") .. "\\PopstarDevs\\2Take1Menu\\scripts\\lib\\ProddyUtils")
if Exists then
	if IsDir then
		-- Exists and is directory.
	else
		-- Exists and is file.
	end
else
	-- Directory or file doesn't exist on user's system.
end

if ProddyUtils.IO.DirExists(os.getenv("APPDATA") .. "\\PopstarDevs\\2Take1Menu\\scripts\\lib\\ProddyUtils.dll") then
	-- Exists and is file
end

local files = ProddyUtils.IO.GetFiles(os.getenv("APPDATA") .. "\\PopstarDevs\\2Take1Menu\\scripts\\lib")
-- All files in top level directory
local luaAndDLLFiles = ProddyUtils.IO.GetFiles(os.getenv("APPDATA") .. "\\PopstarDevs\\2Take1Menu\\scripts\\lib", ".lua", ".dll")
-- All Lua and DLL files in top level directory

local IterateCompleted = ProddyUtils.IO.IterateDirectory(os.getenv("APPDATA") .. "\\PopstarDevs\\2Take1Menu\\scripts", function(Name, IsDir)
	-- Returns item name (not full path), and if it's a directory.
	-- Return true to continue iteration, false to exit
	return true
end)
if IterateCompleted then
	-- Iteration completed without a return false
else
	-- Iteration was returned false
end