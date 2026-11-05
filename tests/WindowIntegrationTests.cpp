#include <windows.h>
#include <commctrl.h>
#include "resource.h"
#include "MainWindow.h"
#include <filesystem>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <cstdint>
#include <thread>
#include <future>

namespace {
int checks = 0;
void require(bool condition, const char* name) {
    if (!condition) throw std::runtime_error(name);
    ++checks; std::cout << "PASS: " << name << '\n';
}
void wait(const std::function<bool()>& condition, const char* message) {
    const auto deadline = GetTickCount64() + 15000;
    while (GetTickCount64() < deadline) { if (condition()) return; Sleep(30); }
    throw std::runtime_error(message);
}
std::wstring text(HWND window) {
    wchar_t buffer[512]{}; GetWindowTextW(window, buffer, 512); return buffer;
}
std::uint64_t frame(HWND window) {
    RedrawWindow(window, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
    RECT client{}; GetClientRect(window, &client);
    HDC source = GetDC(window), target = CreateCompatibleDC(source);
    BITMAPINFO info{}; info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth = client.right; info.bmiHeader.biHeight = -client.bottom;
    info.bmiHeader.biPlanes = 1; info.bmiHeader.biBitCount = 32; info.bmiHeader.biCompression = BI_RGB;
    void* pixels = nullptr;
    HBITMAP bitmap = CreateDIBSection(source, &info, DIB_RGB_COLORS, &pixels, nullptr, 0);
    auto old = SelectObject(target, bitmap);
    const BOOL copied = PrintWindow(window, target, PW_CLIENTONLY);
    std::uint64_t hash = 1469598103934665603ULL;
    if (copied && pixels) {
        const auto* data = static_cast<const unsigned char*>(pixels);
        for (size_t i = 0; i < static_cast<size_t>(client.right) * client.bottom * 4; ++i)
            if (i % 4 != 3) hash = (hash ^ data[i]) * 1099511628211ULL;
    }
    SelectObject(target, old); DeleteObject(bitmap); DeleteDC(target); ReleaseDC(window, source);
    if (!copied || !pixels) throw std::runtime_error("Canvas frame capture failed");
    return hash;
}
struct WindowSession {
    HWND window = nullptr;
    std::thread ui;
    explicit WindowSession(const std::vector<std::filesystem::path>& paths) {
        std::promise<HWND> started; auto result = started.get_future();
        ui = std::thread([&, paths] {
            try {
                INITCOMMONCONTROLSEX controls{sizeof(controls), ICC_BAR_CLASSES | ICC_STANDARD_CLASSES | ICC_TAB_CLASSES};
                InitCommonControlsEx(&controls);
                spdf::MainWindow viewer(GetModuleHandleW(nullptr));
                std::vector<std::wstring> names; for (const auto& path : paths) names.push_back(path.wstring());
                if (!viewer.create(SW_SHOWNORMAL, names)) throw std::runtime_error("Main window not created");
                started.set_value(viewer.handle());
                MSG message{};
                while (GetMessageW(&message, nullptr, 0, 0) > 0) { TranslateMessage(&message); DispatchMessageW(&message); }
                if (IsWindow(viewer.handle())) DestroyWindow(viewer.handle());
            } catch (...) { try { started.set_exception(std::current_exception()); } catch (...) {} }
        });
        try { window = result.get(); } catch (...) { ui.join(); throw; }
    }
    ~WindowSession() {
        if (window) PostMessageW(window, WM_CLOSE, 0, 0);
        if (ui.joinable()) ui.join();
    }
    HWND tabs() { return GetDlgItem(window, IDC_TABS); }
    HWND toolbar() { return GetDlgItem(window, IDC_TOOLBAR); }
    int count() { return static_cast<int>(SendMessageW(tabs(), TCM_GETITEMCOUNT, 0, 0)); }
    bool fits(int id) {
        RECT rect{}, client{}; GetWindowRect(GetDlgItem(window, id), &rect);
        MapWindowPoints(nullptr, window, reinterpret_cast<POINT*>(&rect), 2); GetClientRect(window, &client);
        return rect.left >= 0 && rect.top >= 0 && rect.right <= client.right && rect.bottom <= client.bottom;
    }
    std::wstring status(int part) {
        wchar_t buffer[512]{};
        SendMessageW(GetDlgItem(window, IDC_STATUS), SB_GETTEXTW, part, reinterpret_cast<LPARAM>(buffer));
        return buffer;
    }
    void loaded(const std::wstring& name, int expected) {
        wait([&] { return count() == expected && text(window).find(name) != std::wstring::npos && status(5) == L"Kész"; }, "Tabs did not finish loading");
    }
    void command(int id) { SendMessageW(window, WM_COMMAND, id, 0); }
    void clickTab(int index, bool close = false) {
        RECT rect{};
        const auto ok = SendMessageW(tabs(), TCM_GETITEMRECT, index, reinterpret_cast<LPARAM>(&rect));
        if (!ok) throw std::runtime_error("Tab rectangle not found");
        HDC dc = GetDC(window); const int dpi = GetDeviceCaps(dc, LOGPIXELSX); ReleaseDC(window, dc);
        const int x = close ? rect.right - MulDiv(16, dpi, 96) : (rect.left + rect.right) / 2;
        const int y = (rect.top + rect.bottom) / 2;
        SendMessageW(tabs(), WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(x, y));
        SendMessageW(tabs(), WM_LBUTTONUP, 0, MAKELPARAM(x, y));
    }
    HWND dialog(bool password) {
        struct Search { DWORD pid; bool password; HWND found; } search{GetCurrentProcessId(), password, nullptr};
        EnumWindows([](HWND hwnd, LPARAM context) -> BOOL {
            auto* s = reinterpret_cast<Search*>(context); DWORD pid = 0; GetWindowThreadProcessId(hwnd, &pid);
            wchar_t name[64]{}; GetClassNameW(hwnd, name, 64);
            if (pid == s->pid && std::wstring(name) == L"#32770" &&
                (GetDlgItem(hwnd, IDC_PASSWORD) != nullptr) == s->password) { s->found = hwnd; return FALSE; }
            return TRUE;
        }, reinterpret_cast<LPARAM>(&search));
        return search.found;
    }
};
}
int main(int argc, char** argv) {
    if (argc != 2) return 1;
    try {
        const auto fixtures = std::filesystem::u8path(argv[1]);
        const auto temp = std::filesystem::temp_directory_path() / (L"sPDF-ui-" + std::to_wstring(GetCurrentProcessId()) + L"-" + std::to_wstring(GetTickCount64()));
        std::filesystem::create_directories(temp);
        const auto a = temp / L"Dokumentum-A.pdf", b = temp / L"Dokumentum-B.pdf";
        std::filesystem::copy_file(fixtures / "sample.pdf", a);
        std::filesystem::copy_file(fixtures / "sample.pdf", b);
        {
            WindowSession app({a, b, a}); app.loaded(L"Dokumentum-A.pdf", 2);
            require(app.count() == 2, "initial paths open two documents and deduplicate the third path");
            require(!IsWindowEnabled(GetDlgItem(app.window, IDC_SEARCH)), "unfinished search is visibly disabled");
            require(!SendMessageW(app.toolbar(), TB_ISBUTTONENABLED, IDM_PRINT, 0), "unfinished printing is disabled");
            SetWindowPos(app.window, nullptr, 0, 0, 1366, 768, SWP_NOZORDER);
            require(app.fits(IDC_SEARCH) && app.fits(IDM_VIEWMENU) && app.fits(IDC_PAGE), "controls fit a 1366x768 window");
            SetWindowPos(app.window, nullptr, 0, 0, 800, 620, SWP_NOZORDER);
            require(app.fits(IDC_SEARCH) && app.fits(IDM_VIEWMENU) && app.fits(IDC_PAGE), "narrow layout keeps navigation and view controls visible");
            require(SendMessageW(GetDlgItem(app.window, IDC_STATUS), SB_GETPARTS, 0, 0) == 6, "status bar has six separate fields");
            SetWindowPos(app.window, nullptr, 0, 0, 1180, 740, SWP_NOZORDER);
            app.command(IDM_NEXT); app.command(IDM_ACTUALSIZE);
            for (int i = 0; i < 8; ++i) app.command(IDM_ZOOMIN);
            app.command(IDM_ROTATE); app.loaded(L"Dokumentum-A.pdf", 2);
            const auto zoom = app.status(1);
            HWND canvas = FindWindowExW(app.window, nullptr, L"sPDF.Canvas", nullptr);
            const auto unscrolled = frame(canvas);
            SendMessageW(canvas, WM_VSCROLL, SB_BOTTOM, 0);
            const auto saved = frame(canvas);
            require(saved != unscrolled, "enlarged page can scroll visibly");
            app.clickTab(1); app.loaded(L"Dokumentum-B.pdf", 2);
            require(text(GetDlgItem(app.window, IDC_PAGE)) == L"1", "second tab retains its own initial page");
            app.clickTab(0); app.loaded(L"Dokumentum-A.pdf", 2);
            require(text(GetDlgItem(app.window, IDC_PAGE)) == L"2", "first tab page restored through real tab click");
            require(app.status(1) == zoom, "first tab zoom restored in status bar");
            require(frame(canvas) == saved, "first tab rotated/scrolled page restored pixel for pixel");
            app.clickTab(1, true);
            wait([&] { return app.count() == 1; }, "Background X did not close tab");
            require(text(app.window).find(L"Dokumentum-A.pdf") != std::wstring::npos, "background X preserves active document");
            app.clickTab(0, true);
            wait([&] { return app.count() == 0; }, "Last X did not close tab");
            require(!IsWindowEnabled(GetDlgItem(app.window, IDC_PAGE)), "closing final tab returns to empty viewer");
        }
        {
            WindowSession app({a, fixtures / "broken.pdf", b});
            wait([&] { return app.dialog(false) != nullptr; }, "Corrupt document error not shown");
            SendMessageW(app.dialog(false), WM_COMMAND, IDOK, 0); app.loaded(L"Dokumentum-B.pdf", 2);
            require(app.count() == 2, "corrupt document does not close either valid tab");
            app.command(IDM_CLOSE); app.loaded(L"Dokumentum-A.pdf", 1);
            require(app.count() == 1, "Ctrl+W command closes only active document");
        }
        {
            WindowSession app({a, fixtures / "encrypted.pdf"});
            wait([&] { return app.dialog(true) != nullptr; }, "Password dialog not shown");
            const auto dialog = app.dialog(true);
            SetWindowTextW(GetDlgItem(dialog, IDC_PASSWORD), L"spdf-test"); SendMessageW(dialog, WM_COMMAND, IDOK, 0);
            app.loaded(L"encrypted.pdf", 2);
            require(app.count() == 2, "password dialog opens encrypted PDF alongside existing tab");
            app.command(IDM_CLOSE); app.loaded(L"Dokumentum-A.pdf", 1);
            require(app.count() == 1, "closing encrypted tab preserves original PDF");
        }
        std::filesystem::remove_all(temp);
        std::cout << checks << " checks passed; 0 failed\n"; return 0;
    } catch (const std::exception& error) {
        std::cerr << "FAIL after " << checks << " checks: " << error.what() << '\n'; return 1;
    }
}
