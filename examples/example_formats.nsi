; Test different image formats with nsSplashPNG
Unicode true

Name "Format Test"
OutFile "test_formats.exe"
InstallDir "$TEMP\test"

!addplugindir "N:\Code\Workspace\Launchers\plugins\x86-unicode"

Section
    SetOutPath $TEMP
    File "images\splash.png"
    File "images\splash.jpg"
    File "images\splash.bmp"
    File "images\splash.gif"
    
    ; Test PNG (with alpha)
    MessageBox MB_OK "Testing PNG format (with alpha transparency)"
    nssplashpng::show /NOUNLOAD 2000 /FADEIN /FADEOUT "$TEMP\splash.png"
    Pop $0
    
    ; Test JPEG (no alpha)
    MessageBox MB_OK "Testing JPEG format (no transparency)"
    nssplashpng::show /NOUNLOAD 2000 /FADEIN /FADEOUT "$TEMP\splash.jpg"
    Pop $0
    
    ; Test BMP (no alpha)
    MessageBox MB_OK "Testing BMP format (no transparency)"
    nssplashpng::show /NOUNLOAD 2000 /FADEIN /FADEOUT "$TEMP\splash.bmp"
    Pop $0
    
    ; Test GIF (with transparency)
    MessageBox MB_OK "Testing GIF format (basic transparency)"
    nssplashpng::show /NOUNLOAD 2000 /FADEIN /FADEOUT "$TEMP\splash.gif"
    Pop $0
    
    Delete "$TEMP\splash.png"
    Delete "$TEMP\splash.jpg"
    Delete "$TEMP\splash.bmp"
    Delete "$TEMP\splash.gif"
    
    MessageBox MB_OK "All format tests completed!"
SectionEnd
