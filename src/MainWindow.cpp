#include "MainWindow.h"
#include "resource.h"
#include <commdlg.h>
#include <shellapi.h>
#include <uxtheme.h>
#include <algorithm>
#include <cmath>
#include <cwchar>
#include <sstream>

namespace spdf {
namespace {
constexpr wchar_t WindowClass[] = L"sPDF.MainWindow";
constexpr wchar_t CanvasClass[] = L"sPDF.Canvas";
constexpr UINT_PTR ResizeTimer = 1;
constexpr UINT_PTR SmokeTimer = 2;
std::wstring filename(const std::wstring& path) { return path.substr(path.find_last_of(L"\\/") + 1); }
INT_PTR CALLBACK passwordProc(HWND dialog, UINT message, WPARAM wparam, LPARAM lparam) {
    if (message == WM_INITDIALOG) {
        SetWindowLongPtrW(dialog, DWLP_USER, lparam);
        SendDlgItemMessageW(dialog, IDC_PASSWORD, EM_SETLIMITTEXT, 255, 0);
        SetFocus(GetDlgItem(dialog, IDC_PASSWORD));
        return FALSE;
    }
    if (message == WM_COMMAND) {
        if (LOWORD(wparam) == IDOK) {
            auto* password = reinterpret_cast<std::wstring*>(GetWindowLongPtrW(dialog, DWLP_USER));
            wchar_t buffer[256]{};
            GetDlgItemTextW(dialog, IDC_PASSWORD, buffer, 256);
            *password = buffer;
            SecureZeroMemory(buffer, sizeof(buffer));
            EndDialog(dialog, IDOK); return TRUE;
        }
        if (LOWORD(wparam) == IDCANCEL) { EndDialog(dialog, IDCANCEL); return TRUE; }
    }
    return FALSE;
}
std::string utf8(const std::wstring& text) {
    if (text.empty()) return {};
    int count = WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0, nullptr, nullptr);
    std::string output(count, '\0');
    WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), output.data(), count, nullptr, nullptr);
    return output;
}
}
MainWindow::~MainWindow() {
    renderer_.reset();
    if (images_) ImageList_Destroy(images_);
    if (font_) DeleteObject(font_);
    if (background_) DeleteObject(background_);
}
bool MainWindow::create(int show, const std::wstring& path, bool smoke) {
    smokeTest_ = smoke;
    WNDCLASSEXW wc{}; wc.cbSize = sizeof(wc);
    wc.hInstance = instance_; wc.lpfnWndProc = windowProc; wc.lpszClassName = WindowClass;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hIcon = LoadIconW(instance_, MAKEINTRESOURCEW(IDI_APP));
    wc.hIconSm = wc.hIcon;
    wc.lpszMenuName = MAKEINTRESOURCEW(IDR_MENU);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    if (!RegisterClassExW(&wc)) return false;
    wc.lpfnWndProc = canvasProc; wc.lpszClassName = CanvasClass; wc.lpszMenuName = nullptr;
    wc.hbrBackground = nullptr;
    if (!RegisterClassExW(&wc)) return false;
    window_ = CreateWindowExW(WS_EX_ACCEPTFILES, WindowClass, L"sPDF Megtekintő — 0.1.0 előzetes",
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, CW_USEDEFAULT, CW_USEDEFAULT, 1100, 780,
        nullptr, nullptr, instance_, this);
    if (!window_) return false;
    ShowWindow(window_, show); UpdateWindow(window_);
    renderer_ = std::make_unique<PdfRenderer>(window_);
    if (!path.empty()) open(path);
    if (smokeTest_) SetTimer(window_, SmokeTimer, 30000, nullptr);
    return true;
}
LRESULT CALLBACK MainWindow::windowProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
    auto* self = reinterpret_cast<MainWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (message == WM_NCCREATE) {
        self = static_cast<MainWindow*>(reinterpret_cast<CREATESTRUCTW*>(lparam)->lpCreateParams);
        self->window_ = hwnd;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
    }
    return self ? self->message(message, wparam, lparam) : DefWindowProcW(hwnd, message, wparam, lparam);
}
void MainWindow::makeControls() {
    HDC dc = GetDC(window_); dpi_ = GetDeviceCaps(dc, LOGPIXELSX); ReleaseDC(window_, dc);
    background_ = CreateSolidBrush(RGB(248, 249, 251));
    toolbar_ = CreateWindowExW(0, TOOLBARCLASSNAMEW, nullptr, WS_CHILD | WS_VISIBLE |
        TBSTYLE_FLAT | TBSTYLE_TOOLTIPS | CCS_NORESIZE | CCS_NOPARENTALIGN | CCS_NODIVIDER,
        0, 0, 0, 0, window_, nullptr, instance_, nullptr);
    SendMessageW(toolbar_, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
    SendMessageW(toolbar_, TB_SETUNICODEFORMAT, TRUE, 0);
    status_ = CreateWindowExW(0, STATUSCLASSNAMEW, nullptr, WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,
        0, 0, 0, 0, window_, nullptr, instance_, nullptr);
    canvas_ = CreateWindowExW(0, CanvasClass, nullptr, WS_CHILD | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL,
        0, 0, 0, 0, window_, nullptr, instance_, this);
    pageEdit_ = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"1", WS_CHILD | WS_VISIBLE | WS_TABSTOP |
        ES_NUMBER | ES_CENTER | ES_AUTOHSCROLL, 0, 0, 0, 0, window_, reinterpret_cast<HMENU>(IDC_PAGE), instance_, nullptr);
    SendMessageW(pageEdit_, EM_SETLIMITTEXT, 7, 0);
    SetWindowSubclass(pageEdit_, pageProc, 1, reinterpret_cast<DWORD_PTR>(this));
    SetWindowTheme(toolbar_, L"Explorer", nullptr);
    rebuildToolbar();
    updateControls();
}
void MainWindow::rebuildToolbar() {
    if (font_) DeleteObject(font_);
    font_ = CreateFontW(-px(12), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    SendMessageW(pageEdit_, WM_SETFONT, reinterpret_cast<WPARAM>(font_), TRUE);
    SendMessageW(status_, WM_SETFONT, reinterpret_cast<WPARAM>(font_), TRUE);
    while (SendMessageW(toolbar_, TB_BUTTONCOUNT, 0, 0)) SendMessageW(toolbar_, TB_DELETEBUTTON, 0, 0);
    if (images_) ImageList_Destroy(images_);
    images_ = ImageList_Create(px(24), px(24), ILC_COLOR32 | ILC_MASK, 8, 0);
    const int icons[] = {IDI_OPEN, IDI_PREVIOUS, IDI_NEXT, IDI_ZOOMOUT, IDI_ZOOMIN, IDI_FITPAGE, IDI_FITWIDTH, IDI_ROTATE};
    const int commands[] = {IDM_OPEN, IDM_PREVIOUS, IDM_NEXT, IDM_ZOOMOUT, IDM_ZOOMIN, IDM_FITPAGE, IDM_FITWIDTH, IDM_ROTATE};
    SendMessageW(toolbar_, TB_SETIMAGELIST, 0, reinterpret_cast<LPARAM>(images_));
    SendMessageW(toolbar_, TB_SETBITMAPSIZE, 0, MAKELPARAM(px(24), px(24)));
    SendMessageW(toolbar_, TB_SETBUTTONSIZE, 0, MAKELPARAM(px(38), px(36)));
    for (int i = 0; i < 8; ++i) {
        HICON icon = static_cast<HICON>(LoadImageW(instance_, MAKEINTRESOURCEW(icons[i]), IMAGE_ICON, px(24), px(24), 0));
        ImageList_AddIcon(images_, icon); DestroyIcon(icon);
        TBBUTTON button{};
        button.iBitmap = i; button.idCommand = commands[i]; button.fsState = TBSTATE_ENABLED;
        button.fsStyle = BTNS_BUTTON;
        SendMessageW(toolbar_, TB_ADDBUTTONSW, 1, reinterpret_cast<LPARAM>(&button));
        if (i == 0 || i == 2 || i == 4) {
            TBBUTTON separator{}; separator.fsStyle = BTNS_SEP; separator.iBitmap = px(12);
            SendMessageW(toolbar_, TB_ADDBUTTONSW, 1, reinterpret_cast<LPARAM>(&separator));
        }
    }
    SendMessageW(toolbar_, TB_AUTOSIZE, 0, 0);
}
void MainWindow::layout() {
    if (!canvas_) return;
    RECT area{}; GetClientRect(window_, &area);
    SendMessageW(status_, WM_SIZE, 0, 0);
    RECT statusArea{}; GetWindowRect(status_, &statusArea);
    const int statusHeight = statusArea.bottom - statusArea.top;
    const int header = px(48);
    const int toolbarWidth = px(365);
    MoveWindow(toolbar_, px(8), px(6), toolbarWidth, px(38), TRUE);
    MoveWindow(pageEdit_, toolbarWidth + px(24), px(12), px(64), px(26), TRUE);
    MoveWindow(canvas_, 0, header, area.right, std::max(1L, area.bottom - header - statusHeight), TRUE);
    int parts[] = {std::max(px(200), static_cast<int>(area.right) - px(235)), -1};
    SendMessageW(status_, SB_SETPARTS, 2, reinterpret_cast<LPARAM>(parts));
    updateScrollbars();
}
void MainWindow::openDialog() {
    std::vector<wchar_t> path(32768);
    OPENFILENAMEW dialog{}; dialog.lStructSize = sizeof(dialog);
    dialog.hwndOwner = window_; dialog.lpstrFile = path.data(); dialog.nMaxFile = static_cast<DWORD>(path.size());
    dialog.lpstrFilter = L"PDF-dokumentumok (*.pdf)\0*.pdf\0Minden fájl (*.*)\0*.*\0\0";
    dialog.lpstrTitle = L"PDF megnyitása";
    dialog.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_EXPLORER | OFN_NOCHANGEDIR;
    if (GetOpenFileNameW(&dialog)) open(path.data());
    else if (CommDlgExtendedError()) MessageBoxW(window_, L"A fájlválasztó nem nyitható meg.", L"sPDF", MB_OK | MB_ICONERROR);
}
void MainWindow::open(const std::wstring& path) {
    request_ = RenderRequest{}; request_.openPath = path;
    opening_ = true; scrollX_ = scrollY_ = 0;
    requestRender();
}
void MainWindow::requestRender() {
    if (!renderer_) return;
    RECT viewport{}; GetClientRect(canvas_, &viewport);
    request_.viewportWidth = viewport.right; request_.viewportHeight = viewport.bottom;
    busy_ = true;
    generation_ = renderer_->submit(request_);
    updateControls();
}
void MainWindow::acceptResults() {
    for (auto& result : renderer_->takeResults()) {
        if (result.generation != generation_) continue;
        busy_ = false;
        if (!result.error.empty()) {
            if (smokeTest_) { DestroyWindow(window_); PostQuitMessage(2); return; }
            if (result.errorCode == FPDF_ERR_PASSWORD && request_.openPath) {
                std::wstring password;
                if (DialogBoxParamW(instance_, MAKEINTRESOURCEW(IDD_PASSWORD), window_, passwordProc,
                        reinterpret_cast<LPARAM>(&password)) == IDOK) {
                    request_.password = utf8(password);
                    SecureZeroMemory(password.data(), password.size() * sizeof(wchar_t));
                    requestRender(); return;
                }
            } else MessageBoxW(window_, result.error.c_str(), L"sPDF — Megnyitási vagy megjelenítési hiba", MB_OK | MB_ICONERROR);
            request_.page = displayed_.page;
            request_.openPath.reset(); request_.password.clear(); opening_ = false;
            updateControls(); continue;
        }
        request_.openPath.reset();
        SecureZeroMemory(request_.password.data(), request_.password.size()); request_.password.clear();
        opening_ = false;
        displayed_ = std::move(result);
        if (displayed_.closed) { displayed_ = RenderResult{}; request_ = RenderRequest{}; }
        updateScrollbars(); updateControls();
        InvalidateRect(canvas_, nullptr, FALSE);
        if (smokeTest_ && !displayed_.pixels.empty()) {
            UpdateWindow(canvas_); DestroyWindow(window_); return;
        }
    }
}
void MainWindow::updateControls() {
    if (!status_) return;
    const bool loaded = displayed_.pageCount > 0 && !opening_;
    const int actions[] = {IDM_CLOSE, IDM_FIRST, IDM_LAST, IDM_PREVIOUS, IDM_NEXT, IDM_ZOOMIN,
        IDM_ZOOMOUT, IDM_FITPAGE, IDM_FITWIDTH, IDM_ACTUALSIZE, IDM_ROTATE};
    for (int action : actions) {
        bool enable = loaded;
        if (action == IDM_FIRST || action == IDM_PREVIOUS) enable = loaded && request_.page > 0;
        if (action == IDM_LAST || action == IDM_NEXT) enable = loaded && request_.page + 1 < displayed_.pageCount;
        EnableMenuItem(GetMenu(window_), action, MF_BYCOMMAND | (enable ? MF_ENABLED : MF_GRAYED));
        SendMessageW(toolbar_, TB_ENABLEBUTTON, action, MAKELONG(enable, 0));
    }
    EnableWindow(pageEdit_, loaded);
    SetWindowTextW(pageEdit_, std::to_wstring(request_.page + 1).c_str());
    const auto title = displayed_.path.empty() ? L"sPDF Megtekintő — 0.1.0 előzetes" : filename(displayed_.path) + L" — sPDF Megtekintő";
    SetWindowTextW(window_, title.c_str());
    std::wstring status = busy_ ? L"Betöltés / megjelenítés…" : L"PDF megnyitása: Ctrl+O vagy húzza ide a fájlt";
    if (loaded && !busy_) {
        status = L"Oldal: " + std::to_wstring(displayed_.page + 1) + L" / " + std::to_wstring(displayed_.pageCount) +
            L"   •   " + std::to_wstring(static_cast<int>(displayed_.pageWidth * 25.4 / 72)) + L" × " +
            std::to_wstring(static_cast<int>(displayed_.pageHeight * 25.4 / 72)) + L" mm";
        if (displayed_.limited) status += L"   •   Korlátozott renderfelbontás";
    }
    SendMessageW(status_, SB_SETTEXTW, 0, reinterpret_cast<LPARAM>(status.c_str()));
    const auto info = displayed_.pageCount ? std::to_wstring(static_cast<int>(std::round(displayed_.zoom * 100))) +
        L"%   •   " + std::to_wstring((displayed_.fileBytes + 1023) / 1024) + L" KiB" : L"0.1.0 előzetes";
    SendMessageW(status_, SB_SETTEXTW, 1, reinterpret_cast<LPARAM>(info.c_str()));
    CheckMenuRadioItem(GetMenu(window_), IDM_FITPAGE, IDM_ACTUALSIZE,
        request_.mode == ZoomMode::FitPage ? IDM_FITPAGE : request_.mode == ZoomMode::FitWidth ? IDM_FITWIDTH : IDM_ACTUALSIZE, MF_BYCOMMAND);
}
void MainWindow::command(int id) {
    if (id == IDM_OPEN) { openDialog(); return; }
    if (id == IDM_EXIT) { DestroyWindow(window_); return; }
    if (id == IDM_ABOUT) {
        TASKDIALOGCONFIG about{}; about.cbSize = sizeof(about);
        about.hwndParent = window_; about.dwFlags = TDF_ENABLE_HYPERLINKS | TDF_ALLOW_DIALOG_CANCELLATION;
        about.dwCommonButtons = TDCBF_CLOSE_BUTTON; about.pszWindowTitle = L"Névjegy";
        about.pszMainInstruction = L"sPDF Megtekintő — 0.1.0 előzetes";
        about.pszContent = L"Könnyű, gyors PDF-megtekintő Windowsra.\n\n<a href=\"https://www.edutech.hu\">www.edutech.hu</a>\n\n<a href=\"licenses\">Licencek és köszönetnyilvánítás</a>\n\nFejlesztői változat. A Windows 10 1607 kompatibilitása még nem igazolt.";
        about.lpCallbackData = reinterpret_cast<LONG_PTR>(this);
        about.pfCallback = [](HWND dialog, UINT notification, WPARAM, LPARAM lparam, LONG_PTR context) -> HRESULT {
            if (notification == TDN_HYPERLINK_CLICKED) {
                const auto* url = reinterpret_cast<const wchar_t*>(lparam);
                if (std::wstring(url) == L"licenses") reinterpret_cast<MainWindow*>(context)->command(IDM_LICENSES);
                else if (std::wstring(url) == L"https://www.edutech.hu") ShellExecuteW(dialog, L"open", url, nullptr, nullptr, SW_SHOWNORMAL);
            }
            return S_OK;
        };
        TaskDialogIndirect(&about, nullptr, nullptr, nullptr); return;
    }
    if (id == IDM_LICENSES) {
        const auto path = PdfiumLibrary::executableDirectory() + L"\\licenses";
        if (reinterpret_cast<INT_PTR>(ShellExecuteW(window_, L"open", path.c_str(), nullptr, nullptr, SW_SHOWNORMAL)) <= 32)
            MessageBoxW(window_, L"PDFium: BSD és harmadik felek licencei.\nKDE Modern Classic: LGPL-3.0-or-later, Shaun Belcher.\nA teljes csomag licenses mappájában találhatók a licencszövegek.", L"Licencek", MB_OK);
        return;
    }
    if (id == IDM_SHORTCUTS) {
        MessageBoxW(window_, L"Ctrl+O: PDF megnyitása\nCtrl+W: dokumentum bezárása\nPgUp / PgDn: lapozás\nCtrl+Home / Ctrl+End: első / utolsó oldal\nCtrl++ / Ctrl+- / Ctrl+görgő: nagyítás\nEnter az oldalszámmezőben: ugrás\nGörgő: görgetés; az oldal szélén lapozás\nAlt+F4: kilépés", L"Billentyűparancsok", MB_OK); return;
    }
    if (!displayed_.pageCount || opening_) return;
    switch (id) {
    case IDM_CLOSE: request_ = RenderRequest{}; request_.close = true; break;
    case IDM_PREVIOUS: if (request_.page > 0) --request_.page; break;
    case IDM_NEXT: if (request_.page + 1 < displayed_.pageCount) ++request_.page; break;
    case IDM_FIRST: request_.page = 0; break;
    case IDM_LAST: request_.page = displayed_.pageCount - 1; break;
    case IDM_ZOOMIN: case IDM_ZOOMOUT:
        request_.zoom = std::clamp((busy_ && request_.mode == ZoomMode::Percent ? request_.zoom : displayed_.zoom) *
            (id == IDM_ZOOMIN ? 1.25 : 0.8), 0.1, 4.0);
        request_.mode = ZoomMode::Percent; break;
    case IDM_FITPAGE: request_.mode = ZoomMode::FitPage; break;
    case IDM_FITWIDTH: request_.mode = ZoomMode::FitWidth; break;
    case IDM_ACTUALSIZE: request_.mode = ZoomMode::Percent; request_.zoom = 1; break;
    case IDM_ROTATE: request_.rotation = (request_.rotation + 1) % 4; break;
    default: return;
    }
    scrollX_ = scrollY_ = 0; requestRender();
}
void MainWindow::updateScrollbars() {
    if (!canvas_) return;
    RECT rect{}; GetClientRect(canvas_, &rect);
    scrollX_ = std::clamp(scrollX_, 0, std::max(0, displayed_.width + px(40) - static_cast<int>(rect.right)));
    scrollY_ = std::clamp(scrollY_, 0, std::max(0, displayed_.height + px(40) - static_cast<int>(rect.bottom)));
    SCROLLINFO horizontal{}; horizontal.cbSize = sizeof(horizontal); horizontal.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
    horizontal.nMax = displayed_.width + px(40) - 1;
    horizontal.nPage = rect.right; horizontal.nPos = scrollX_;
    SetScrollInfo(canvas_, SB_HORZ, &horizontal, TRUE);
    SCROLLINFO vertical{}; vertical.cbSize = sizeof(vertical); vertical.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
    vertical.nMax = displayed_.height + px(40) - 1;
    vertical.nPage = rect.bottom; vertical.nPos = scrollY_;
    SetScrollInfo(canvas_, SB_VERT, &vertical, TRUE);
}
void MainWindow::scrollBy(int x, int y) {
    scrollX_ += x; scrollY_ += y; updateScrollbars(); InvalidateRect(canvas_, nullptr, FALSE);
}
void MainWindow::scroll(int bar, int code, int thumb) {
    SCROLLINFO info{}; info.cbSize = sizeof(info); info.fMask = SIF_ALL; GetScrollInfo(canvas_, bar, &info);
    int next = info.nPos;
    switch (code) {
    case SB_LINEUP: next -= px(40); break;
    case SB_LINEDOWN: next += px(40); break;
    case SB_PAGEUP: next -= static_cast<int>(info.nPage); break;
    case SB_PAGEDOWN: next += static_cast<int>(info.nPage); break;
    case SB_THUMBTRACK: next = info.nTrackPos; break;
    case SB_THUMBPOSITION: next = thumb; break;
    case SB_TOP: next = 0; break;
    case SB_BOTTOM: next = info.nMax; break;
    default: return;
    }
    if (bar == SB_HORZ) scrollX_ = next; else scrollY_ = next;
    updateScrollbars(); InvalidateRect(canvas_, nullptr, FALSE);
}
void MainWindow::paintCanvas() {
    PAINTSTRUCT paint{}; HDC dc = BeginPaint(canvas_, &paint);
    RECT rect{}; GetClientRect(canvas_, &rect);
    HBRUSH workspace = CreateSolidBrush(RGB(230, 233, 238));
    FillRect(dc, &rect, workspace); DeleteObject(workspace);
    if (displayed_.pixels.empty()) {
        SetBkMode(dc, TRANSPARENT); SetTextColor(dc, RGB(90, 100, 116));
        const auto previous = SelectObject(dc, font_);
        DrawTextW(dc, L"sPDF Megtekintő\n\nNyisson meg egy PDF-dokumentumot\nCtrl+O vagy húzza a fájlt az ablakba", -1, &rect, DT_CENTER | DT_WORDBREAK | DT_NOPREFIX);
        SelectObject(dc, previous);
    } else {
        const int x = std::max(px(20), static_cast<int>((rect.right - displayed_.width) / 2)) - scrollX_;
        const int y = std::max(px(20), static_cast<int>((rect.bottom - displayed_.height) / 2)) - scrollY_;
        RECT shadow{x + px(3), y + px(3), x + displayed_.width + px(3), y + displayed_.height + px(3)};
        HBRUSH brush = CreateSolidBrush(RGB(194, 199, 207)); FillRect(dc, &shadow, brush); DeleteObject(brush);
        BITMAPINFO bitmap{};
        bitmap.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bitmap.bmiHeader.biWidth = displayed_.width; bitmap.bmiHeader.biHeight = -displayed_.height;
        bitmap.bmiHeader.biPlanes = 1; bitmap.bmiHeader.biBitCount = 32; bitmap.bmiHeader.biCompression = BI_RGB;
        SetDIBitsToDevice(dc, x, y, displayed_.width, displayed_.height, 0, 0, 0, displayed_.height,
            displayed_.pixels.data(), &bitmap, DIB_RGB_COLORS);
    }
    EndPaint(canvas_, &paint);
}
LRESULT CALLBACK MainWindow::pageProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam, UINT_PTR, DWORD_PTR context) {
    auto* self = reinterpret_cast<MainWindow*>(context);
    if (message == WM_KEYDOWN && wparam == VK_RETURN) {
        wchar_t buffer[16]{}; GetWindowTextW(hwnd, buffer, 16);
        const long page = std::wcstol(buffer, nullptr, 10);
        if (page > 0 && page <= self->displayed_.pageCount) {
            self->request_.page = static_cast<int>(page - 1);
            self->scrollX_ = self->scrollY_ = 0;
            self->requestRender(); SetFocus(self->canvas_);
        } else { MessageBeep(MB_ICONWARNING); self->updateControls(); }
        return 0;
    }
    return DefSubclassProc(hwnd, message, wparam, lparam);
}
LRESULT CALLBACK MainWindow::canvasProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
    auto* self = reinterpret_cast<MainWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (message == WM_NCCREATE) {
        self = static_cast<MainWindow*>(reinterpret_cast<CREATESTRUCTW*>(lparam)->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
    }
    if (!self) return DefWindowProcW(hwnd, message, wparam, lparam);
    switch (message) {
    case WM_PAINT: self->paintCanvas(); return 0;
    case WM_ERASEBKGND: return 1;
    case WM_LBUTTONDOWN: SetFocus(hwnd); return 0;
    case WM_HSCROLL: self->scroll(SB_HORZ, LOWORD(wparam), HIWORD(wparam)); return 0;
    case WM_VSCROLL: self->scroll(SB_VERT, LOWORD(wparam), HIWORD(wparam)); return 0;
    case WM_MOUSEWHEEL:
        if (GET_KEYSTATE_WPARAM(wparam) & MK_CONTROL) self->command(GET_WHEEL_DELTA_WPARAM(wparam) > 0 ? IDM_ZOOMIN : IDM_ZOOMOUT);
        else {
            const int delta = GET_WHEEL_DELTA_WPARAM(wparam);
            const int before = self->scrollY_;
            self->scrollBy(0, -delta * self->px(60) / WHEEL_DELTA);
            if (self->scrollY_ == before && !self->busy_) self->command(delta > 0 ? IDM_PREVIOUS : IDM_NEXT);
        }
        return 0;
    }
    return DefWindowProcW(hwnd, message, wparam, lparam);
}
LRESULT MainWindow::message(UINT message, WPARAM wparam, LPARAM lparam) {
    switch (message) {
    case WM_CREATE: makeControls(); return 0;
    case WM_GETMINMAXINFO: {
        auto* info = reinterpret_cast<MINMAXINFO*>(lparam);
        info->ptMinTrackSize = {px(560), px(360)}; return 0;
    }
    case WM_SIZE:
        if (wparam != SIZE_MINIMIZED) {
            layout();
            if (renderer_ && (displayed_.pageCount || opening_)) SetTimer(window_, ResizeTimer, 160, nullptr);
        }
        return 0;
    case WM_TIMER:
        if (wparam == ResizeTimer) {
            KillTimer(window_, ResizeTimer);
            if (displayed_.pageCount || opening_) requestRender();
        } else if (wparam == SmokeTimer) { DestroyWindow(window_); PostQuitMessage(3); }
        return 0;
    case WM_DPICHANGED: {
        dpi_ = HIWORD(wparam); rebuildToolbar();
        const auto* rect = reinterpret_cast<RECT*>(lparam);
        SetWindowPos(window_, nullptr, rect->left, rect->top, rect->right - rect->left, rect->bottom - rect->top, SWP_NOZORDER | SWP_NOACTIVATE);
        layout(); updateControls(); return 0;
    }
    case WM_COMMAND: if (HIWORD(wparam) == 0 || HIWORD(wparam) == 1) command(LOWORD(wparam)); return 0;
    case WM_NOTIFY: {
        const auto* header = reinterpret_cast<NMHDR*>(lparam);
        if (header->code == TBN_GETINFOTIPW) {
            auto* tip = reinterpret_cast<NMTBGETINFOTIPW*>(lparam);
            const wchar_t* text = L"";
            switch (tip->iItem) {
            case IDM_OPEN: text = L"PDF megnyitása (Ctrl+O)"; break;
            case IDM_PREVIOUS: text = L"Előző oldal (PgUp)"; break;
            case IDM_NEXT: text = L"Következő oldal (PgDn)"; break;
            case IDM_ZOOMIN: text = L"Nagyítás"; break;
            case IDM_ZOOMOUT: text = L"Kicsinyítés"; break;
            case IDM_FITPAGE: text = L"Teljes oldal"; break;
            case IDM_FITWIDTH: text = L"Oldalszélességhez illesztés"; break;
            case IDM_ROTATE: text = L"Forgatás jobbra — csak megjelenítés"; break;
            }
            lstrcpynW(tip->pszText, text, tip->cchTextMax); return 0;
        }
        break;
    }
    case WM_DROPFILES: {
        HDROP drop = reinterpret_cast<HDROP>(wparam);
        UINT length = DragQueryFileW(drop, 0, nullptr, 0);
        std::vector<wchar_t> path(length + 1);
        if (length && DragQueryFileW(drop, 0, path.data(), length + 1)) open(path.data());
        DragFinish(drop); return 0;
    }
    case WM_CTLCOLORSTATIC: SetBkMode(reinterpret_cast<HDC>(wparam), TRANSPARENT); return reinterpret_cast<LRESULT>(background_);
    case PdfRenderer::ResultMessage: acceptResults(); return 0;
    case WM_CLOSE: DestroyWindow(window_); return 0;
    case WM_DESTROY:
        KillTimer(window_, ResizeTimer); KillTimer(window_, SmokeTimer);
        renderer_.reset(); PostQuitMessage(0); return 0;
    }
    return DefWindowProcW(window_, message, wparam, lparam);
}
}
