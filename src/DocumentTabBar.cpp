#include "DocumentTabBar.h"
#include "resource.h"
#include <windowsx.h>
#include <algorithm>

namespace spdf {
DocumentTabBar::~DocumentTabBar() {
    if (IsWindow(window_)) RemoveWindowSubclass(window_, subclass, 1);
    if (icon_) DestroyIcon(icon_);
}
void DocumentTabBar::create(HWND parent, HINSTANCE instance, int id) {
    window_ = CreateWindowExW(0, WC_TABCONTROLW, nullptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_CLIPCHILDREN |
        TCS_BUTTONS | TCS_FLATBUTTONS | TCS_FIXEDWIDTH | TCS_TOOLTIPS,
        0, 0, 0, 0, parent, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), instance, nullptr);
    SendMessageW(window_, TCM_SETUNICODEFORMAT, TRUE, 0);
    SetWindowSubclass(window_, subclass, 1, reinterpret_cast<DWORD_PTR>(this));
    icon_ = static_cast<HICON>(LoadImageW(instance, MAKEINTRESOURCEW(IDI_APP), IMAGE_ICON, 32, 32, 0));
}
void DocumentTabBar::style(HFONT font, int dpi) {
    font_ = font; dpi_ = dpi;
    SendMessageW(window_, WM_SETFONT, reinterpret_cast<WPARAM>(font), FALSE);
    RECT client{}; GetClientRect(window_, &client); resize(client.right);
}
void DocumentTabBar::resize(int width) {
    const int count = std::max(1, TabCtrl_GetItemCount(window_));
    const int item = std::clamp((width - px(16)) / count - px(4), px(150), px(224));
    TabCtrl_SetItemSize(window_, item, px(34));
    InvalidateRect(window_, nullptr, FALSE);
}
void DocumentTabBar::update(const TabManager& documents) {
    SendMessageW(window_, WM_SETREDRAW, FALSE, 0);
    TabCtrl_DeleteAllItems(window_); paths_.clear();
    int selected = -1, index = 0;
    for (const auto& doc : documents.items()) {
        paths_.push_back(doc.path);
        auto name = doc.path.substr(doc.path.find_last_of(L"\\/") + 1);
        if (doc.opening) name += L" …";
        TCITEMW item{}; item.mask = TCIF_TEXT | TCIF_PARAM;
        item.pszText = name.data(); item.lParam = static_cast<LPARAM>(doc.id);
        SendMessageW(window_, TCM_INSERTITEMW, index, reinterpret_cast<LPARAM>(&item));
        if (doc.id == documents.activeId()) selected = index;
        ++index;
    }
    TabCtrl_SetCurSel(window_, selected);
    hover_ = pressed_ = -1;
    SendMessageW(window_, WM_SETREDRAW, TRUE, 0);
    RECT client{}; GetClientRect(window_, &client); resize(client.right);
}
DocumentId DocumentTabBar::selected() const {
    TCITEMW item{}; item.mask = TCIF_PARAM;
    if (!SendMessageW(window_, TCM_GETITEMW, TabCtrl_GetCurSel(window_), reinterpret_cast<LPARAM>(&item))) return 0;
    return static_cast<DocumentId>(item.lParam);
}
int DocumentTabBar::closeHit(POINT point) const {
    for (int i = 0; i < TabCtrl_GetItemCount(window_); ++i) {
        RECT rect{}; TabCtrl_GetItemRect(window_, i, &rect);
        rect.left = rect.right - px(30);
        if (PtInRect(&rect, point)) return i;
    }
    return -1;
}
void DocumentTabBar::paint() {
    HDC dc = GetDC(window_);
    RECT client{}; GetClientRect(window_, &client);
    HBRUSH base = CreateSolidBrush(RGB(245, 247, 250)); FillRect(dc, &client, base); DeleteObject(base);
    auto previousFont = SelectObject(dc, font_); SetBkMode(dc, TRANSPARENT);
    const int count = TabCtrl_GetItemCount(window_);
    if (!count) {
        RECT label = client; label.left += px(18);
        SetTextColor(dc, RGB(112, 122, 137));
        DrawTextW(dc, L"Nincs megnyitott dokumentum", -1, &label, DT_SINGLELINE | DT_VCENTER);
    }
    for (int i = 0; i < count; ++i) {
        RECT rect{}; TabCtrl_GetItemRect(window_, i, &rect);
        if (rect.right < 0 || rect.left >= client.right) continue;
        const bool active = i == TabCtrl_GetCurSel(window_);
        HBRUSH fill = CreateSolidBrush(active ? RGB(255, 255, 255) : RGB(239, 242, 247));
        FillRect(dc, &rect, fill); DeleteObject(fill);
        RECT line{rect.left, rect.top, rect.right, rect.top + px(3)};
        if (active) { HBRUSH accent = CreateSolidBrush(RGB(31, 111, 214)); FillRect(dc, &line, accent); DeleteObject(accent); }
        DrawIconEx(dc, rect.left + px(12), rect.top + (rect.bottom - rect.top - px(16)) / 2,
            icon_, px(16), px(16), 0, nullptr, DI_NORMAL);
        wchar_t text[512]{};
        TCITEMW item{}; item.mask = TCIF_TEXT; item.pszText = text; item.cchTextMax = 512;
        SendMessageW(window_, TCM_GETITEMW, i, reinterpret_cast<LPARAM>(&item));
        RECT label = rect; label.left += px(36); label.right -= px(32);
        SetTextColor(dc, active ? RGB(26, 55, 91) : RGB(95, 106, 121));
        DrawTextW(dc, text, -1, &label, DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS | DT_NOPREFIX);
        const int x = rect.right - px(16), y = (rect.top + rect.bottom) / 2;
        if (hover_ == i) {
            RECT hot{x - px(10), y - px(10), x + px(10), y + px(10)};
            HBRUSH hotBrush = CreateSolidBrush(RGB(229, 235, 244)); FillRect(dc, &hot, hotBrush); DeleteObject(hotBrush);
        }
        HPEN pen = CreatePen(PS_SOLID, std::max(1, px(1)), RGB(90, 105, 126));
        auto oldPen = SelectObject(dc, pen);
        MoveToEx(dc, x - px(4), y - px(4), nullptr); LineTo(dc, x + px(4), y + px(4));
        MoveToEx(dc, x + px(4), y - px(4), nullptr); LineTo(dc, x - px(4), y + px(4));
        SelectObject(dc, oldPen); DeleteObject(pen);
    }
    SelectObject(dc, previousFont); ReleaseDC(window_, dc);
}
LRESULT CALLBACK DocumentTabBar::subclass(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam, UINT_PTR, DWORD_PTR context) {
    auto* self = reinterpret_cast<DocumentTabBar*>(context);
    switch (message) {
    case WM_PAINT: {
        // The native control computes layout/overflow lazily while painting.
        const auto result = DefSubclassProc(hwnd, message, wparam, lparam);
        self->paint(); return result;
    }
    case WM_ERASEBKGND: return 1;
    case WM_MOUSEMOVE: {
        const int hover = self->closeHit({GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)});
        if (hover != self->hover_) { self->hover_ = hover; InvalidateRect(hwnd, nullptr, FALSE); }
        TRACKMOUSEEVENT track{sizeof(track), TME_LEAVE, hwnd, 0}; TrackMouseEvent(&track); break;
    }
    case WM_MOUSELEAVE: self->hover_ = -1; InvalidateRect(hwnd, nullptr, FALSE); break;
    case WM_LBUTTONDOWN:
        self->pressed_ = self->closeHit({GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)});
        if (self->pressed_ >= 0) { SetCapture(hwnd); return 0; }
        break;
    case WM_LBUTTONUP:
        if (self->pressed_ >= 0) {
            const int index = self->pressed_; self->pressed_ = -1; ReleaseCapture();
            if (self->closeHit({GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)}) == index) {
                TCITEMW item{}; item.mask = TCIF_PARAM;
                SendMessageW(hwnd, TCM_GETITEMW, index, reinterpret_cast<LPARAM>(&item));
                PostMessageW(GetParent(hwnd), CloseMessage, static_cast<WPARAM>(item.lParam), 0);
            }
            return 0;
        }
        break;
    case WM_NOTIFY: {
        const auto* header = reinterpret_cast<NMHDR*>(lparam);
        if (header->code == TTN_GETDISPINFOW) {
            auto* tip = reinterpret_cast<NMTTDISPINFOW*>(lparam);
            if (tip->hdr.idFrom < self->paths_.size()) tip->lpszText = self->paths_[tip->hdr.idFrom].data();
            return 0;
        }
        break;
    }
    case WM_NCDESTROY: RemoveWindowSubclass(hwnd, subclass, 1); break;
    }
    return DefSubclassProc(hwnd, message, wparam, lparam);
}
}
