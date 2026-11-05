#pragma once
#include "PdfRenderer.h"
#include "TabManager.h"
#include "DocumentTabBar.h"
#include <deque>
#include <commctrl.h>

namespace spdf {
class MainWindow {
public:
    explicit MainWindow(HINSTANCE instance) : instance_(instance) {}
    ~MainWindow();
    bool create(int show, const std::vector<std::wstring>& initialPaths, bool smokeTest = false);
    HWND handle() const { return window_; }
private:
    static LRESULT CALLBACK windowProc(HWND, UINT, WPARAM, LPARAM);
    static LRESULT CALLBACK canvasProc(HWND, UINT, WPARAM, LPARAM);
    static LRESULT CALLBACK pageProc(HWND, UINT, WPARAM, LPARAM, UINT_PTR, DWORD_PTR);
    LRESULT message(UINT, WPARAM, LPARAM);
    void makeControls();
    void rebuildToolbar();
    void layout();
    void openDialog();
    void open(const std::wstring&);
    void activate(DocumentId);
    void showActive();
    void closeTab(DocumentId);
    void syncScroll();
    void promptPassword();
    void command(int);
    void requestRender();
    void acceptResults();
    void updateControls();
    void updateScrollbars();
    void paintCanvas();
    void drawCanvas(HDC);
    void paintHeader();
    void scroll(int bar, int code, int thumb);
    void scrollBy(int x, int y);
    int px(int value) const { return MulDiv(value, dpi_, 96); }
    HINSTANCE instance_;
    HWND window_ = nullptr, toolbar_ = nullptr, status_ = nullptr, canvas_ = nullptr, pageEdit_ = nullptr;
    HWND pageTotal_ = nullptr, search_ = nullptr, viewButton_ = nullptr;
    HFONT font_ = nullptr, headingFont_ = nullptr;
    HICON welcomeIcon_ = nullptr, searchIcon_ = nullptr;
    HIMAGELIST images_ = nullptr;
    HBRUSH background_ = nullptr;
    std::unique_ptr<PdfRenderer> renderer_;
    TabManager tabs_;
    DocumentTabBar tabBar_;
    std::deque<DocumentId> passwords_;
    RenderResult displayed_;
    int dpi_ = 96, scrollX_ = 0, scrollY_ = 0;
    bool smokeTest_ = false, promptingPassword_ = false, destroying_ = false;
};
}
