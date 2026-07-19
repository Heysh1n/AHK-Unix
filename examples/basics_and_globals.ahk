/*
 A H*KUnix 0.6.0: Basics, Globals, and Time Interpolation
 Демонстрация базовых фич парсера.
 */

; --- ГЛОБАЛЬНЫЕ ПЕРЕМЕННЫЕ ---
; Объявляются в начале файла. Парсер сам уберет лишние пробелы.
Username = Heysh1n
Role = Student
System = Debian Linux

; 1. Базовая подстановка переменных и времени
:*?:/info::
SendInput, User: %Username% | Role: %Role% | OS: %System%{Enter}
SendInput, Current time: %A_Hour%:%A_Min%:%A_Sec%{Enter}
Return

; 2. Математика со временем (вычисление сдвигов на лету)
:*?:/shift::
SendInput, Начало смены в %A_Hour%:%A_Min%.{Enter}
; Парсер автоматически посчитает (%A_Hour%+8) и вставит готовое число
SendInput, Конец смены через 8 часов в (%A_Hour%+8):%A_Min%.{Enter}
Return
