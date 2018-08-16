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
Dim Const $INT_TYPE = @AutoItX64 ? "int64" : "int"

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

Dim $apiLevel = DllCall($dll, $INT_TYPE, "SMSrvApiLevel")
If @error Then
	MsgBox(0, $APP_TITLE, "Could not obtain API level.")
EndIf

add_msg($DLL_NAME, FileGetVersion($DLL_NAME))
add_msg("SMSrvApiLevel", $apiLevel[0])

update_buttons_state(False)
GUICtrlSetState($CheckReply, $doReply ? $GUI_CHECKED : $GUI_UNCHECKED)
GUISetState(@SW_SHOW)

Func my_primitive_handler($cmd, $val)
	If $cmd = 1 Then
		GUICtrlSendToDummy($DummyUrlMsg, $val)
	EndIf
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

	$handler = DllCallbackRegister("my_primitive_handler", $INT_TYPE, $INT_TYPE & ";wstr")

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

	Local $u = DllCall($dll, $INT_TYPE, "SMSrvUnRegister", "ptr", $srvHandle)
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
