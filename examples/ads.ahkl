/*
    AHKUnix 0.5.7 compact demo.
    Shows Cancel, mixed command casing, if/else, Sleep, and comments.
*/

:*?:11::221B Baker Street, London
:*?:22::Sent with AHKUnix 0.5.7

F12::
Cancel
Return

Ctrl & F12::
Pause
Return

NumPad1::
SendInput, Quick ad line one.{Enter}
sendinput, Quick ad line two keeps its string casing.{Enter}
Return

NumPad2::
Random, variant, 1, 2
If (variant = 1) {
    SendInput, Variant ONE selected.{Enter}
    sleep, 250
    sendinput, First branch complete.{Enter}
} else {
    sEnDiNpUt, Variant TWO selected.{Enter}
    Sleep, 250
    SendInput, Second branch complete.{Enter}
}
Return
