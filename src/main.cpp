#include "MainWindow.h"
#include "resource.h"
#include <shellapi.h>

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int show) {
    INITCOMMONCONTROLSEX controls{sizeof(controls), ICC_BAR_CLASSES | ICC_STANDARD_CLASSES};
    InitCommonControlsEx(&controls);
    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    std::wstring path;
    bool smoke = false;
    if (argv && argc > 1) {
        smoke = std::wstring(argv[1]) == L"--smoke-test";
        if (smoke && argc > 2) path = argv[2];
        else if (!smoke) path = argv[1];
    }
    if (argv) LocalFree(argv);
    try {
        spdf::MainWindow window(instance);
        if (!window.create(show, path, smoke)) return 1;
        const HACCEL accelerators = LoadAcceleratorsW(instance, MAKEINTRESOURCEW(IDR_ACCELERATORS));
        MSG message{};
        int result;
        while ((result = GetMessageW(&message, nullptr, 0, 0)) > 0) {
            if (!TranslateAcceleratorW(window.handle(), accelerators, &message)) {
                TranslateMessage(&message); DispatchMessageW(&message);
            }
        }
        return result < 0 ? 1 : static_cast<int>(message.wParam);
    } catch (...) {
        MessageBoxW(nullptr, L"Az alkalmazás nem indítható. Nincs elég memória vagy egy szükséges erőforrás hiányzik.",
            L"sPDF Megtekintő", MB_OK | MB_ICONERROR);
        return 1;
    }
}
