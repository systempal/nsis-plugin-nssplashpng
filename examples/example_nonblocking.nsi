; Test non-blocking behavior
Unicode true
!addplugindir "N:\Code\Workspace\Launchers\plugins\x86-unicode"

Name "Non-Blocking Test"
OutFile "test_nonblocking.exe"
SilentInstall silent
RequestExecutionLevel user

Function .onInit
    SetOutPath $TEMP
    File "images\splash.png"
    
    ; Show splash with fade-in, manual close (0 timeout)
    ; CRITICAL: /NOUNLOAD required since we call stop() later
    MessageBox MB_OK "Showing splash with fade-in. Script should continue immediately after fade-in completes."
    nssplashpng::show /NOUNLOAD 0 /FADEIN "$TEMP\splash.png"
    Pop $0
    
    ; This should appear right after fade-in, while splash is still visible
    MessageBox MB_OK "Splash is visible! Result: $0$\n$\nNow doing some work..."
    Sleep 2000
    
    MessageBox MB_OK "Work done. Now closing splash with fade-out..."
    nssplashpng::stop /FADEOUT
    Pop $0
    
    MessageBox MB_OK "Splash closed! Stop result: $0"
    Delete "$TEMP\splash.png"
    Quit
FunctionEnd

Section
SectionEnd
