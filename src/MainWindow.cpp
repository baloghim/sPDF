#include "MainWindow.h"
#include "resource.h"
#include <commdlg.h>
#include <shellapi.h>
#include <uxtheme.h>
#include <algorithm>
#include <cmath>
#include <cwchar>
#include <cstring>

namespace spdf {
namespace {
constexpr wchar_t WindowClass[] = L"sPDF.MainWindow";
constexpr wchar_t CanvasClass[] = L"sPDF.Canvas";
constexpr UINT_PTR ResizeTimer = 1, SmokeTimer = 2;
constexpr UINT PasswordMessage = WM_APP + 20;
std::wstring filename(const std::wstring& path) { return path.substr(path.find_last_of(L"\\/") + 1); }
struct PasswordContext { std::wstring path, value; };
INT_PTR CALLBACK passwordProc(HWND dialog, UINT message, WPARAM wparam, LPARAM lparam) {
    if (message == WM_INITDIALOG) {
        SetWindowLongPtrW(dialog, DWLP_USER, lparam);
        const auto* context = reinterpret_cast<PasswordContext*>(lparam);
        SetWindowTextW(dialog, (L"Jelszóval védett PDF — " + filename(context->path)).c_str());
        SendDlgItemMessageW(dialog, IDC_PASSWORD, EM_SETLIMITTEXT, 255, 0);
        SetFocus(GetDlgItem(dialog, IDC_PASSWORD)); return FALSE;
    }
    if (message == WM_COMMAND) {
        if (LOWORD(wparam) == IDOK) {
            auto* context = reinterpret_cast<PasswordContext*>(GetWindowLongPtrW(dialog, DWLP_USER));
            wchar_t buffer[256]{};
            GetDlgItemTextW(dialog, IDC_PASSWORD, buffer, 256);
            context->value = buffer; SecureZeroMemory(buffer, sizeof(buffer));
            EndDialog(dialog, IDOK); return TRUE;
        }
        if (LOWORD(wparam) == IDCANCEL) { EndDialog(dialog, IDCANCEL); return TRUE; }
    }
    return FALSE;
}
std::string utf8(const std::wstring& text) {
    if (text.empty()) return {};
    const int count = WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0, nullptr, nullptr);
    std::string output(count, '\0');
    WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), output.data(), count, nullptr, nullptr);
    return output;
}
}
MainWindow::~MainWindow() {
    renderer_.reset();
    if (images_) ImageList_Destroy(images_);
    if (font_) DeleteObject(font_);
    if (headingFont_) DeleteObject(headingFont_);
    if (welcomeIcon_) DestroyIcon(welcomeIcon_);
    if (searchIcon_) DestroyIcon(searchIcon_);
    if (background_) DeleteObject(background_);
    if (noticeBrush_) DeleteObject(noticeBrush_);
}
bool MainWindow::create(int show, const std::vector<std::wstring>& paths, bool smoke) {
    smokeTest_ = smoke;
    WNDCLASSEXW wc{}; wc.cbSize = sizeof(wc);
    wc.hInstance = instance_; wc.lpfnWndProc = windowProc; wc.lpszClassName = WindowClass;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hIcon = LoadIconW(instance_, MAKEINTRESOURCEW(IDI_APP)); wc.hIconSm = wc.hIcon;
    wc.lpszMenuName = MAKEINTRESOURCEW(IDR_MENU);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    if (!RegisterClassExW(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) return false;
    wc.lpfnWndProc = canvasProc; wc.lpszClassName = CanvasClass;
    wc.lpszMenuName = nullptr; wc.hbrBackground = nullptr;
    if (!RegisterClassExW(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) return false;
    window_ = CreateWindowExW(WS_EX_ACCEPTFILES, WindowClass, L"sPDF Megtekintő — 0.2.0 előzetes",
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, CW_USEDEFAULT, CW_USEDEFAULT, 1180, 740,
        nullptr, nullptr, instance_, this);
    if (!window_) return false;
    renderer_ = std::make_unique<PdfRenderer>(window_);
    ShowWindow(window_, show); UpdateWindow(window_);
    for (const auto& path : paths) open(path);
    if (smokeTest_) SetTimer(window_, SmokeTimer, 30000, nullptr);
    return true;
}
LRESULT CALLBACK MainWindow::windowProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
    auto* self = reinterpret_cast<MainWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (message == WM_NCCREATE) {
        self = static_cast<MainWindow*>(reinterpret_cast<CREATESTRUCTW*>(lparam)->lpCreateParams);
        self->window_ = hwnd; SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
    }
    return self ? self->message(message, wparam, lparam) : DefWindowProcW(hwnd, message, wparam, lparam);
}
void MainWindow::makeControls() {
    HDC dc = GetDC(window_); dpi_ = GetDeviceCaps(dc, LOGPIXELSX); ReleaseDC(window_, dc);
    using GetWindowDpi = UINT(WINAPI*)(HWND);
    auto address = GetProcAddress(GetModuleHandleW(L"user32.dll"), "GetDpiForWindow");
    if (address) {
        GetWindowDpi getDpi = nullptr; static_assert(sizeof(getDpi) == sizeof(address));
        std::memcpy(&getDpi, &address, sizeof(getDpi));
        if (const UINT dpi = getDpi(window_)) dpi_ = static_cast<int>(dpi);
    }
    background_ = CreateSolidBrush(RGB(255, 255, 255));
    noticeBrush_ = CreateSolidBrush(RGB(255, 245, 225));
    toolbar_ = CreateWindowExW(0, TOOLBARCLASSNAMEW, nullptr, WS_CHILD | WS_VISIBLE |
        TBSTYLE_FLAT | TBSTYLE_TOOLTIPS | TBSTYLE_LIST | CCS_NORESIZE | CCS_NOPARENTALIGN | CCS_NODIVIDER,
        0, 0, 0, 0, window_, reinterpret_cast<HMENU>(IDC_TOOLBAR), instance_, nullptr);
    SendMessageW(toolbar_, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
    SendMessageW(toolbar_, TB_SETUNICODEFORMAT, TRUE, 0);
    SendMessageW(toolbar_, TB_SETEXTENDEDSTYLE, 0, TBSTYLE_EX_MIXEDBUTTONS);
    status_ = CreateWindowExW(0, STATUSCLASSNAMEW, nullptr, WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,
        0, 0, 0, 0, window_, reinterpret_cast<HMENU>(IDC_STATUS), instance_, nullptr);
    canvas_ = CreateWindowExW(0, CanvasClass, nullptr, WS_CHILD | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL,
        0, 0, 0, 0, window_, nullptr, instance_, this);
    tabBar_.create(window_, instance_, IDC_TABS);
    pageEdit_ = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"1", WS_CHILD | WS_VISIBLE | WS_TABSTOP |
        ES_NUMBER | ES_CENTER | ES_AUTOHSCROLL, 0, 0, 0, 0, window_, reinterpret_cast<HMENU>(IDC_PAGE), instance_, nullptr);
    pageTotal_ = CreateWindowExW(0, L"STATIC", L"/ —", WS_CHILD | WS_VISIBLE | SS_CENTERIMAGE,
        0, 0, 0, 0, window_, nullptr, instance_, nullptr);
    search_ = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"Keresés — hamarosan", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
        0, 0, 0, 0, window_, reinterpret_cast<HMENU>(IDC_SEARCH), instance_, nullptr);
    SendMessageW(search_, EM_SETCUEBANNER, TRUE, reinterpret_cast<LPARAM>(L"Keresés — hamarosan"));
    EnableWindow(search_, FALSE);
    viewButton_ = CreateWindowExW(0, L"BUTTON", L"Egy oldal  ▼", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
        0, 0, 0, 0, window_, reinterpret_cast<HMENU>(IDM_VIEWMENU), instance_, nullptr);
    noticeLabel_ = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | SS_LEFT,
        0, 0, 0, 0, window_, reinterpret_cast<HMENU>(IDC_NOTICE), instance_, nullptr);
    noticeClose_ = CreateWindowExW(0, L"BUTTON", L"Bezárás", WS_CHILD | WS_TABSTOP | BS_PUSHBUTTON,
        0, 0, 0, 0, window_, reinterpret_cast<HMENU>(IDM_DISMISS_NOTICE), instance_, nullptr);
    SendMessageW(pageEdit_, EM_SETLIMITTEXT, 7, 0);
    SetWindowSubclass(pageEdit_, pageProc, 1, reinterpret_cast<DWORD_PTR>(this));
    SetWindowTheme(toolbar_, L"Explorer", nullptr); SetWindowTheme(viewButton_, L"Explorer", nullptr);
    welcomeIcon_ = static_cast<HICON>(LoadImageW(instance_, MAKEINTRESOURCEW(IDI_APP), IMAGE_ICON, 128, 128, 0));
    searchIcon_ = static_cast<HICON>(LoadImageW(instance_, MAKEINTRESOURCEW(IDI_SEARCH), IMAGE_ICON, 32, 32, 0));
    rebuildToolbar(); updateControls();
}
void MainWindow::rebuildToolbar() {
    if (font_) DeleteObject(font_);
    if (headingFont_) DeleteObject(headingFont_);
    font_ = CreateFontW(-px(12), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    headingFont_ = CreateFontW(-px(16), 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    for (HWND control : {pageEdit_, pageTotal_, search_, status_, toolbar_, viewButton_, noticeLabel_, noticeClose_})
        SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(font_), TRUE);
    tabBar_.style(font_, dpi_);
    while (SendMessageW(toolbar_, TB_BUTTONCOUNT, 0, 0)) SendMessageW(toolbar_, TB_DELETEBUTTON, 0, 0);
    SendMessageW(toolbar_, TB_SETIMAGELIST, 0, 0);
    if (images_) ImageList_Destroy(images_);
    images_ = ImageList_Create(px(24), px(24), ILC_COLOR32 | ILC_MASK, 9, 0);
    const int icons[] = {IDI_OPEN, IDI_PRINT, IDI_PREVIOUS, IDI_NEXT, IDI_ZOOMOUT, IDI_ZOOMIN, IDI_FITPAGE, IDI_FITWIDTH, IDI_ROTATE};
    for (int id : icons) {
        HICON icon = static_cast<HICON>(LoadImageW(instance_, MAKEINTRESOURCEW(id), IMAGE_ICON, px(24), px(24), 0));
        ImageList_AddIcon(images_, icon); DestroyIcon(icon);
    }
    SendMessageW(toolbar_, TB_SETIMAGELIST, 0, reinterpret_cast<LPARAM>(images_));
    SendMessageW(toolbar_, TB_SETBITMAPSIZE, 0, MAKELPARAM(px(24), px(24)));
    SendMessageW(toolbar_, TB_SETBUTTONSIZE, 0, MAKELPARAM(px(42), px(42)));
    auto button = [&](int image, int id, const wchar_t* label = nullptr) {
        TBBUTTON item{}; item.iBitmap = image; item.idCommand = id;
        item.fsState = id == IDM_PRINT ? 0 : TBSTATE_ENABLED;
        item.fsStyle = BTNS_BUTTON;
        if (id == IDM_FITPAGE || id == IDM_FITWIDTH) item.fsStyle |= BTNS_CHECK;
        if (label) { item.fsStyle |= BTNS_SHOWTEXT | BTNS_AUTOSIZE; item.iString = reinterpret_cast<INT_PTR>(label); }
        SendMessageW(toolbar_, TB_ADDBUTTONSW, 1, reinterpret_cast<LPARAM>(&item));
    };
    auto separator = [&](int width, int id = 0) {
        TBBUTTON item{}; item.fsStyle = BTNS_SEP; item.iBitmap = px(width); item.idCommand = id;
        SendMessageW(toolbar_, TB_ADDBUTTONSW, 1, reinterpret_cast<LPARAM>(&item));
    };
    button(0, IDM_OPEN, L"Megnyitás"); button(1, IDM_PRINT); separator(14);
    button(2, IDM_PREVIOUS); separator(100, ID_SEPARATOR_PAGE); button(3, IDM_NEXT); separator(14);
    button(4, IDM_ZOOMOUT); button(5, IDM_ZOOMIN); separator(14);
    button(6, IDM_FITPAGE); button(7, IDM_FITWIDTH); button(8, IDM_ROTATE);
    for (int id : {IDM_PRINT, IDM_PREVIOUS, IDM_NEXT, IDM_ZOOMOUT, IDM_ZOOMIN, IDM_FITPAGE, IDM_FITWIDTH, IDM_ROTATE}) {
        TBBUTTONINFOW info{}; info.cbSize = sizeof(info); info.dwMask = TBIF_SIZE; info.cx = static_cast<WORD>(px(42));
        SendMessageW(toolbar_, TB_SETBUTTONINFOW, id, reinterpret_cast<LPARAM>(&info));
    }
    SendMessageW(toolbar_, TB_AUTOSIZE, 0, 0);
}
void MainWindow::layout() {
    if (!canvas_) return;
    RECT area{}; GetClientRect(window_, &area);
    SendMessageW(status_, WM_SIZE, 0, 0);
    RECT statusArea{}; GetWindowRect(status_, &statusArea);
    const int statusHeight = statusArea.bottom - statusArea.top;
    SIZE toolbarSize{}; SendMessageW(toolbar_, TB_GETMAXSIZE, 0, reinterpret_cast<LPARAM>(&toolbarSize));
    const int top = px(42), toolbarHeight = px(52), tabHeight = px(40);
    const bool compact = area.right < toolbarSize.cx + px(210);
    MoveWindow(search_, std::max(px(300), static_cast<int>(area.right) - px(272)), px(8), px(254), px(27), TRUE);
    MoveWindow(toolbar_, px(10), top + px(4), toolbarSize.cx, px(44), TRUE);
    const int pageIndex = static_cast<int>(SendMessageW(toolbar_, TB_COMMANDTOINDEX, ID_SEPARATOR_PAGE, 0));
    RECT pageRect{}; SendMessageW(toolbar_, TB_GETITEMRECT, pageIndex, reinterpret_cast<LPARAM>(&pageRect));
    MoveWindow(pageEdit_, px(10) + pageRect.left + px(5), top + px(12), px(46), px(25), TRUE);
    MoveWindow(pageTotal_, px(10) + pageRect.left + px(55), top + px(12), px(43), px(25), TRUE);
    const int tabsTop = top + toolbarHeight;
    MoveWindow(tabBar_.handle(), 0, tabsTop, area.right, tabHeight, TRUE); tabBar_.resize(area.right);
    const int extra = compact ? px(36) : 0;
    MoveWindow(viewButton_, compact ? px(14) : area.right - px(186), compact ? tabsTop + tabHeight + px(3) : top + px(11), px(168), px(28), TRUE);
    const int noticeTop = tabsTop + tabHeight + extra;
    const int noticeHeight = notice_.empty() ? 0 : px(52);
    ShowWindow(noticeLabel_, notice_.empty() ? SW_HIDE : SW_SHOW);
    ShowWindow(noticeClose_, notice_.empty() ? SW_HIDE : SW_SHOW);
    if (noticeHeight) {
        MoveWindow(noticeLabel_, px(18), noticeTop + px(9), std::max(px(100), static_cast<int>(area.right) - px(122)), px(36), TRUE);
        MoveWindow(noticeClose_, area.right - px(92), noticeTop + px(12), px(76), px(28), TRUE);
    }
    const int canvasTop = noticeTop + noticeHeight;
    MoveWindow(canvas_, 0, canvasTop, area.right, std::max(1L, area.bottom - canvasTop - statusHeight), TRUE);
    int parts[] = {area.right * 18 / 100, area.right * 30 / 100, area.right * 49 / 100,
        area.right * 63 / 100, area.right * 81 / 100, -1};
    SendMessageW(status_, SB_SETPARTS, 6, reinterpret_cast<LPARAM>(parts));
    updateScrollbars(); InvalidateRect(window_, nullptr, FALSE);
}
void MainWindow::paintHeader() {
    PAINTSTRUCT paint{}; HDC dc = BeginPaint(window_, &paint);
    RECT client{}; GetClientRect(window_, &client); FillRect(dc, &client, background_);
    SetBkMode(dc, TRANSPARENT); SetTextColor(dc, RGB(31, 51, 78));
    auto previous = SelectObject(dc, headingFont_);
    RECT title{px(18), px(8), px(280), px(34)};
    DrawTextW(dc, L"sPDF Megtekintő", -1, &title, DT_SINGLELINE | DT_VCENTER);
    SelectObject(dc, font_); SetTextColor(dc, RGB(112, 123, 140));
    RECT count{px(184), px(10), px(410), px(32)};
    const auto text = std::to_wstring(tabs_.items().size()) + L" dokumentum";
    DrawTextW(dc, text.c_str(), -1, &count, DT_SINGLELINE | DT_VCENTER);
    RECT searchRect{}; GetWindowRect(search_, &searchRect); MapWindowPoints(nullptr, window_, reinterpret_cast<POINT*>(&searchRect), 2);
    DrawIconEx(dc, searchRect.left - px(28), px(11), searchIcon_, px(20), px(20), 0, nullptr, DI_NORMAL);
    if (!notice_.empty()) {
        RECT label{}; GetWindowRect(noticeLabel_, &label); MapWindowPoints(nullptr, window_, reinterpret_cast<POINT*>(&label), 2);
        RECT band{0, label.top - px(9), client.right, label.top + px(43)};
        FillRect(dc, &band, noticeBrush_);
        RECT accent{0, band.top, px(4), band.bottom}; HBRUSH stripe = CreateSolidBrush(RGB(211, 140, 31));
        FillRect(dc, &accent, stripe); DeleteObject(stripe);
    }
    SelectObject(dc, previous); EndPaint(window_, &paint);
}
void MainWindow::openDialog() {
    std::vector<wchar_t> buffer(65536);
    OPENFILENAMEW dialog{}; dialog.lStructSize = sizeof(dialog); dialog.hwndOwner = window_;
    dialog.lpstrFile = buffer.data(); dialog.nMaxFile = static_cast<DWORD>(buffer.size());
    dialog.lpstrFilter = L"PDF-dokumentumok (*.pdf)\0*.pdf\0Minden fájl (*.*)\0*.*\0\0";
    dialog.lpstrTitle = L"PDF-dokumentumok megnyitása";
    dialog.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_EXPLORER | OFN_NOCHANGEDIR | OFN_ALLOWMULTISELECT;
    if (GetOpenFileNameW(&dialog)) {
        const std::wstring first = buffer.data();
        const wchar_t* next = buffer.data() + first.size() + 1;
        if (!*next) open(first);
        else while (*next) { const std::wstring name = next; open(first + L"\\" + name); next += name.size() + 1; }
    } else if (CommDlgExtendedError()) MessageBoxW(window_, L"A fájlválasztó nem nyitható meg.", L"sPDF", MB_OK | MB_ICONERROR);
}
void MainWindow::syncScroll() {
    if (auto* tab = tabs_.active()) { tab->scrollX = scrollX_; tab->scrollY = scrollY_; }
}
void MainWindow::open(const std::wstring& path) {
    syncScroll();
    try {
        const auto opened = tabs_.open(path);
        showActive();
        if (opened.created) {
            RenderRequest request; request.documentId = opened.id; request.openPath = tabs_.find(opened.id)->path;
            renderer_->submit(std::move(request));
        }
    } catch (const std::exception&) {
        MessageBoxW(window_, L"A fájl elérési útja nem használható.", L"sPDF", MB_OK | MB_ICONERROR);
    }
}
void MainWindow::activate(DocumentId id) {
    if (!id || !tabs_.find(id)) return;
    syncScroll(); tabs_.select(id); showActive();
}
void MainWindow::showActive() {
    KillTimer(window_, ResizeTimer);
    renderer_->activate(tabs_.activeId()); displayed_ = RenderResult{};
    auto* tab = tabs_.active();
    // Publish readiness only after the new bitmap arrives. Cross-thread window
    // messages can observe controls while a tab switch is still being handled.
    if (tab && !tab->opening) tab->rendering = true;
    scrollX_ = tab ? tab->scrollX : 0; scrollY_ = tab ? tab->scrollY : 0;
    tabBar_.update(tabs_); updateScrollbars(); updateControls();
    InvalidateRect(window_, nullptr, FALSE); InvalidateRect(canvas_, nullptr, FALSE);
    if (tab && !tab->opening) requestRender();
}
void MainWindow::closeTab(DocumentId id) {
    const bool active = tabs_.activeId() == id;
    syncScroll();
    if (!tabs_.close(id)) return;
    RenderRequest request; request.documentId = id; request.close = true; renderer_->submit(std::move(request));
    if (active) showActive();
    else { tabBar_.update(tabs_); updateControls(); InvalidateRect(window_, nullptr, FALSE); }
}
void MainWindow::requestRender() {
    auto* tab = tabs_.active();
    if (!renderer_ || !tab || tab->opening || destroying_) return;
    RECT viewport{}; GetClientRect(canvas_, &viewport);
    RenderRequest request; request.documentId = tab->id; request.page = tab->page;
    request.rotation = tab->rotation; request.zoom = tab->zoom; request.mode = tab->zoomMode;
    request.deviceScale = static_cast<double>(dpi_) / 96;
    request.viewportWidth = viewport.right; request.viewportHeight = viewport.bottom;
    tab->rendering = true; tab->renderGeneration = renderer_->submit(std::move(request)); updateControls();
}
void MainWindow::promptPassword() {
    if (promptingPassword_ || destroying_) return;
    while (!passwords_.empty()) {
        const auto id = passwords_.front(); passwords_.pop_front();
        const auto* tab = tabs_.find(id);
        if (!tab || !tab->passwordPending) continue;
        PasswordContext context{tab->path, {}};
        promptingPassword_ = true;
        const auto answer = DialogBoxParamW(instance_, MAKEINTRESOURCEW(IDD_PASSWORD), window_, passwordProc, reinterpret_cast<LPARAM>(&context));
        promptingPassword_ = false;
        // A modal dialog pumps messages: reacquire by ID instead of retaining a vector pointer.
        if (tabs_.find(id) && !destroying_) {
            if (answer == IDOK) {
                RenderRequest request; request.documentId = id; request.openPath = context.path; request.password = utf8(context.value);
                tabs_.find(id)->passwordPending = false; renderer_->submit(std::move(request)); updateControls();
            } else closeTab(id);
        }
        SecureZeroMemory(context.value.data(), context.value.size() * sizeof(wchar_t));
    }
}
void MainWindow::showNotice(const std::wstring& message, bool rerender) {
    notice_ = message; SetWindowTextW(noticeLabel_, notice_.c_str()); layout();
    if (rerender) requestRender();
    InvalidateRect(window_, nullptr, FALSE);
}
void MainWindow::acceptResults() {
    for (auto& result : renderer_->takeResults()) {
        auto* tab = tabs_.find(result.documentId);
        if (!tab || result.operation == RenderOperation::Close) continue;
        if (result.operation == RenderOperation::Render &&
            (result.documentId != tabs_.activeId() || result.generation != tab->renderGeneration)) continue;
        if (!result.error.empty()) {
            tab->rendering = false;
            if (smokeTest_) { DestroyWindow(window_); PostQuitMessage(2); return; }
            if (result.operation == RenderOperation::Open && result.errorCode == FPDF_ERR_PASSWORD) {
                tab->passwordPending = true; passwords_.push_back(tab->id); updateControls();
                PostMessageW(window_, PasswordMessage, 0, 0); continue;
            }
            const auto text = result.error + L" — " + filename(tab->path);
            if (result.operation == RenderOperation::Open) closeTab(tab->id);
            else { if (displayed_.documentId == tab->id) tab->page = displayed_.page; updateControls(); }
            showNotice(text, result.operation == RenderOperation::Open); continue;
        }
        if (result.operation == RenderOperation::Open) {
            if (result.duplicateOf) {
                if (!tabs_.find(result.duplicateOf)) {
                    RenderRequest retry; retry.documentId = tab->id; retry.openPath = tab->path;
                    renderer_->submit(std::move(retry)); continue;
                }
                const bool active = tabs_.activeId() == tab->id;
                tabs_.close(tab->id);
                if (active) { tabs_.select(result.duplicateOf); showActive(); }
                else { tabBar_.update(tabs_); updateControls(); }
                continue;
            }
            tab->rendering = tab->id == tabs_.activeId();
            tab->opening = false; tab->passwordPending = false; tab->pageCount = result.pageCount; tab->fileBytes = result.fileBytes;
            tabBar_.update(tabs_); updateControls();
            if (tab->id == tabs_.activeId()) requestRender();
            continue;
        }
        tab->rendering = false; tab->renderedZoom = result.zoom; tab->limited = result.limited;
        tab->pageWidth = result.pageWidth; tab->pageHeight = result.pageHeight;
        displayed_ = std::move(result);
        updateScrollbars(); syncScroll(); updateControls(); InvalidateRect(canvas_, nullptr, FALSE);
        if (smokeTest_ && !displayed_.pixels.empty()) {
            bool loaded = true; for (const auto& item : tabs_.items()) loaded &= !item.opening;
            if (loaded) { UpdateWindow(canvas_); DestroyWindow(window_); return; }
        }
    }
}
void MainWindow::updateControls() {
    if (!status_) return;
    const auto* tab = tabs_.active(); const bool loaded = tab && !tab->opening;
    const int actions[] = {IDM_CLOSE, IDM_FIRST, IDM_LAST, IDM_PREVIOUS, IDM_NEXT, IDM_ZOOMIN, IDM_ZOOMOUT,
        IDM_FITPAGE, IDM_FITWIDTH, IDM_ACTUALSIZE, IDM_ROTATE};
    for (int action : actions) {
        bool enable = loaded;
        if (action == IDM_CLOSE) enable = tab != nullptr;
        if (action == IDM_FIRST || action == IDM_PREVIOUS) enable = loaded && tab->page > 0;
        if (action == IDM_LAST || action == IDM_NEXT) enable = loaded && tab->page + 1 < tab->pageCount;
        EnableMenuItem(GetMenu(window_), action, MF_BYCOMMAND | (enable ? MF_ENABLED : MF_GRAYED));
        SendMessageW(toolbar_, TB_ENABLEBUTTON, action, MAKELONG(enable, 0));
    }
    for (int action : {IDM_FITPAGE, IDM_FITWIDTH}) {
        const bool selected = loaded && (action == IDM_FITPAGE ? tab->zoomMode == ZoomMode::FitPage : tab->zoomMode == ZoomMode::FitWidth);
        SendMessageW(toolbar_, TB_CHECKBUTTON, action, MAKELONG(selected, 0));
    }
    EnableWindow(pageEdit_, loaded);
    SetWindowTextW(pageEdit_, tab ? std::to_wstring(tab->page + 1).c_str() : L"—");
    SetWindowTextW(pageTotal_, loaded ? (L"/ " + std::to_wstring(tab->pageCount)).c_str() : L"/ —");
    const auto title = tab ? filename(tab->path) + L" — sPDF Megtekintő 0.2.0" : L"sPDF Megtekintő — 0.2.0 előzetes";
    SetWindowTextW(window_, title.c_str());
    std::wstring fields[6] = {L"Oldal —", L"Nagyítás —", L"Papír —", L"Fájl —", L"Nézet: Egy oldal", L"Készenlét"};
    if (tab) {
        if (loaded) {
            fields[0] = L"Oldal " + std::to_wstring(tab->page + 1) + L" / " + std::to_wstring(tab->pageCount);
            fields[1] = std::to_wstring(static_cast<int>(std::round(tab->renderedZoom * 100))) + L"%";
            if (tab->pageWidth > 0) fields[2] = std::to_wstring(static_cast<int>(std::round(tab->pageWidth * 25.4 / 72))) + L" × " +
                std::to_wstring(static_cast<int>(std::round(tab->pageHeight * 25.4 / 72))) + L" mm";
            fields[3] = std::to_wstring((tab->fileBytes + 1023) / 1024) + L" KiB";
        }
        fields[5] = tab->passwordPending ? L"Jelszó szükséges" : tab->opening ? L"Betöltés…" :
            tab->rendering ? L"Megjelenítés…" : tab->limited ? L"Felbontáskorlát" : L"Kész";
    }
    for (int i = 0; i < 6; ++i) SendMessageW(status_, SB_SETTEXTW, i, reinterpret_cast<LPARAM>(fields[i].c_str()));
    for (int action : {IDM_FITPAGE, IDM_FITWIDTH, IDM_ACTUALSIZE}) {
        const bool selected = loaded && (action == IDM_FITPAGE ? tab->zoomMode == ZoomMode::FitPage :
            action == IDM_FITWIDTH ? tab->zoomMode == ZoomMode::FitWidth : tab->zoomMode == ZoomMode::Percent && tab->zoom == 1);
        CheckMenuItem(GetMenu(window_), action, MF_BYCOMMAND | (selected ? MF_CHECKED : MF_UNCHECKED));
    }
}
void MainWindow::command(int id) {
    if (id == IDM_DISMISS_NOTICE) { showNotice(L""); return; }
    if (id == IDM_OPEN) { openDialog(); return; }
    if (id == IDM_EXIT) { DestroyWindow(window_); return; }
    if (id == IDM_ABOUT) {
        TASKDIALOGCONFIG about{}; about.cbSize = sizeof(about);
        about.hwndParent = window_; about.dwFlags = TDF_ENABLE_HYPERLINKS | TDF_ALLOW_DIALOG_CANCELLATION;
        about.dwCommonButtons = TDCBF_CLOSE_BUTTON; about.pszWindowTitle = L"Névjegy";
        about.pszMainInstruction = L"sPDF Megtekintő — 0.2.0 előzetes";
        about.pszContent = L"Könnyű, gyors PDF-megtekintő Windowsra.\n\n<a href=\"https://www.edutech.hu\">www.edutech.hu</a>\n\n<a href=\"licenses\">Licencek és köszönetnyilvánítás</a>\n\nFejlesztői változat. A 0.1.0 kiadást a célrendszeren visszaigazolták; a 0.2.0 külön Windows 10 1607 próbát igényel.";
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
        MessageBoxW(window_, L"Ctrl+O: PDF megnyitása\nCtrl+W: aktuális fül bezárása\nCtrl+Tab / Ctrl+Shift+Tab: fülváltás\nPgUp / PgDn: lapozás\nCtrl+Home / Ctrl+End: első / utolsó oldal\nCtrl++ / Ctrl+- / Ctrl+görgő: nagyítás\nEnter az oldalszámmezőben: ugrás\nGörgő: görgetés; az oldal szélén lapozás\nAlt+F4: kilépés", L"Billentyűparancsok", MB_OK); return;
    }
    if (id == IDM_NEXTTAB || id == IDM_PREVIOUSTAB) {
        syncScroll(); tabs_.cycle(id == IDM_NEXTTAB ? 1 : -1); showActive(); return;
    }
    if (id == IDM_VIEWMENU) {
        HMENU popup = CreatePopupMenu();
        AppendMenuW(popup, MF_STRING | MF_CHECKED, IDM_SINGLE, L"Egy oldal");
        AppendMenuW(popup, MF_STRING | MF_GRAYED, IDM_CONTINUOUS, L"Folyamatos — hamarosan");
        AppendMenuW(popup, MF_STRING | MF_GRAYED, IDM_ORGANIZER, L"Oldalrendező — hamarosan");
        RECT rect{}; GetWindowRect(viewButton_, &rect);
        const int selected = TrackPopupMenu(popup, TPM_RETURNCMD | TPM_LEFTALIGN, rect.left, rect.bottom, 0, window_, nullptr);
        DestroyMenu(popup); if (selected == IDM_SINGLE) SetFocus(canvas_); return;
    }
    if (id == IDM_CLOSE) { closeTab(tabs_.activeId()); return; }
    auto* tab = tabs_.active();
    if (!tab || tab->opening) return;
    switch (id) {
    case IDM_PREVIOUS: if (tab->page > 0) --tab->page; break;
    case IDM_NEXT: if (tab->page + 1 < tab->pageCount) ++tab->page; break;
    case IDM_FIRST: tab->page = 0; break;
    case IDM_LAST: tab->page = tab->pageCount - 1; break;
    case IDM_ZOOMIN: case IDM_ZOOMOUT:
        tab->zoom = std::clamp((tab->rendering && tab->zoomMode == ZoomMode::Percent ? tab->zoom : tab->renderedZoom) *
            (id == IDM_ZOOMIN ? 1.25 : 0.8), 0.1, 4.0);
        tab->zoomMode = ZoomMode::Percent; break;
    case IDM_FITPAGE: tab->zoomMode = ZoomMode::FitPage; break;
    case IDM_FITWIDTH: tab->zoomMode = ZoomMode::FitWidth; break;
    case IDM_ACTUALSIZE: tab->zoomMode = ZoomMode::Percent; tab->zoom = 1; break;
    case IDM_ROTATE: tab->rotation = (tab->rotation + 1) % 4; break;
    default: return;
    }
    scrollX_ = scrollY_ = 0; syncScroll(); requestRender();
}
void MainWindow::updateScrollbars() {
    if (!canvas_) return;
    RECT rect{};
    // Scrollbars change the viewport. Settle visibility before clamping saved
    // tab offsets; otherwise restoring a bottom position loses one bar height.
    for (int pass = 0; pass < 3; ++pass) {
        GetClientRect(canvas_, &rect);
        SCROLLINFO horizontal{}; horizontal.cbSize = sizeof(horizontal); horizontal.fMask = SIF_RANGE | SIF_PAGE;
        horizontal.nMax = displayed_.width + px(40) - 1; horizontal.nPage = rect.right;
        SetScrollInfo(canvas_, SB_HORZ, &horizontal, TRUE);
        SCROLLINFO vertical{}; vertical.cbSize = sizeof(vertical); vertical.fMask = SIF_RANGE | SIF_PAGE;
        vertical.nMax = displayed_.height + px(40) - 1; vertical.nPage = rect.bottom;
        SetScrollInfo(canvas_, SB_VERT, &vertical, TRUE);
    }
    GetClientRect(canvas_, &rect);
    if (!displayed_.pixels.empty()) {
        scrollX_ = std::clamp(scrollX_, 0, std::max(0, displayed_.width + px(40) - static_cast<int>(rect.right)));
        scrollY_ = std::clamp(scrollY_, 0, std::max(0, displayed_.height + px(40) - static_cast<int>(rect.bottom)));
    }
    SCROLLINFO position{}; position.cbSize = sizeof(position); position.fMask = SIF_POS;
    position.nPos = scrollX_; SetScrollInfo(canvas_, SB_HORZ, &position, TRUE);
    position.nPos = scrollY_; SetScrollInfo(canvas_, SB_VERT, &position, TRUE);
}
void MainWindow::scrollBy(int x, int y) {
    scrollX_ += x; scrollY_ += y; updateScrollbars(); syncScroll(); InvalidateRect(canvas_, nullptr, FALSE);
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
    updateScrollbars(); syncScroll(); InvalidateRect(canvas_, nullptr, FALSE);
}
void MainWindow::paintCanvas() {
    PAINTSTRUCT paint{}; HDC dc = BeginPaint(canvas_, &paint);
    drawCanvas(dc); EndPaint(canvas_, &paint);
}
void MainWindow::drawCanvas(HDC dc) {
    RECT rect{}; GetClientRect(canvas_, &rect);
    HBRUSH workspace = CreateSolidBrush(RGB(232, 236, 242)); FillRect(dc, &rect, workspace); DeleteObject(workspace);
    if (displayed_.pixels.empty()) {
        const auto* tab = tabs_.active();
        const int center = rect.right / 2, top = std::max(px(20), static_cast<int>(rect.bottom / 2) - px(105));
        DrawIconEx(dc, center - px(28), top, welcomeIcon_, px(56), px(56), 0, nullptr, DI_NORMAL);
        SetBkMode(dc, TRANSPARENT); SetTextColor(dc, RGB(41, 61, 87));
        auto previous = SelectObject(dc, headingFont_);
        RECT label{px(20), top + px(78), rect.right - px(20), top + px(110)};
        DrawTextW(dc, tab ? L"Dokumentum betöltése…" : L"Nyisson meg egy PDF-dokumentumot", -1, &label, DT_CENTER | DT_SINGLELINE | DT_VCENTER);
        SelectObject(dc, font_); SetTextColor(dc, RGB(104, 118, 139));
        label.top += px(40); label.bottom += px(40);
        const auto help = tab ? filename(tab->path) : L"Ctrl+O  •  Húzza ide a PDF-fájlokat";
        DrawTextW(dc, help.c_str(), -1, &label, DT_CENTER | DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS | DT_NOPREFIX);
        SelectObject(dc, previous);
    } else {
        const int x = std::max(px(20), static_cast<int>((rect.right - displayed_.width) / 2)) - scrollX_;
        const int y = std::max(px(20), static_cast<int>((rect.bottom - displayed_.height) / 2)) - scrollY_;
        RECT shadow{x + px(3), y + px(3), x + displayed_.width + px(3), y + displayed_.height + px(3)};
        HBRUSH brush = CreateSolidBrush(RGB(194, 201, 211)); FillRect(dc, &shadow, brush); DeleteObject(brush);
        BITMAPINFO bitmap{}; bitmap.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bitmap.bmiHeader.biWidth = displayed_.width; bitmap.bmiHeader.biHeight = -displayed_.height;
        bitmap.bmiHeader.biPlanes = 1; bitmap.bmiHeader.biBitCount = 32; bitmap.bmiHeader.biCompression = BI_RGB;
        SetDIBitsToDevice(dc, x, y, displayed_.width, displayed_.height, 0, 0, 0, displayed_.height,
            displayed_.pixels.data(), &bitmap, DIB_RGB_COLORS);
    }
}
LRESULT CALLBACK MainWindow::pageProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam, UINT_PTR, DWORD_PTR context) {
    auto* self = reinterpret_cast<MainWindow*>(context);
    if (message == WM_KEYDOWN && wparam == VK_RETURN) {
        wchar_t buffer[16]{}; GetWindowTextW(hwnd, buffer, 16);
        const long page = std::wcstol(buffer, nullptr, 10);
        auto* tab = self->tabs_.active();
        if (tab && !tab->opening && page > 0 && page <= tab->pageCount) {
            tab->page = static_cast<int>(page - 1); self->scrollX_ = self->scrollY_ = 0;
            self->syncScroll(); self->requestRender(); SetFocus(self->canvas_);
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
    case WM_PRINTCLIENT: self->drawCanvas(reinterpret_cast<HDC>(wparam)); GdiFlush(); return 0;
    case WM_ERASEBKGND: return 1;
    case WM_LBUTTONDOWN: SetFocus(hwnd); return 0;
    case WM_HSCROLL: self->scroll(SB_HORZ, LOWORD(wparam), HIWORD(wparam)); return 0;
    case WM_VSCROLL: self->scroll(SB_VERT, LOWORD(wparam), HIWORD(wparam)); return 0;
    case WM_MOUSEWHEEL:
        if (GET_KEYSTATE_WPARAM(wparam) & MK_CONTROL) self->command(GET_WHEEL_DELTA_WPARAM(wparam) > 0 ? IDM_ZOOMIN : IDM_ZOOMOUT);
        else {
            const int delta = GET_WHEEL_DELTA_WPARAM(wparam), before = self->scrollY_;
            self->scrollBy(0, -delta * self->px(60) / WHEEL_DELTA);
            const auto* tab = self->tabs_.active();
            if (self->scrollY_ == before && tab && !tab->rendering) self->command(delta > 0 ? IDM_PREVIOUS : IDM_NEXT);
        }
        return 0;
    }
    return DefWindowProcW(hwnd, message, wparam, lparam);
}
LRESULT MainWindow::message(UINT message, WPARAM wparam, LPARAM lparam) {
    switch (message) {
    case WM_CREATE: makeControls(); return 0;
    case WM_PAINT: paintHeader(); return 0;
    case WM_GETMINMAXINFO: {
        auto* info = reinterpret_cast<MINMAXINFO*>(lparam); info->ptMinTrackSize = {px(760), px(420)}; return 0;
    }
    case WM_SIZE:
        if (wparam != SIZE_MINIMIZED) {
            layout();
            if (renderer_ && tabs_.active()) SetTimer(window_, ResizeTimer, 160, nullptr);
        }
        return 0;
    case WM_TIMER:
        if (wparam == ResizeTimer) { KillTimer(window_, ResizeTimer); requestRender(); }
        else if (wparam == SmokeTimer) { DestroyWindow(window_); PostQuitMessage(3); }
        return 0;
    case WM_DPICHANGED: {
        syncScroll(); const int oldDpi = dpi_; dpi_ = HIWORD(wparam);
        for (const auto& item : tabs_.items()) {
            auto* tab = tabs_.find(item.id); tab->scrollX = MulDiv(tab->scrollX, dpi_, oldDpi); tab->scrollY = MulDiv(tab->scrollY, dpi_, oldDpi);
        }
        if (const auto* tab = tabs_.active()) { scrollX_ = tab->scrollX; scrollY_ = tab->scrollY; }
        rebuildToolbar(); const auto* rect = reinterpret_cast<RECT*>(lparam);
        SetWindowPos(window_, nullptr, rect->left, rect->top, rect->right - rect->left, rect->bottom - rect->top, SWP_NOZORDER | SWP_NOACTIVATE);
        layout(); updateControls(); return 0;
    }
    case WM_COMMAND:
        if (HIWORD(wparam) == 0 || HIWORD(wparam) == 1) command(LOWORD(wparam));
        return 0;
    case WM_NOTIFY: {
        const auto* header = reinterpret_cast<NMHDR*>(lparam);
        if (header->hwndFrom == tabBar_.handle() && header->code == TCN_SELCHANGE) {
            activate(tabBar_.selected()); return 0;
        }
        if (header->code == TBN_GETINFOTIPW) {
            auto* tip = reinterpret_cast<NMTBGETINFOTIPW*>(lparam); const wchar_t* text = L"";
            switch (tip->iItem) {
            case IDM_OPEN: text = L"PDF-ek megnyitása (Ctrl+O)"; break;
            case IDM_PRINT: text = L"Nyomtatás — későbbi verzióban"; break;
            case IDM_PREVIOUS: text = L"Előző oldal (PgUp)"; break;
            case IDM_NEXT: text = L"Következő oldal (PgDn)"; break;
            case IDM_ZOOMIN: text = L"Nagyítás (Ctrl++)"; break;
            case IDM_ZOOMOUT: text = L"Kicsinyítés (Ctrl+-)"; break;
            case IDM_FITPAGE: text = L"Teljes oldal"; break;
            case IDM_FITWIDTH: text = L"Oldalszélességhez illesztés"; break;
            case IDM_ROTATE: text = L"Forgatás jobbra — csak megjelenítés"; break;
            }
            lstrcpynW(tip->pszText, text, tip->cchTextMax); return 0;
        }
        break;
    }
    case WM_DROPFILES: {
        HDROP drop = reinterpret_cast<HDROP>(wparam); const UINT count = DragQueryFileW(drop, 0xFFFFFFFF, nullptr, 0);
        for (UINT i = 0; i < count; ++i) {
            const UINT length = DragQueryFileW(drop, i, nullptr, 0); std::vector<wchar_t> path(length + 1);
            if (length && DragQueryFileW(drop, i, path.data(), length + 1)) open(path.data());
        }
        DragFinish(drop); return 0;
    }
    case WM_CTLCOLORSTATIC:
        if (reinterpret_cast<HWND>(lparam) == noticeLabel_) {
            SetTextColor(reinterpret_cast<HDC>(wparam), RGB(122, 77, 15));
            SetBkColor(reinterpret_cast<HDC>(wparam), RGB(255, 245, 225));
            return reinterpret_cast<LRESULT>(noticeBrush_);
        }
        SetBkMode(reinterpret_cast<HDC>(wparam), TRANSPARENT); return reinterpret_cast<LRESULT>(background_);
    case PdfRenderer::ResultMessage: if (!destroying_) acceptResults(); return 0;
    case DocumentTabBar::CloseMessage: closeTab(static_cast<DocumentId>(wparam)); return 0;
    case PasswordMessage: promptPassword(); return 0;
    case WM_CLOSE: DestroyWindow(window_); return 0;
    case WM_DESTROY:
        destroying_ = true; KillTimer(window_, ResizeTimer); KillTimer(window_, SmokeTimer);
        renderer_.reset(); PostQuitMessage(0); return 0;
    }
    return DefWindowProcW(window_, message, wparam, lparam);
}
}
