/*
 A H*KUnix 0.6.0: Spintax and Persistent Counters
 Демонстрация работы с файловой системой и рандомизацией строк.
 */

; 1. Спинтакс (рандомизация фраз при каждом вызове)
:*?:/greet::
SendInput, {F6}{~Привет|Здравствуйте|Доброго времени суток}, добро пожаловать на сервер!{Enter}
Return

; 2. Персистентный счетчик (значение переживает рестарт демона)
:*?:/report::
; Команда инкремента обновляет файл /tmp/ahkunix_var_ReportNum.txt
ReportNum++
SendInput, {F6}/me открыл дело №%ReportNum%{Enter}
Sleep, 500
SendInput, {F6}/do В деле №%ReportNum% записано нарушение.{Enter}
Return

; 3. Ультимативное комбо (Спинтакс + Счетчик + Время)
:*?:/combo::
ActionCount++
SendInput, {~Выполнено|Завершено|Сделано} действие №%ActionCount% в %A_Hour%:%A_Min%.{Enter}
Return
