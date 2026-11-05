#include "TabManager.h"
#include <windows.h>
#include <algorithm>
#include <stdexcept>

namespace spdf {
TabManager::OpenResult TabManager::open(const std::wstring& path) {
    if (path.empty()) throw std::invalid_argument("Empty document path");
    std::wstring normalized(32768, L'\0');
    const DWORD count = GetFullPathNameW(path.c_str(), static_cast<DWORD>(normalized.size()), normalized.data(), nullptr);
    if (!count || count >= normalized.size()) throw std::invalid_argument("Document path too long");
    normalized.resize(count);
    std::replace(normalized.begin(), normalized.end(), L'/', L'\\');
    for (auto& item : items_) {
        if (CompareStringOrdinal(item.path.c_str(), -1, normalized.c_str(), -1, TRUE) == CSTR_EQUAL) {
            active_ = item.id; return {item.id, false};
        }
    }
    DocumentState state;
    state.id = next_++; state.path = std::move(normalized);
    active_ = state.id; items_.push_back(std::move(state));
    return {active_, true};
}
bool TabManager::select(DocumentId id) {
    if (!find(id)) return false;
    active_ = id; return true;
}
bool TabManager::close(DocumentId id) {
    const auto found = std::find_if(items_.begin(), items_.end(), [=](const auto& item) { return item.id == id; });
    if (found == items_.end()) return false;
    const auto index = static_cast<size_t>(found - items_.begin());
    items_.erase(found);
    if (active_ == id) active_ = items_.empty() ? 0 : items_[std::min(index, items_.size() - 1)].id;
    return true;
}
void TabManager::cycle(int direction) {
    if (items_.empty()) return;
    auto it = std::find_if(items_.begin(), items_.end(), [&](const auto& item) { return item.id == active_; });
    const int size = static_cast<int>(items_.size());
    const int index = static_cast<int>(it - items_.begin());
    active_ = items_[(index + direction % size + size) % size].id;
}
DocumentState* TabManager::find(DocumentId id) {
    for (auto& item : items_) if (item.id == id) return &item;
    return nullptr;
}
const DocumentState* TabManager::find(DocumentId id) const {
    for (const auto& item : items_) if (item.id == id) return &item;
    return nullptr;
}
}
