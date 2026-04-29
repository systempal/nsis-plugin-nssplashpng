; Test fade speed customization
Unicode true
!addplugindir "N:\Code\Workspace\Launchers\plugins\x86-unicode"

Name "Fade Speed Test"
OutFile "test_fade_speed.exe"
SilentInstall silent
RequestExecutionLevel user

Function .onInit
    SetOutPath $TEMP
    File "images\splash.png"
    
    ; Test 1: Default speed (step 15, ~510ms fade)
    MessageBox MB_OK "Test 1: Default fade speed (step 15)$\n$\nFade duration: ~510ms"
    nssplashpng::show /NOUNLOAD 60000 /FADEIN "$TEMP\splash.png"
    Pop $0
    Sleep 3000
    nssplashpng::stop /FADEOUT
    ; No need to sleep - stop now waits for fade-out internally
    
    ; Test 2: Fast fade (step 51, ~150ms fade)
    MessageBox MB_OK "Test 2: Fast fade (step 51)$\n$\nFade duration: ~150ms"
    nssplashpng::show /NOUNLOAD 60000 /FADEIN 51 "$TEMP\splash.png"
    Pop $0
    Sleep 3000
    nssplashpng::stop /FADEOUT 51
    ; No need to sleep - stop now waits for fade-out internally
    
    ; Test 3: Slow fade (step 5, ~1530ms fade)
    MessageBox MB_OK "Test 3: Slow fade (step 5)$\n$\nFade duration: ~1530ms"
    nssplashpng::show /NOUNLOAD 60000 /FADEIN 5 "$TEMP\splash.png"
    Pop $0
    Sleep 5000
    nssplashpng::stop /FADEOUT 5
    ; No need to sleep - stop now waits for fade-out internally
    
    ; Test 4: Instant (step 255, ~30ms fade)
    MessageBox MB_OK "Test 4: Instant fade (step 255)$\n$\nFade duration: ~30ms"
    nssplashpng::show /NOUNLOAD 60000 /FADEIN 255 "$TEMP\splash.png"
    Pop $0
    Sleep 2000
    nssplashpng::stop /FADEOUT 255
    ; No need to sleep - stop now waits for fade-out internally    Delete "$TEMP\splash.png"
    MessageBox MB_OK "All fade speed tests completed!"
    Quit
FunctionEnd

Section
SectionEnd
