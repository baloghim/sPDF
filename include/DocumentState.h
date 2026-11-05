#pragma once
#include <cstdint>
#include <string>

namespace spdf {
using DocumentId = std::uint64_t;
enum class ZoomMode { FitPage, FitWidth, Percent };
// Lightweight tab state. PDFium handles stay on the worker, bitmaps in the view.
// Future workspace/organizer state can be attached here using stable IDs.
struct DocumentState {
    DocumentId id = 0;
    std::wstring path;
    int page = 0, pageCount = 0, rotation = 0;
    int scrollX = 0, scrollY = 0;
    double zoom = 1, renderedZoom = 1, pageWidth = 0, pageHeight = 0;
    ZoomMode zoomMode = ZoomMode::FitPage;
    std::uint64_t fileBytes = 0, renderGeneration = 0;
    bool opening = true, rendering = false, passwordPending = false, limited = false;
};
}
