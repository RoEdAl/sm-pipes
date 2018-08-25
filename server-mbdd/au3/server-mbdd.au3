#
# SMSrvMbdd Host
#
#NoTrayIcon

#include <ButtonConstants.au3>
#include <GUIConstantsEx.au3>
#include <ListViewConstants.au3>
#include <WindowsConstants.au3>
#include <GuiListView.au3>

Dim Const $DLL_NAME = "SMServerMbdd.dll"
Dim Const $APP_TITLE = "SMSrvMbdd Host [" & (@AutoItX64 ? "64bit]" : "32bit]")

Dim Const $SM_SRV_CMD_URL = 1

Dim Const $SM_SRV_STRUCT = "struct;int cmd;int structSize;endstruct"
Dim Const $SM_SRV_URL_STRUCT = "struct;int cmd;int structSize;endstruct;int urlOffset;int urlSize"

Dim $handler = 0
Dim $srvHandle = 0
Dim $doReply = True
Dim $dll = DllOpen($DLL_NAME)

If $dll = -1 Then
	MsgBox(0, $APP_TITLE, "Could not load SMSrvMbdd.dll.")
	Exit 1
EndIf

$MainForm = GUICreate($APP_TITLE, 604, 264, 192, 124, BitOR($GUI_SS_DEFAULT_GUI, $WS_MAXIMIZEBOX, $WS_SIZEBOX, $WS_THICKFRAME, $WS_TABSTOP))
$MsgListView = GUICtrlCreateListView("Field|Value", 8, 8, 585, 217)
GUICtrlSendMsg(-1, $LVM_SETCOLUMNWIDTH, 0, 120)
GUICtrlSendMsg(-1, $LVM_SETCOLUMNWIDTH, 1, 460)
GUICtrlSetResizing(-1, $GUI_DOCKLEFT + $GUI_DOCKRIGHT + $GUI_DOCKTOP + $GUI_DOCKBOTTOM)
$ButtonRegister = GUICtrlCreateButton("&Register", 8, 232, 73, 25)
GUICtrlSetResizing(-1, $GUI_DOCKLEFT + $GUI_DOCKBOTTOM + $GUI_DOCKWIDTH + $GUI_DOCKHEIGHT)
$ButtonUnregister = GUICtrlCreateButton("&UnRegister", 88, 232, 73, 25)
GUICtrlSetResizing(-1, $GUI_DOCKLEFT + $GUI_DOCKBOTTOM + $GUI_DOCKWIDTH + $GUI_DOCKHEIGHT)
$CheckReply = GUICtrlCreateCheckbox("Re&ply", 170, 232, 60, 25)
GUICtrlSetResizing(-1, $GUI_DOCKLEFT + $GUI_DOCKBOTTOM + $GUI_DOCKWIDTH + $GUI_DOCKHEIGHT)
$ButtonClear = GUICtrlCreateButton("&Clear", 520, 232, 73, 25)
GUICtrlSetResizing(-1, $GUI_DOCKRIGHT + $GUI_DOCKBOTTOM + $GUI_DOCKWIDTH + $GUI_DOCKHEIGHT)
$DummyUrlMsg = GUICtrlCreateDummy()

Func add_msg($field, $val)
	Local $idx = _GUICtrlListView_AddItem($MsgListView, $field)
	_GUICtrlListView_AddSubItem($MsgListView, $idx, $val, 1)
	_GUICtrlListView_SetItemSelected($MsgListView, $idx)
	_GUICtrlListView_EnsureVisible($MsgListView, $idx)
EndFunc   ;==>add_msg

Dim $apiLevel = DllCall($dll, "int", "SMSrvApiLevel")
If @error Then
	MsgBox(0, $APP_TITLE, "Could not obtain API level.")
EndIf

add_msg($DLL_NAME, FileGetVersion($DLL_NAME))
add_msg("SMSrvApiLevel", $apiLevel[0])

update_buttons_state(False)
GUICtrlSetState($CheckReply, $doReply ? $GUI_CHECKED : $GUI_UNCHECKED)
GUISetIcon($DLL_NAME)
GUISetState(@SW_SHOW)

Func get_wide_string($p, $offset, $size)
	Local $r = DllCall($dll, _
			"int", "SMSrvGetWideString", _
			"ptr", $p, _
			"int", $offset, _
			"int", $size, _
			"ptr", 0, _
			"int", 0)

	If @error Then Return ""

	If $r[0] <= 0 Then Return ""
	Local $bufLen = $r[0] + 1
	Local $WCharArrayDesc = StringFormat("wchar val[%d]", $bufLen)
	Local $bufStruct = DllStructCreate($WCharArrayDesc)

	$r = DllCall($dll, _
			"int", "SMSrvGetWideString", _
			"ptr", $p, _
			"int", $offset, _
			"int", $size, _
			"ptr", DllStructGetPtr($bufStruct), _
			"int", $bufLen )
	If Not @error Then
		Local $res = DllStructGetData($bufStruct, "val")
		Return $res
	EndIf

	Return ""
EndFunc   ;==>get_wide_string

Volatile Func my_primitive_handler($p)
	Local $srvStruct = DllStructCreate($SM_SRV_STRUCT, $p)
	Local $cmd = DllStructGetData($srvStruct, "cmd")
	ConsoleWriteError($cmd & @CRLF)

	Switch $cmd
		Case $SM_SRV_CMD_URL
			Local $srvUrlStruct = DllStructCreate($SM_SRV_URL_STRUCT, $p)
			Local $urlOffset = DllStructGetData($srvUrlStruct, "urlOffset")
			Local $urlSize = DllStructGetData($srvUrlStruct, "urlSize")
			Local $url = get_wide_string($p, $urlOffset, $urlSize)
			If StringLen($url) > 0 Then
				GUICtrlSendToDummy($DummyUrlMsg, $url)
			EndIf
	EndSwitch
	Return $doReply ? 0 : -2
EndFunc   ;==>my_primitive_handler

Func update_buttons_state($connected)
	If $connected Then
		GUICtrlSetState($ButtonUnregister, BitOR($GUI_ENABLE, $GUI_FOCUS))
		GUICtrlSetState($ButtonRegister, $GUI_DISABLE)
	Else
		GUICtrlSetState($ButtonRegister, BitOR($GUI_ENABLE, $GUI_FOCUS))
		GUICtrlSetState($ButtonUnregister, $GUI_DISABLE)
	EndIf
EndFunc   ;==>update_buttons_state

Func register()
	If $handler Then Return

	$handler = DllCallbackRegister("my_primitive_handler", "int", "ptr")

	If $handler = 0 Then
		MsgBox(0, $APP_TITLE, "Could not create callback.", 0, $MainForm)
	EndIf

	Local $r = DllCall($dll, "ptr", "SMSrvRegister", "ptr", DllCallbackGetPtr($handler))
	If @error Then
		MsgBox(0, $APP_TITLE, "Could not register callback.", 0, $MainForm)
	Else
		$srvHandle = $r[0]
		update_buttons_state(True)
		add_msg("SMSrvRegister", $srvHandle)
	EndIf
EndFunc   ;==>register

Func unregister()
	If Not $handler Then Return

	Local $u = DllCall($dll, "int", "SMSrvUnRegister", "ptr", $srvHandle)
	If @error Then
		MsgBox(0, $APP_TITLE, "Could not unregister callback.", 0, $MainForm)
	Else
		update_buttons_state(False)
		add_msg("SMSrvUnRegister", $u[0])
		DllCallbackFree($handler)
		$handler = 0
		$srvHandle = 0
	EndIf
EndFunc   ;==>unregister

While True
	$nMsg = GUIGetMsg()
	Switch $nMsg
		Case $ButtonRegister
			register()

		Case $ButtonUnregister
			unregister()

		Case $CheckReply
			$doReply = (GUICtrlRead($CheckReply) = $GUI_CHECKED) ? True : False

		Case $ButtonClear
			_GUICtrlListView_DeleteAllItems($MsgListView)

		Case $DummyUrlMsg
			add_msg("URL", GUICtrlRead($DummyUrlMsg))

		Case $GUI_EVENT_CLOSE
			ExitLoop
	EndSwitch
WEnd

unregister()
DllClose($dll)
