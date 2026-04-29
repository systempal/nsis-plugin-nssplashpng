# nsSplashPNG Quick Reference

## Basic Syntax

```nsis
nsSplashPNG::show [/NOUNLOAD] <timeout> [/FADEIN [step]] [/FADEOUT [step]] [/NOCANCEL] <image_path>
nsSplashPNG::stop [/FADEOUT [step]]
```

## Critical Requirements

⚠️ **ALWAYS use `/NOUNLOAD` when calling `stop()` manually**

Without `/NOUNLOAD`, the plugin unloads after `show()` and loses all state, causing `stop()` to fail.

## Common Use Cases

### Auto-Close Only (No /NOUNLOAD needed)
```nsis
; Splash closes automatically after 3 seconds
nssplashpng::show 3000 "$TEMP\splash.png"
```

### Manual Close (Requires /NOUNLOAD)
```nsis
; Show splash, no auto-close
nssplashpng::show /NOUNLOAD 0 "$TEMP\splash.png"

; ... do work ...

; Close manually
nssplashpng::stop
```

### With Fade Effects (Auto-close)
```nsis
; Fade in, wait 3 seconds, fade out
nssplashpng::show 3000 /FADEIN /FADEOUT "$TEMP\splash.png"
```

### With Fade Effects (Manual close - Requires /NOUNLOAD)
```nsis
; Fade in, no auto-close
nssplashpng::show /NOUNLOAD 0 /FADEIN "$TEMP\splash.png"

; ... do work ...

; Fade out when closing
nssplashpng::stop /FADEOUT
```

### Prevent User Closing
```nsis
; User cannot click to close, must wait for timer or stop()
nssplashpng::show /NOUNLOAD 0 /NOCANCEL "$TEMP\splash.png"
nssplashpng::stop
```

### Custom Fade Speed
```nsis
; Fast fade (step 51, ~150ms)
nssplashpng::show /NOUNLOAD 0 /FADEIN 51 "$TEMP\splash.png"
nssplashpng::stop /FADEOUT 51

; Slow fade (step 5, ~1530ms)
nssplashpng::show /NOUNLOAD 0 /FADEIN 5 "$TEMP\splash.png"
nssplashpng::stop /FADEOUT 5

; Instant (step 255, ~30ms)
nssplashpng::show /NOUNLOAD 0 /FADEIN 255 "$TEMP\splash.png"
nssplashpng::stop /FADEOUT 255
```

## Fade Speed Reference

| Step Value | Duration | Description |
|------------|----------|-------------|
| 1 | ~7650ms | Very slow |
| 5 | ~1530ms | Slow |
| 15 | ~510ms | **Default**, smooth |
| 51 | ~150ms | Fast |
| 85 | ~90ms | Very fast |
| 255 | ~30ms | Instant |

**Formula**: Duration = (255 / step) × 30ms

## Parameter Order

The order matters! Parameters must appear in this sequence:

```nsis
nssplashpng::show [flags] <timeout> [more_flags] <image_path>
```

**Correct**:
```nsis
nssplashpng::show /NOUNLOAD 5000 /FADEIN /FADEOUT "$TEMP\splash.png"
```

**Wrong** (image_path must be last):
```nsis
nssplashpng::show /FADEIN "$TEMP\splash.png" 5000 /FADEOUT
```

## Supported Image Formats

- **PNG** - Full alpha transparency ✅
- **JPEG/JPG** - Opaque (no transparency)
- **BMP** - Alpha if 32-bit, otherwise opaque
- **GIF** - Basic transparency support
- **TIFF** - Alpha if supported by variant
- **ICO** - Icon format with alpha
- **WMP** - Windows Media Photo

## Troubleshooting

### Problem: stop() doesn't close the splash
**Solution**: Add `/NOUNLOAD` to the `show()` call

### Problem: Splash closes too quickly/slowly
**Solution**: Adjust the step parameter:
- Faster: Increase step value (e.g., 51, 85, 255)
- Slower: Decrease step value (e.g., 5, 3, 1)

### Problem: User can close splash during installation
**Solution**: Add `/NOCANCEL` flag

### Problem: Script waits at show() forever
**Solution**: Either:
- Use a timeout value > 0 for auto-close, OR
- Use timeout = 0 and call `stop()` manually (requires `/NOUNLOAD`)

## Non-Blocking Behavior

The `show()` function returns control to your script **after the fade-in completes**, allowing you to:
1. Show the splash
2. Continue executing your script
3. Close the splash when ready

```nsis
nssplashpng::show /NOUNLOAD 0 /FADEIN "$TEMP\splash.png"
; Control returns here after fade-in
DetailPrint "Installing..."
; Splash is still visible while we work
Sleep 2000
nssplashpng::stop /FADEOUT
; Control returns here after fade-out
```

## Examples

See the `examples/` directory for complete working examples:
- `example_basic.nsi` - Simple usage patterns
- `example_advanced.nsi` - Real installer with progress
- `example_formats.nsi` - Multi-format demonstrations
- `example_fade_speed.nsi` - All fade speed options
- `example_nonblocking.nsi` - Non-blocking execution pattern
