#pragma once
#include "PdfRenderer.h"
#include <commctrl.h>

namespace spdf {
class MainWindow {
public:
    explicit MainWindow(HINSTANCE instance) : instance_(instance) {}
    ~MainWindow();
    bool create(int show, const std::wstring& initialPath, bool smokeTest = false);
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
    void command(int);
    void requestRender();
    void acceptResults();
    void updateControls();
    void updateScrollbars();
    void paintCanvas();
    void scroll(int bar, int code, int thumb);
    void scrollBy(int x, int y);
    int px(int value) const { return MulDiv(value, dpi_, 96); }
    HINSTANCE instance_;
    HWND window_ = nullptr, toolbar_ = nullptr, status_ = nullptr, canvas_ = nullptr, pageEdit_ = nullptr;
    HFONT font_ = nullptr;
    HIMAGELIST images_ = nullptr;
    HBRUSH background_ = nullptr;
    std::unique_ptr<PdfRenderer> renderer_;
    RenderRequest request_;
    RenderResult displayed_;
    std::uint64_t generation_ = 0;
    int dpi_ = 96, scrollX_ = 0, scrollY_ = 0;
    bool busy_ = false, opening_ = false, smokeTest_ = false;
};
}
