/*
    AHKUnix 0.5.7 long macro demo.
    Press F6 to start the broadcast script.
    Press F12 to interrupt it with Cancel.
*/

F12::
Cancel
Return

F6::
SendInput, {Esc}
sleep, 500
Random, greetingType, 1, 3

If (greetingType = 1) {
    SendInput, Weather desk online. Good afternoon.{Enter}
} else {
    sendinput, Weather desk online. Welcome back.{Enter}
}

Sleep, 750
Random, forecastType, 1, 3

If (forecastType = 1) {
    sEnDiNpUt, Forecast: clear sky, light wind, stable pressure.{Enter}
} else {
    SendInput, Forecast: variable clouds and short sunny breaks.{Enter}
}

sleep, 750
Random, outroType, 1, 2

If (outroType = 1) {
    sendinput, That was the current weather update.{Enter}
} else {
    SendInput, Stay tuned for the next forecast.{Enter}
}

Sleep, 500
SendInput, Broadcast macro complete.{Enter}
Return
