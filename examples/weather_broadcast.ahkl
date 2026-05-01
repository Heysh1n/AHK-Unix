:*?:погода1::
SendMessage, 0x50,, 0x4190419,, A
SendInput, {Esc}
Sleep 1000
Random, announceType, 1, 3
if (announceType = 1) {
SendInput {F6}/r [*Тег*] Внимание, занимаю эфир для прогноза погоды. {Enter}
} else if (announceType = 2) {
SendInput {F6}/r [*Тег*] Занял эфир на метеосводку. {Enter}
} else {
SendInput {F6}/r [*Тег*] Эфир занимаю - передаём прогноз погоды. {Enter}
}
Sleep 2000
SendInput {F6}/efir {Enter}
Sleep 1500
SendInput {F6}/t ...::: Музыкальная заставка ТРК "Ритм" :::... {Enter}
Sleep 2000

Random, greetingType, 1, 4
if (greetingType = 1) {
SendInput {F6}/t Добрый день, жители Нижегородской области{!} {Enter}
} else if (greetingType = 2) {
SendInput {F6}/t Здравствуйте, дорогие зрители и слушатели{!} {Enter}
} else if (greetingType = 3) {
SendInput {F6}/t Приветствуем вас в эфире ТРК "Ритм"{!} {Enter}
} else {
SendInput {F6}/t Доброго времени суток. С вами метеосводка. {Enter}
}
Sleep 2000

Random, hostIntro, 1, 3
if (hostIntro = 1) {
SendInput {F6}/t У микрофона ваш *Должность* - *Имя_Фамилия*. {Enter}
} else {
SendInput {F6}/t С вами как всегда - *Должность* *Имя_Фамилия*. {Enter}
}
Sleep 2000

Random, forecastIntro, 1, 3
if (forecastIntro = 1) {
SendInput {F6}/t Итак, актуальный прогноз на сегодняшний день. {Enter}
} else if (forecastIntro = 2) {
SendInput {F6}/t Передаём сводку погоды по региону. {Enter}
} else {
SendInput {F6}/t Сейчас озвучим метеорологическую ситуацию. {Enter}
}
Sleep 2000

Random, yuzhnyWeather, 1, 3
if (yuzhnyWeather = 1) {
SendInput {F6}/t В Южном сегодня около +5°, переменная облачность. {Enter}
} else if (yuzhnyWeather = 2) {
SendInput {F6}/t Город Южный: +5°, солнце сменяется облаками. {Enter}
} else {
SendInput {F6}/t Южный: термометры показывают +5°, слабый ветер. {Enter}
}
Sleep 2000

Random, batyrevoWeather, 1, 3
if (batyrevoWeather = 1) {
SendInput {F6}/t В Батырево 0°, чувствуется лёгкий морозец. {Enter}
} else if (batyrevoWeather = 2) {
SendInput {F6}/t Посёлок Батырево: 0°, без осадков. {Enter}
} else {
SendInput {F6}/t Батырево встречает нас нулевой температурой. {Enter}
}
Sleep 2000

Random, generalWeather, 1, 3
if (generalWeather = 1) {
SendInput {F6}/t По региону: солнечно, но возможны отдельные облака. {Enter}
} else if (generalWeather = 2) {
SendInput {F6}/t Общая картина: ясно, временами переменная облачность. {Enter}
} else {
SendInput {F6}/t На большей части области преобладает ясная погода. {Enter}
}
Sleep 2000

Random, nizhWeather, 1, 3
if (nizhWeather = 1) {
SendInput {F6}/t В Нижегородске -3°, одевайтесь теплее. {Enter}
} else if (nizhWeather = 2) {
SendInput {F6}/t Нижегородск: -3°, возможен лёгкий ветер. {Enter}
} else {
SendInput {F6}/t Столица региона встречает нас -3°, без осадков. {Enter}
}
Sleep 2000

Random, endingType, 1, 2
if (endingType = 1) {
SendInput {F6}/t На этом наш прогноз завершён. Хорошего дня{!} {Enter}
} else {
SendInput {F6}/t Это была актуальная метеосводка. До свидания{!} {Enter}
}
Sleep 2000
SendInput {F6}/t ...::: Музыкальная заставка ТРК "Ритм" :::... {Enter}
Sleep 4000
SendInput {F6}/efir {Enter}
Sleep 4000
Return