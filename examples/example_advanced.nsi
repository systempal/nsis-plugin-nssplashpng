; nsSplashPNG Advanced Example
; Shows how to use nsSplashPNG in a real installer with progress updates

Name "nsSplashPNG Advanced Test"
OutFile "nsSplashPNG_Advanced.exe"
InstallDir "$PROGRAMFILES\TestApp"
RequestExecutionLevel admin

!addplugindir "..\plugins\x86-unicode"

Page instfiles

Section "Main Installation"
    ; Copy splash image to temp directory
    SetOutPath $TEMP
    File "images\splash.png"
    
    ; Show splash with fade in, no cancel button
    ; CRITICAL: /NOUNLOAD required since we call stop() later
    nsSplashPNG::show /NOUNLOAD 0 /NOCANCEL /FADEIN "$TEMP\splash.png"
    
    ; Simulate installation steps
    DetailPrint "Installing files..."
    SetOutPath "$INSTDIR"
    Sleep 1000
    
    DetailPrint "Configuring application..."
    Sleep 1000
    
    DetailPrint "Creating shortcuts..."
    Sleep 1000
    
    DetailPrint "Finalizing installation..."
    Sleep 1000
    
    ; Close splash with fade out
    nsSplashPNG::stop /FADEOUT
    
    ; Clean up
    Delete "$TEMP\splash.png"
    
    DetailPrint "Installation complete!"
SectionEnd

Section "Uninstall"
    RMDir /r "$INSTDIR"
SectionEnd
