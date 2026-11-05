#pragma once
#include "PdfDocument.h"
#include "DocumentState.h"
#include <atomic>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <thread>
#include <vector>
#include <deque>

namespace spdf {
enum class RenderOperation { Render, Open, Close };
struct RenderRequest {
    DocumentId documentId = 0;
    std::uint64_t generation = 0;
    std::optional<std::wstring> openPath;
    std::string password;
    bool close = false;
    int page = 0, viewportWidth = 800, viewportHeight = 600, rotation = 0;
    double zoom = 1.0;
    double deviceScale = 1.0;
    ZoomMode mode = ZoomMode::FitPage;
};
struct RenderResult {
    DocumentId documentId = 0, duplicateOf = 0;
    RenderOperation operation = RenderOperation::Render;
    std::uint64_t generation = 0, fileBytes = 0;
    bool opened = false, closed = false, limited = false;
    unsigned long errorCode = 0;
    std::wstring path, error;
    int page = 0, pageCount = 0, width = 0, height = 0;
    double pageWidth = 0, pageHeight = 0, zoom = 1;
    std::vector<unsigned char> pixels; // top-down, opaque BGRA, tightly packed
};
RenderResult renderPage(PdfDocument& doc, const RenderRequest& request,
    const std::function<bool()>& cancelled);

class PdfRenderer {
public:
    explicit PdfRenderer(HWND notifyWindow);
    ~PdfRenderer();
    std::uint64_t submit(RenderRequest request);
    void activate(DocumentId id);
    std::vector<RenderResult> takeResults();
    static constexpr UINT ResultMessage = WM_APP + 1;
private:
    void run();
    HWND window_;
    std::atomic<std::uint64_t> generation_{0};
    std::atomic<DocumentId> active_{0};
    std::atomic<bool> stopping_{false};
    std::mutex mutex_;
    std::condition_variable ready_;
    std::deque<RenderRequest> pending_;
    std::vector<RenderResult> results_;
    std::thread worker_;
};
}
