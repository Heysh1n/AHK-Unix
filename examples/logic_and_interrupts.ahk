/*
 A H*KUnix 0.6.0: Logic, Worker Threads, and Interrupts
 Демонстрация независимых потоков макросов и экстренной отмены.
 */

; Глобальная кнопка отмены макроса (очистит очередь и отожмет все зависшие клавиши)
F12::
Cancel
Return

Ctrl & F12::
Pause
Return

; 1. Длинный макрос (выполняется в отдельном потоке, не блочит клавиатуру)
F6::
SendInput, Запускаю долгий процесс... (Нажми F12 для отмены){Enter}
Sleep, 2000
SendInput, Шаг 1 завершен.{Enter}
Sleep, 2000
SendInput, Шаг 2 завершен.{Enter}
Sleep, 2000
SendInput, Макрос успешно отработал без прерываний.{Enter}
Return

; 2. Рандом и ветвление (If / Else)
F7::
Random, branch, 1, 2
If (branch = 1) {
    SendInput, {~Сработала|Выпала} первая ветка.{Enter}
    Sleep, 500
    SendInput, Успех.{Enter}
} Else {
    SendInput, {~Сработала|Выпала} вторая ветка.{Enter}
    Sleep, 500
    SendInput, Провал.{Enter}
}
Return
