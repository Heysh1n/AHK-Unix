/*
    Worker-thread and cancellation demo for AHKUnix 0.5.7.

    F6 starts a long AST command block.
    F12 cancels the currently running worker macro.
    Ctrl+F12 uses the Pause alias.
*/

F12::
Cancel
Return

Ctrl & F12::
Pause
Return

F6::
SendInput, Worker macro started.{Enter}
sleep, 1000
sendinput, Step 1 finished.{Enter}
Sleep, 1000
SendInput, Step 2 finished.{Enter}
sleep, 1000
Random, branch, 1, 2
If (branch = 1) {
    sEnDiNpUt, Random branch one.{Enter}
} else {
    SendInput, Random branch two.{Enter}
}
Sleep, 1000
sendinput, Worker macro finished without cancellation.{Enter}
Return
