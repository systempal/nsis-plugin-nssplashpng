; nsSplashPNG Example Script
; Demonstrates the usage of the nsSplashPNG plugin with various options

Name "nsSplashPNG Test"
OutFile "nsSplashPNG_Example.exe"
SilentInstall silent
RequestExecutionLevel user

!addplugindir "..\plugins\x86-unicode"

Section
    ; Copy splash image to temp directory
    SetOutPath $TEMP
    File "images\splash.png"
    
    ; Example 1: Simple splash with auto-close after 3 seconds
    ; nsSplashPNG::show 3000 "test_image.png"
    
    ; Example 2: Splash with fade in effect, auto-close after 3 seconds
    ; nsSplashPNG::show /FADEIN 3000 "test_image.png"
    
    ; Example 3: Splash with fade out effect, auto-close after 3 seconds
    ; nsSplashPNG::show /FADEOUT 3000 "test_image.png"
    
    ; Example 4: Splash with fade in, manual close (no auto-close)
    ; CRITICAL: /NOUNLOAD required since we call stop() later
    nsSplashPNG::show /NOUNLOAD 0 /FADEIN "$TEMP\splash.png"
    
    ; Simulate some installation work
    Sleep 2000
    
    ; Example 5: Manual close with fade out
    nsSplashPNG::stop /FADEOUT
    
    ; Clean up
    Delete "$TEMP\splash.png"
    
    MessageBox MB_OK "Installation complete!"
SectionEnd
