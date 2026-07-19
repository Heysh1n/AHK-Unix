/*
    AHKUnix 0.6.0: Markdown Overlay Integration
    Demonstration of displaying graphical cheat sheets on top of windows.

    Requirements:
    Python 3 and the md_overlay.py script located at /home/heysh1n/scripts/md_overlay.py
*/

; Status flags for windows (0 – closed, 1 – open)
Toggle_UK = 0
Toggle_KOAP = 0

; ------------------------------------------------- --------
; F2: Show / Hide the Criminal Code
; ---------------------------------------------------------
F2::
If (Toggle_UK = 0) {
    Toggle_UK = 1
    ; The command will kill old windows itself before launching a new one
    showMarkDown, /home/heysh1n/laws_uk.md
} Else {
    Toggle_UK = 0
    ; Workaround for closing without the built-in HideMarkdown command
    ; Run an empty command (or the pkill command directly, if you add Run)
    Run, pkill -f md_overlay.py
}
Return

; ----------------------------------- ----------------------
; F3: Show / Hide the Code of Administrative Offences
; ---------------------------------------------------------
F3::
If (Toggle_KOAP = 0) {
    Toggle_KOAP = 1
    showMarkDown, /home/heysh1n/laws_koap.md
} Else {
    Toggle_KOAP = 0
    Run, pkill -f md_overlay.py
}
Return

Translated with DeepL.com (free version)