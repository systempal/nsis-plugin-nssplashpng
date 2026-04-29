// nsSplashPNG - PNG Splash Screen Plugin for NSIS
// Full PNG alpha transparency support using WIC and layered windows

#define _WIN32_WINNT 0x0500
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <objbase.h>
#include <wincodec.h>
#include <stdio.h>
#include <tchar.h>
#include "pluginapi.h"

#pragma comment(lib, "windowscodecs.lib")
#pragma comment(lib, "ole32.lib")

// Constants
#define RESOLUTION 30  // 30ms timer for smooth animation
#define FADEIN_DURATION 500   // 500ms fade in (255 / 15 * 30ms ≈ 510ms)
#define FADEOUT_DURATION 500  // 500ms fade out
#define CLASS_NAME _T("_nsSplashPNG")

// Global variables
HWND g_hWnd = NULL;
HINSTANCE g_hInstance = NULL;
HBITMAP g_hBitmap = NULL;
HDC g_hdcMem = NULL;
int g_imageWidth = 0;
int g_imageHeight = 0;
int g_timeoutMs = 0;
int g_fadeAlpha = 0;
bool g_fadeIn = false;
bool g_fadeOut = false;
bool g_noCancel = false;
int g_fadeInStep = 15;   // Default: 15 alpha units per timer tick
int g_fadeOutStep = 15;  // Default: 15 alpha units per timer tick
HANDLE g_hThread = NULL;
HANDLE g_hEvent = NULL;

// Monitor selection
#define MONITOR_MODE_PRIMARY 0  // Primary monitor (default)
#define MONITOR_MODE_MOUSE   1  // Monitor where mouse cursor is
#define MONITOR_MODE_INDEX   2  // Monitor by 1-based index
#define MONITOR_MODE_POINT   3  // Monitor nearest to a specific screen point
int g_monitorMode = MONITOR_MODE_PRIMARY;
int g_monitorIndex = 0; // 0-based index used internally
int g_monitorX = 0;    // Screen X coordinate for MONITOR_MODE_POINT
int g_monitorY = 0;    // Screen Y coordinate for MONITOR_MODE_POINT

struct MonitorEnumData {
    int targetIndex;
    int currentIndex;
    HMONITOR hMonitor;
    MONITORINFO info;
};

static BOOL CALLBACK GetMonitorByIndexProc(HMONITOR hMonitor, HDC, LPRECT, LPARAM dwData) {
    MonitorEnumData* pData = (MonitorEnumData*)dwData;
    if (pData->currentIndex == pData->targetIndex) {
        pData->hMonitor = hMonitor;
        pData->info.cbSize = sizeof(MONITORINFO);
        GetMonitorInfo(hMonitor, &pData->info);
        return FALSE; // Stop enumeration
    }
    pData->currentIndex++;
    return TRUE;
}

typedef BOOL (WINAPI *SetLayeredWindowAttributesProc)(HWND, COLORREF, BYTE, DWORD);
SetLayeredWindowAttributesProc g_SetLayeredWindowAttributes = NULL;

// Window procedure
LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE:
            return 0;

        case WM_PAINT: {
            PAINTSTRUCT ps;
            BeginPaint(hwnd, &ps);
            // Don't draw anything - layered window uses UpdateLayeredWindow
            EndPaint(hwnd, &ps);
            return 0;
        }

        case WM_LBUTTONDOWN:
            if (g_noCancel)
                return 0;
            // Intentional fallthrough to close splash on click
            DestroyWindow(hwnd);
            return 0;

        case WM_TIMER:
        {
            if (wParam == 1) { // Fade in timer
                if (g_fadeAlpha < 255) {
                    g_fadeAlpha += g_fadeInStep;
                    if (g_fadeAlpha > 255) g_fadeAlpha = 255;
                    
                    // Update alpha using UpdateLayeredWindow
                    if (g_hBitmap && g_hdcMem) {
                        HDC hdcScreen = GetDC(NULL);
                        POINT ptSrc = {0, 0};
                        SIZE sizeWnd = {g_imageWidth, g_imageHeight};
                        BLENDFUNCTION blend = {AC_SRC_OVER, 0, (BYTE)g_fadeAlpha, AC_SRC_ALPHA};
                        UpdateLayeredWindow(hwnd, hdcScreen, NULL, &sizeWnd, g_hdcMem, &ptSrc, 0, &blend, ULW_ALPHA);
                        ReleaseDC(NULL, hdcScreen);
                    }
                } else {
                    KillTimer(hwnd, 1);
                }
            } else if (wParam == 2) { // Close timer
                KillTimer(hwnd, 2);
                if (g_fadeOut) {
                    // Start fade out
                    SetTimer(hwnd, 3, RESOLUTION, NULL);
                } else {
                    DestroyWindow(hwnd);
                }
            } else if (wParam == 3) { // Fade out timer
                if (g_fadeAlpha > 0) {
                    g_fadeAlpha -= g_fadeOutStep;
                    if (g_fadeAlpha < 0) g_fadeAlpha = 0;
                    
                    // Update alpha using UpdateLayeredWindow
                    if (g_hBitmap && g_hdcMem) {
                        HDC hdcScreen = GetDC(NULL);
                        POINT ptSrc = {0, 0};
                        SIZE sizeWnd = {g_imageWidth, g_imageHeight};
                        BLENDFUNCTION blend = {AC_SRC_OVER, 0, (BYTE)g_fadeAlpha, AC_SRC_ALPHA};
                        UpdateLayeredWindow(hwnd, hdcScreen, NULL, &sizeWnd, g_hdcMem, &ptSrc, 0, &blend, ULW_ALPHA);
                        ReleaseDC(NULL, hdcScreen);
                    }
                } else {
                    KillTimer(hwnd, 3);
                    DestroyWindow(hwnd);
                }
            }
            return 0;
        }

        case WM_CLOSE:
            return 0;

        case WM_DESTROY:
        {
            if (g_hdcMem) {
                DeleteDC(g_hdcMem);
                g_hdcMem = NULL;
            }
            if (g_hBitmap) {
                DeleteObject(g_hBitmap);
                g_hBitmap = NULL;
            }
            g_hWnd = NULL;
            PostQuitMessage(0);
            return 0;
        }
    }
    
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

// DLL entry point
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved) {
    if (dwReason == DLL_PROCESS_ATTACH) {
        g_hInstance = hInstance;
        // This is acceptable for NSIS plugins which are loaded/unloaded by the installer process
    }
    return TRUE;
}

// Thread function to create and manage the splash window
DWORD WINAPI SplashThreadProc(LPVOID lpParam) {
    // Register window class
    WNDCLASS wc = {0};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = g_hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    RegisterClass(&wc);
    
    // Calculate centered position on the target monitor
    RECT monitorRect = {0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN)};
    if (g_monitorMode == MONITOR_MODE_MOUSE) {
        POINT pt;
        if (GetCursorPos(&pt)) {
            HMONITOR hMon = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);
            MONITORINFO mi = {sizeof(mi)};
            if (GetMonitorInfo(hMon, &mi))
                monitorRect = mi.rcMonitor;
        }
    } else if (g_monitorMode == MONITOR_MODE_INDEX) {
        MonitorEnumData data = {};
        data.targetIndex = g_monitorIndex;
        data.info.cbSize = sizeof(MONITORINFO);
        EnumDisplayMonitors(NULL, NULL, GetMonitorByIndexProc, (LPARAM)&data);
        if (data.hMonitor)
            monitorRect = data.info.rcMonitor;
    } else if (g_monitorMode == MONITOR_MODE_POINT) {
        POINT pt = {g_monitorX, g_monitorY};
        HMONITOR hMon = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);
        MONITORINFO mi = {sizeof(mi)};
        if (GetMonitorInfo(hMon, &mi))
            monitorRect = mi.rcMonitor;
    }
    int x = monitorRect.left + (monitorRect.right  - monitorRect.left - g_imageWidth)  / 2;
    int y = monitorRect.top  + (monitorRect.bottom - monitorRect.top  - g_imageHeight) / 2;
    
    // Create layered window
    g_hWnd = CreateWindowEx(
        WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        CLASS_NAME,
        _T(""),
        WS_POPUP,
        x, y, g_imageWidth, g_imageHeight,
        NULL,
        NULL,
        g_hInstance,
        NULL
    );
    
    if (!g_hWnd) {
        SetEvent(g_hEvent);
        return 1;
    }
    
    // Display using UpdateLayeredWindow
    g_fadeAlpha = g_fadeIn ? 0 : 255;
    
    HDC hdcScreen = GetDC(NULL);
    POINT ptSrc = {0, 0};
    SIZE sizeWnd = {g_imageWidth, g_imageHeight};
    BLENDFUNCTION blend = {AC_SRC_OVER, 0, (BYTE)g_fadeAlpha, AC_SRC_ALPHA};
    UpdateLayeredWindow(g_hWnd, hdcScreen, NULL, &sizeWnd, g_hdcMem, &ptSrc, 0, &blend, ULW_ALPHA);
    ReleaseDC(NULL, hdcScreen);
    
    ShowWindow(g_hWnd, SW_SHOW);
    SetForegroundWindow(g_hWnd);
    
    // Start timers
    if (g_fadeIn) {
        SetTimer(g_hWnd, 1, RESOLUTION, NULL);
    }
    if (g_timeoutMs > 0) {
        SetTimer(g_hWnd, 2, g_timeoutMs, NULL);
    }
    
    // Signal that window is created
    SetEvent(g_hEvent);
    
    // Message loop
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    // Cleanup
    UnregisterClass(CLASS_NAME, g_hInstance);
    return 0;
}

// Load PNG using Windows Imaging Component (WIC)
HBITMAP LoadPNGWithWIC(const TCHAR* filename, int* width, int* height) {
    
    HRESULT hr;
    IWICImagingFactory* pFactory = NULL;
    IWICBitmapDecoder* pDecoder = NULL;
    IWICBitmapFrameDecode* pFrame = NULL;
    IWICFormatConverter* pConverter = NULL;
    HBITMAP hBitmap = NULL;
    
    // Initialize COM (don't check failure - might already be initialized by NSIS)
    CoInitialize(NULL);
    
    hr = CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, 
                          IID_IWICImagingFactory, (LPVOID*)&pFactory);
    if (FAILED(hr)) {
        return NULL;
    }
    
    hr = pFactory->CreateDecoderFromFilename((LPCWSTR)filename, NULL, GENERIC_READ,
                                            WICDecodeMetadataCacheOnLoad, &pDecoder);
    if (FAILED(hr)) goto cleanup;
    
    hr = pDecoder->GetFrame(0, &pFrame);
    if (FAILED(hr)) goto cleanup;
    
    UINT imgWidth, imgHeight;
    pFrame->GetSize(&imgWidth, &imgHeight);
    *width = imgWidth;
    *height = imgHeight;
    
    hr = pFactory->CreateFormatConverter(&pConverter);
    if (FAILED(hr)) goto cleanup;
    
    hr = pConverter->Initialize(pFrame, GUID_WICPixelFormat32bppPBGRA,
                                WICBitmapDitherTypeNone, NULL, 0.0,
                                WICBitmapPaletteTypeMedianCut);
    if (FAILED(hr)) goto cleanup;
    
    // Declare variables before goto
    BITMAPINFO bmi;
    ZeroMemory(&bmi, sizeof(bmi));
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = imgWidth;
    bmi.bmiHeader.biHeight = -(int)imgHeight;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    
    void* pBits;
    pBits = NULL;
    HDC hdc;
    hdc = GetDC(NULL);
    hBitmap = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, &pBits, NULL, 0);
    ReleaseDC(NULL, hdc);
    
    if (!hBitmap) goto cleanup;
    
    hr = pConverter->CopyPixels(NULL, imgWidth * 4, imgWidth * imgHeight * 4, (BYTE*)pBits);
    if (FAILED(hr)) {
        DeleteObject(hBitmap);
        hBitmap = NULL;
    }
    
cleanup:
    if (pConverter) pConverter->Release();
    if (pFrame) pFrame->Release();
    if (pDecoder) pDecoder->Release();
    if (pFactory) pFactory->Release();
    // Don't call CoUninitialize - let NSIS handle COM cleanup
    // CoUninitialize();
    
    return hBitmap;
}

// Plugin exported function: show
extern "C" void __declspec(dllexport) show(HWND hwndParent, int string_size, TCHAR *variables, stack_t **stacktop) {
    EXDLL_INIT();
    
    // Close any existing window and thread first
    if (g_hWnd && IsWindow(g_hWnd)) {
        DestroyWindow(g_hWnd);
        // Give thread time to process WM_DESTROY and exit message loop
        Sleep(200);
    }
    
    // Wait for previous thread to terminate if it exists
    if (g_hThread) {
        // Wait briefly for thread to terminate
        WaitForSingleObject(g_hThread, 500);
        CloseHandle(g_hThread);
        g_hThread = NULL;
    }
    
    // Ensure window handle is cleared
    g_hWnd = NULL;
    
    // Parameters come from stack in order: timeout, flags..., imagepath
    // Read timeout first
    TCHAR param[MAX_PATH];
    TCHAR imagePath[MAX_PATH] = {0};
    
    popstring(param);
    g_timeoutMs = myatoi(param);
    
    // Reset flags
    g_fadeIn = false;
    g_fadeOut = false;
    g_noCancel = false;
    g_fadeInStep = 15;   // Default step
    g_fadeOutStep = 15;  // Default step
    g_monitorMode = MONITOR_MODE_PRIMARY;
    g_monitorIndex = 0;
    g_monitorX = 0;
    g_monitorY = 0;
    
    // Read flags and image path
    while (popstring(param) == 0 && param[0] == _T('/')) {
        if (_tcsicmp(param, _T("/FADEIN")) == 0) {
            g_fadeIn = true;
            // Check if next parameter is a number (custom step)
            TCHAR nextParam[MAX_PATH];
            if (popstring(nextParam) == 0 && nextParam[0] != _T('/')) {
                int customStep = myatoi(nextParam);
                if (customStep > 0 && customStep <= 255) {
                    g_fadeInStep = customStep;
                } else {
                    // Not a valid step, push it back (it's probably the image path)
                    lstrcpy(param, nextParam);
                    break;
                }
            } else {
                // Push it back for next iteration
                lstrcpy(param, nextParam);
            }
        } else if (_tcsicmp(param, _T("/FADEOUT")) == 0) {
            g_fadeOut = true;
            // Check if next parameter is a number (custom step)
            TCHAR nextParam[MAX_PATH];
            if (popstring(nextParam) == 0 && nextParam[0] != _T('/')) {
                int customStep = myatoi(nextParam);
                if (customStep > 0 && customStep <= 255) {
                    g_fadeOutStep = customStep;
                } else {
                    // Not a valid step, push it back
                    lstrcpy(param, nextParam);
                    break;
                }
            } else {
                // Push it back for next iteration
                lstrcpy(param, nextParam);
            }
        } else if (_tcsicmp(param, _T("/NOCANCEL")) == 0) {
            g_noCancel = true;
        } else if (_tcsicmp(param, _T("/MONITOR")) == 0) {
            TCHAR monParam[64] = {0};
            if (popstring(monParam) == 0) {
                if (_tcsicmp(monParam, _T("CURRENT")) == 0 || _tcsicmp(monParam, _T("MOUSE")) == 0) {
                    g_monitorMode = MONITOR_MODE_MOUSE;
                } else if (_tcsicmp(monParam, _T("PRIMARY")) == 0) {
                    g_monitorMode = MONITOR_MODE_PRIMARY;
                } else if (_tcsicmp(monParam, _T("POINT")) == 0) {
                    TCHAR xParam[64] = {0}, yParam[64] = {0};
                    if (popstring(xParam) == 0 && popstring(yParam) == 0) {
                        g_monitorMode = MONITOR_MODE_POINT;
                        g_monitorX = myatoi(xParam);
                        g_monitorY = myatoi(yParam);
                    }
                } else {
                    int idx = myatoi(monParam);
                    if (idx >= 1) {
                        g_monitorMode = MONITOR_MODE_INDEX;
                        g_monitorIndex = idx - 1; // Convert to 0-based
                    }
                }
            }
        }
    }
    
    // Last parameter read (not starting with /) is the image path
    lstrcpy(imagePath, param);
    
    // Validate image path
    if (imagePath[0] == 0) {
        pushstring(_T("error: no image path"));
        return;
    }
    
    // Load PNG image with WIC
    g_hBitmap = LoadPNGWithWIC(imagePath, &g_imageWidth, &g_imageHeight);
    if (!g_hBitmap) {
        pushstring(_T("error: cannot load image"));
        return;
    }
    
    // Create memory DC for the bitmap
    HDC hdc = GetDC(NULL);
    g_hdcMem = CreateCompatibleDC(hdc);
    ReleaseDC(NULL, hdc);
    SelectObject(g_hdcMem, g_hBitmap);
    
    // Create event for synchronization
    g_hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (!g_hEvent) {
        pushstring(_T("error: cannot create event"));
        if (g_hdcMem) DeleteDC(g_hdcMem);
        if (g_hBitmap) DeleteObject(g_hBitmap);
        return;
    }
    
    // Create thread to manage the window
    g_hThread = CreateThread(NULL, 0, SplashThreadProc, NULL, 0, NULL);
    if (!g_hThread) {
        pushstring(_T("error: cannot create thread"));
        CloseHandle(g_hEvent);
        if (g_hdcMem) DeleteDC(g_hdcMem);
        if (g_hBitmap) DeleteObject(g_hBitmap);
        return;
    }
    
    // Wait for window to be created (with timeout)
    DWORD result = WaitForSingleObject(g_hEvent, 2000);
    CloseHandle(g_hEvent);
    g_hEvent = NULL;
    
    if (result != WAIT_OBJECT_0 || !g_hWnd) {
        pushstring(_T("error: window not created"));
        return;
    }
    
    // If fade-in is enabled, wait for it to complete before returning
    if (g_fadeIn) {
        // Calculate fade-in duration: (255 / step) * 30ms + 100ms margin
        int fadeSteps = (255 + g_fadeInStep - 1) / g_fadeInStep;  // Round up
        int fadeDuration = fadeSteps * RESOLUTION + 100;
        Sleep(fadeDuration);
    }
    
    // Return control to NSIS immediately - window stays open in background thread
    // Window will close either:
    // 1. When timeout expires (if g_timeoutMs > 0)
    // 2. When user clicks (if !g_noCancel)
    // 3. When stop() is called explicitly
    pushstring(_T("success"));
}

// Plugin exported function: stop
extern "C" void __declspec(dllexport) stop(HWND hwndParent, int string_size, TCHAR *variables, stack_t **stacktop) {
    EXDLL_INIT();
    
    // Parse flags
    TCHAR param[256] = {0};
    bool fadeOut = false;
    
    while (popstring(param) == 0) {
        if (_tcsicmp(param, _T("/FADEOUT")) == 0) {
            fadeOut = true;
            // Check if next parameter is a number (custom step)
            TCHAR nextParam[MAX_PATH];
            if (popstring(nextParam) == 0 && nextParam[0] != _T('/')) {
                int customStep = myatoi(nextParam);
                if (customStep > 0 && customStep <= 255) {
                    g_fadeOutStep = customStep;
                }
            }
        }
    }
    
    if (g_hWnd && IsWindow(g_hWnd)) {
        if (fadeOut) {
            // Start fade out timer
            SetTimer(g_hWnd, 3, RESOLUTION, NULL);
            
            // Calculate expected fade-out duration and wait
            int fadeSteps = (255 + g_fadeOutStep - 1) / g_fadeOutStep;
            int fadeDuration = fadeSteps * RESOLUTION + 200;
            
            Sleep(fadeDuration);
            
            // Give extra time for window destruction and thread cleanup
            Sleep(100);
        } else {
            // No fade-out, destroy immediately
            DestroyWindow(g_hWnd);
            Sleep(100);  // Give thread time to cleanup
        }
    }
    
    pushstring(_T("success"));
}
