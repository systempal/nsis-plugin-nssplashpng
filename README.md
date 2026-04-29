# nsSplashPNG - PNG Splash Screen Plugin for NSIS

A modern NSIS plugin that displays splash screens with full alpha transparency support using WIC (Windows Imaging Component).

## Features

- **Multiple Image Formats**: PNG, JPEG, BMP, GIF, TIFF, ICO via WIC decoder
- **Full Alpha Transparency**: Proper PNG/GIF alpha channel support via WIC and layered windows
- **Configurable Fade Speed**: Customizable fade in/out speed (1-255 steps, 30ms-7650ms duration)
- **Non-Blocking Execution**: Script continues after fade-in while splash stays visible
- **Auto-Close Timer**: Configurable automatic close after specified milliseconds
- **Click-to-Close**: Optional user interaction to close splash
- **No Cancel Option**: Prevent users from closing splash during critical operations
- **Manual Control**: Programmatic show/stop with fade effects
- **Multi-Monitor Support**: Center splash on primary, current (mouse), specific index, or nearest to a screen coordinate
- **Zero Warnings**: Clean compilation with Visual Studio 2022

## Installation

1. Copy `nssplashpng.dll` to your NSIS plugins directory:
   - `x86-unicode`: For 32-bit NSIS Unicode builds
   - `x64-unicode`: For 64-bit NSIS Unicode builds
   - `x86-ansi`: For 32-bit NSIS ANSI builds

2. Include the plugin directory in your script:
   ```nsis
   !addplugindir "path\to\plugins\x86-unicode"
   ```

## Building from Source

### Prerequisites
- Visual Studio 2022 or later with C++ build tools
- Python 3.x

### Build Steps
```powershell
cd nsSplashPNG
python build_plugin.py                            # Build all architectures
python build_plugin.py --config x86-unicode       # Single architecture (x86-ansi|x86-unicode|x64-unicode|all)
python build_plugin.py --vs-version 2026          # Specific VS version (2022|2026|auto)
python build_plugin.py --clean                    # Clean dist/ before build
python build_plugin.py --install-dir "C:\NSIS\Plugins"  # Copy to additional NSIS directory
python build_plugin.py --verbosity minimal        # MSBuild verbosity (quiet|minimal|normal|detailed|diagnostic)
```

The script will:
- Build for x86-unicode, x64-unicode, and x86-ansi configurations
- Copy DLLs to `dist/{x86-unicode|amd64-unicode|x86-ansi}/` directories
- Clean up intermediate build artifacts from `src/Build/`
- Supports parallel builds with automatic CPU optimization

## Usage

### Important: Plugin Memory Management

**CRITICAL**: Always use `/NOUNLOAD` flag on the first `show` call to keep the plugin loaded in memory. Without this, NSIS unloads the plugin after each call, losing all state (window handles, threads, etc.), causing `stop` to fail.

```nsis
; Correct - keeps plugin loaded
nssplashpng::show /NOUNLOAD 5000 /FADEIN /FADEOUT "$TEMP\splash.png"
nssplashpng::stop /FADEOUT

; Wrong - plugin unloaded after show, stop will fail
nssplashpng::show 5000 /FADEIN /FADEOUT "$TEMP\splash.png"
nssplashpng::stop /FADEOUT
```

### Syntax

```nsis
nsSplashPNG::show [/NOUNLOAD] <milliseconds> [/FADEIN [step]] [/FADEOUT [step]] [/NOCANCEL] [/MONITOR <target>] <image_path>
nsSplashPNG::stop [/FADEOUT [step]]
```

**Note**: The timeout must be specified immediately after `/NOUNLOAD` (if present), followed by optional flags, with the image path as the last parameter.

### Parameters

#### show function
- `/NOUNLOAD` - **Required for stop() to work**: Keeps plugin in memory between calls
- `/FADEIN [step]` - Enable fade in effect. Optional step (1-255) controls speed (default: 15)
  - Higher step = faster fade (255=instant, 51=fast, 15=normal, 5=slow)
  - Duration formula: `(255 / step) × 30ms`
- `/FADEOUT [step]` - Enable fade out effect when auto-closing via timer (optional step)
- `/NOCANCEL` - Disable click-to-close (splash can only be closed programmatically or by timer)
- `/MONITOR <target>` - Select the monitor on which to center the splash (default: primary monitor)
  - `PRIMARY` - Primary monitor (default)
  - `CURRENT` or `MOUSE` - Monitor where the mouse cursor is
  - `1`, `2`, `3`, ... - Monitor by 1-based index (enumeration order)
  - `POINT x y` - Monitor nearest to the given screen coordinates (uses `MONITOR_DEFAULTTONEAREST`); useful to target the monitor where an application window was last open
- `<milliseconds>` - Auto-close timer in milliseconds (0 = manual close only)
- `<image_path>` - Path to image file (PNG, JPEG, BMP, GIF, TIFF, ICO)

#### stop function
- `/FADEOUT [step]` - Enable fade out effect when closing. Optional step (1-255) controls speed (default: 15)

### Examples

#### Basic Usage
```nsis
; Show splash for 3 seconds (PNG with alpha) - no /NOUNLOAD needed for auto-close only
nsSplashPNG::show 3000 "$EXEDIR\splash.png"

; Show JPEG splash (opaque, no transparency)
nsSplashPNG::show 3000 "$EXEDIR\splash.jpg"
```

#### Fade Effects
```nsis
; Default fade speed (step 15, ~510ms)
nsSplashPNG::show 3000 /FADEIN /FADEOUT "$EXEDIR\splash.png"

; Fast fade (step 51, ~150ms)
nsSplashPNG::show 3000 /FADEIN 51 /FADEOUT 51 "$EXEDIR\splash.png"

; Slow fade (step 5, ~1530ms)
nsSplashPNG::show 5000 /FADEIN 5 /FADEOUT 5 "$EXEDIR\splash.png"

; Instant (step 255, ~30ms)
nsSplashPNG::show 3000 /FADEIN 255 /FADEOUT 255 "$EXEDIR\splash.png"

; Different speeds for fade-in and fade-out
nsSplashPNG::show 3000 /FADEIN 51 /FADEOUT 5 "$EXEDIR\splash.png"
```

#### Manual Control
```nsis
; CRITICAL: Use /NOUNLOAD when using stop() to close manually
nsSplashPNG::show /NOUNLOAD 0 /NOCANCEL "$EXEDIR\splash.png"

; ... do installation work ...

; Close with fade out
nsSplashPNG::stop /FADEOUT

; Or close with custom fade speed
nsSplashPNG::stop /FADEOUT 51
```

#### Real Installer Example
```nsis
Name "My Application"
OutFile "setup.exe"
InstallDir "$PROGRAMFILES\MyApp"

!addplugindir "plugins\x86-unicode"

Page instfiles

Section "Main Installation"
    ; Show splash with fade in, prevent user closing, no auto-close
    ; CRITICAL: /NOUNLOAD required for stop() to work
    nsSplashPNG::show /NOUNLOAD 0 /NOCANCEL /FADEIN "$EXEDIR\splash.png"
    
    ; Install files
    SetOutPath "$INSTDIR"
    File /r "app\*.*"
    
    ; Create shortcuts
    CreateDirectory "$SMPROGRAMS\MyApp"
    CreateShortcut "$SMPROGRAMS\MyApp\MyApp.lnk" "$INSTDIR\myapp.exe"
    
    ; Close splash with fade out
    nsSplashPNG::stop /FADEOUT
SectionEnd
```

#### Monitor Selection
```nsis
; Show on primary monitor (default)
nsSplashPNG::show /NOUNLOAD 0 /NOCANCEL "$EXEDIR\splash.png"

; Show on the monitor where the mouse cursor is
nsSplashPNG::show /NOUNLOAD 0 /NOCANCEL /MONITOR CURRENT "$EXEDIR\splash.png"

; Show on the second monitor (by 1-based index)
nsSplashPNG::show /NOUNLOAD 0 /NOCANCEL /MONITOR 2 "$EXEDIR\splash.png"

; Show on the monitor nearest to a known screen coordinate
; (e.g. where an application window was previously displayed)
nsSplashPNG::show /NOUNLOAD 0 /MONITOR POINT -1906 200 /FADEIN "$EXEDIR\splash.png"

nsSplashPNG::stop /FADEOUT
```

> **Note on parameter order**: `/MONITOR POINT x y` must be placed **before** `/FADEIN`/`/FADEOUT`.
> The `/FADEIN` parser reads ahead one token to check for a custom step; if that token
> starts with `/`, it is requeued — but a subsequent `popstring()` in the loop will
> overwrite it before `/MONITOR` can be processed.

#### Additional Examples

See the `examples/` folder for more usage demonstrations:
- **example_basic.nsi** - Simple splash with auto-close timer
- **example_advanced.nsi** - Manual control with fade effects
- **example_formats.nsi** - Multi-format support (PNG, JPEG, BMP, GIF)
- **example_fade_speed.nsi** - Customizable fade speed demonstrations
- **example_nonblocking.nsi** - Non-blocking behavior with background processing

## Technical Details

### Implementation
- **Graphics Library**: WIC (Windows Imaging Component) via windowscodecs.lib
- **Image Format**: 32bppPBGRA (pre-multiplied alpha) for proper transparency
- **Window Type**: Layered windows with `WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW`
- **Transparency**: `UpdateLayeredWindow` with per-pixel alpha blending
- **Rendering**: Hardware-accelerated via `UpdateLayeredWindow` with `ULW_ALPHA`
- **Animation**: 30ms timer intervals for smooth fade (0-255 alpha range)
  - Configurable step size (default: 15 steps = ~510ms duration)
  - Duration formula: `(255 / step) × 30ms`
- **Threading**: Background window thread for non-blocking execution
  - `show()` returns after fade-in completes
  - Window stays open in background
  - `stop()` waits for fade-out to complete before returning
- **Plugin State**: Requires `/NOUNLOAD` to maintain global variables (window handles, threads) between calls

### Comparison with nsAdvsplash

| Feature              | nsAdvsplash | nsSplashPNG     |
|----------------------|-------------|-----------------|
| PNG Alpha Support    | Partial     | Full            |
| Fade Effects         | Yes         | Yes             |
| Transparency Method  | Color key   | Per-pixel alpha |
| Graphics API         | GDI         | WIC             |
| Window Type          | Standard    | Layered         |
| Click-to-Close       | Yes         | Yes             |
| Compilation Warnings | Some        | None            |

### Supported Image Formats

WIC automatically detects and decodes the following formats:
- **PNG** - Full alpha transparency support
- **JPEG/JPG** - No transparency (opaque)
- **BMP** - Alpha transparency if 32-bit, otherwise opaque
- **GIF** - Basic transparency support
- **TIFF** - Alpha transparency if supported by variant
- **ICO** - Icon format with alpha
- **WMP** - Windows Media Photo

**Note**: Formats without alpha channels (JPEG, 24-bit BMP) will display as opaque images.

### System Requirements
- Windows Vista or later (for layered window support)
- WIC - Windows Imaging Component (included with Windows Vista+)

## API Reference

### Exported Functions

#### `show()`
Creates and displays a splash screen window in a background thread.

**Parameters** (parsed from NSIS stack in reverse order):
1. Timeout in milliseconds (integer, 0 = manual close only)
2. `/FADEIN [step]` - Optional fade in with configurable step (1-255, default: 15)
3. `/FADEOUT [step]` - Optional auto fade out with configurable step (1-255, default: 15)
4. `/NOCANCEL` - Disable click-to-close
5. `/MONITOR <target>` - Monitor selection (see parameter docs); must appear **before** `/FADEIN`
6. `/NOUNLOAD` - **Required for stop() to work**: Keeps plugin in memory
7. Image path (string) - last parameter

**Behavior**:
- Cleans up any previous window/thread
- Loads image with WIC (`IWICImagingFactory`, `IWICBitmapDecoder`)
- Supports PNG, JPEG, BMP, GIF, TIFF, ICO, WMP formats
- Converts to 32bppPBGRA format for proper alpha handling
- Creates centered topmost layered window on the selected monitor:
  - `MONITOR_MODE_PRIMARY` (default) — `GetSystemMetrics(SM_CX/CYSCREEN)`
  - `MONITOR_MODE_MOUSE` — `GetCursorPos` + `MonitorFromPoint`
  - `MONITOR_MODE_INDEX` — `EnumDisplayMonitors` by 0-based index
  - `MONITOR_MODE_POINT` — `MonitorFromPoint(x, y, MONITOR_DEFAULTTONEAREST)`
- Uses `UpdateLayeredWindow` for per-pixel alpha transparency
- Starts background thread with window message loop
- Applies fade in effect if `/FADEIN` specified (30ms timer with configurable steps)
- Sets auto-close timer if timeout > 0
- Enables click-to-close unless `/NOCANCEL` specified
- **Returns control after fade-in completes** (non-blocking)
- Without `/NOUNLOAD`, plugin unloads and loses all state

**Return value**: Pushes "success" to NSIS stack

#### `stop()`
Closes the splash screen window with optional fade-out.

**Parameters** (parsed from NSIS stack):
1. `/FADEOUT [step]` - Optional fade out with configurable step (1-255, default: 15)

**Behavior**:
- If `/FADEOUT` specified:
  - Starts fade-out timer (30ms intervals)
  - Waits for fade to complete: `(255 / step) × 30ms`
  - Destroys window after fade completes
- Otherwise:
  - Immediately destroys window
- **Requires show() to have used /NOUNLOAD** or window handle will be NULL

**Return value**: Pushes "success" to NSIS stack

**Behavior**:
- Applies fade out effect if `/FADEOUT` specified (30ms timer, 15 steps)
- Destroys window and releases WIC resources
- Terminates splash thread and cleans up handles

## Troubleshooting

### Image not showing
- Verify image file path is correct and accessible
- Check that image file is in a supported format (PNG, JPEG, BMP, GIF, TIFF, ICO)
- Ensure NSIS can read the file at install time
- Verify WIC can decode the specific image file variant

### No transparency
- Confirm PNG has alpha channel (not just RGB)
- Verify you're using the correct plugin DLL for your NSIS version
- Check Windows version supports layered windows (Vista+)

### Fade effects not smooth
- Normal behavior - fade uses 30ms timer with 15-step alpha increments
- Total fade duration ≈ 450-500ms

## License

Created for NSIS installer system.

## Credits

- NSIS Plugin API
- WIC - Windows Imaging Component
- Windows Layered Windows API (`UpdateLayeredWindow`)
- Build system inspired by nsProcess plugin

---

*See [README_IT.md](README_IT.md) for the Italian version.*

## Version History

### 1.0.0 (Current)
- Full PNG alpha transparency support via WIC
- Per-pixel alpha blending with `UpdateLayeredWindow`
- Fade in/out effects (30ms, 15 steps)
- Auto-close timer with configurable timeout
- Click-to-close with optional disable (`/NOCANCEL`)
- Blocks installer until splash closes
- x86-unicode, x64-unicode, and x86-ansi builds
- Zero compilation warnings with guarded macro definitions
- Optimized build system with parallel compilation support
