#pragma once
#include "TabManager.h"
#include <windows.h>
#include <commctrl.h>

namespace spdf {
// Native tab-control input/keyboard/overflow behavior, with a light painted skin.
class DocumentTabBar {
public:
    static constexpr UINT CloseMessage = WM_APP + 10;
    ~DocumentTabBar();
    void create(HWND parent, HINSTANCE instance, int controlId);
    void update(const TabManager& documents);
    void style(HFONT font, int dpi);
    void resize(int width);
    HWND handle() const { return window_; }
    DocumentId selected() const;
private:
    static LRESULT CALLBACK subclass(HWND, UINT, WPARAM, LPARAM, UINT_PTR, DWORD_PTR);
    void paint();
    int closeHit(POINT point) const;
    int px(int value) const { return MulDiv(value, dpi_, 96); }
    HWND window_ = nullptr;
    HICON icon_ = nullptr;
    HFONT font_ = nullptr;
    int dpi_ = 96, hover_ = -1, pressed_ = -1;
    std::vector<std::wstring> paths_;
};
}
