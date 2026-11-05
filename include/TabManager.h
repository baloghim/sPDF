#pragma once
#include "DocumentState.h"
#include <vector>

namespace spdf {
class TabManager {
public:
    struct OpenResult { DocumentId id; bool created; };
    OpenResult open(const std::wstring& path);
    bool select(DocumentId id);
    bool close(DocumentId id);
    void cycle(int direction);
    DocumentState* active() { return find(active_); }
    const DocumentState* active() const { return find(active_); }
    DocumentState* find(DocumentId id);
    const DocumentState* find(DocumentId id) const;
    const std::vector<DocumentState>& items() const { return items_; }
    DocumentId activeId() const { return active_; }
private:
    std::vector<DocumentState> items_;
    DocumentId active_ = 0, next_ = 1;
};
}
