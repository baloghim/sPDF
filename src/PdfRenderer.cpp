#include "PdfRenderer.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <map>

namespace spdf {
namespace {
struct PageGuard {
    PdfiumLibrary& api; FPDF_PAGE page;
    ~PageGuard() { api.FPDF_ClosePage(page); }
};
struct BitmapGuard {
    PdfiumLibrary& api; FPDF_BITMAP bitmap;
    ~BitmapGuard() { api.FPDFBitmap_Destroy(bitmap); }
};
struct Pause : IFSDK_PAUSE {
    const std::function<bool()>* cancelled;
    explicit Pause(const std::function<bool()>& cancel) : IFSDK_PAUSE{}, cancelled(&cancel) {
        version = 1;
        NeedToPauseNow = [](IFSDK_PAUSE* p) -> FPDF_BOOL {
            return (*static_cast<Pause*>(p)->cancelled)();
        };
    }
};
}
RenderResult renderPage(PdfDocument& doc, const RenderRequest& request, const std::function<bool()>& cancelled) {
    RenderResult result;
    result.documentId = request.documentId;
    result.generation = request.generation;
    result.page = request.page;
    result.pageCount = doc.pageCount();
    result.fileBytes = doc.byteSize();
    auto& api = doc.api();
    PageGuard page{api, doc.loadPage(request.page)};
    result.pageWidth = api.FPDF_GetPageWidth(page.page);
    result.pageHeight = api.FPDF_GetPageHeight(page.page);
    double width = result.pageWidth, height = result.pageHeight;
    if (request.rotation % 2) std::swap(width, height);
    if (!std::isfinite(width) || !std::isfinite(height) || width <= 0 || height <= 0)
        throw PdfError(L"Érvénytelen oldalméret.");
    if (!std::isfinite(request.deviceScale) || request.deviceScale <= 0) throw PdfError(L"Érvénytelen DPI-skálázás.");
    double scale = request.zoom * 96.0 / 72.0 * request.deviceScale;
    if (request.mode == ZoomMode::FitPage)
        scale = std::min(std::max(32, request.viewportWidth - 40) / width,
                         std::max(32, request.viewportHeight - 40) / height);
    else if (request.mode == ZoomMode::FitWidth)
        scale = std::max(32, request.viewportWidth - 40) / width;
    if (!std::isfinite(scale) || scale <= 0) throw PdfError(L"Érvénytelen nagyítás.");
    // At most 48 MiB for a bitmap and 48 MiB for its result. No disk cache.
    constexpr double maxPixels = 12.0 * 1024 * 1024;
    const double limit = std::min({scale, 16384.0 / width, 16384.0 / height, std::sqrt(maxPixels / width / height)});
    result.limited = limit < scale;
    scale = limit;
    // Avoid losing a pixel to floating-point error (e.g. 200 * 2.3).
    result.width = std::max(1, static_cast<int>(std::floor(width * scale + 1e-7)));
    result.height = std::max(1, static_cast<int>(std::floor(height * scale + 1e-7)));
    result.zoom = scale * 72.0 / 96.0 / request.deviceScale;
    if (cancelled()) return result;
    BitmapGuard bitmap{api, api.FPDFBitmap_Create(result.width, result.height, 1)};
    if (!bitmap.bitmap) throw PdfError(L"Nincs elég memória az oldal megjelenítéséhez.");
    api.FPDFBitmap_FillRect(bitmap.bitmap, 0, 0, result.width, result.height, 0xFFFFFFFF);
    Pause pause(cancelled);
    int state = api.FPDF_RenderPageBitmap_Start(bitmap.bitmap, page.page, 0, 0, result.width,
        result.height, request.rotation, FPDF_ANNOT | FPDF_LCD_TEXT, &pause);
    while (state == FPDF_RENDER_TOBECONTINUED && !cancelled())
        state = api.FPDF_RenderPage_Continue(page.page, &pause);
    api.FPDF_RenderPage_Close(page.page);
    if (cancelled()) return result;
    if (state != FPDF_RENDER_DONE) throw PdfError(L"Az oldal megjelenítése sikertelen.");
    const auto* data = static_cast<const unsigned char*>(api.FPDFBitmap_GetBuffer(bitmap.bitmap));
    const int stride = api.FPDFBitmap_GetStride(bitmap.bitmap);
    result.pixels.resize(static_cast<size_t>(result.width) * result.height * 4);
    for (int y = 0; y < result.height; ++y)
        std::memcpy(result.pixels.data() + static_cast<size_t>(y) * result.width * 4,
            data + static_cast<size_t>(y) * stride, static_cast<size_t>(result.width) * 4);
    return result;
}
PdfRenderer::PdfRenderer(HWND window) : window_(window), worker_(&PdfRenderer::run, this) {}
PdfRenderer::~PdfRenderer() {
    stopping_ = true; ++generation_; ready_.notify_one();
    if (worker_.joinable()) worker_.join();
}
std::uint64_t PdfRenderer::submit(RenderRequest request) {
    std::lock_guard<std::mutex> lock(mutex_);
    const bool render = !request.openPath && !request.close;
    if (render) {
        if (request.documentId != active_) return 0;
        request.generation = ++generation_;
        pending_.erase(std::remove_if(pending_.begin(), pending_.end(), [](const auto& item) {
            return !item.openPath && !item.close;
        }), pending_.end());
    }
    const auto generation = request.generation;
    pending_.push_back(std::move(request));
    ready_.notify_one();
    return generation;
}
void PdfRenderer::activate(DocumentId id) {
    std::lock_guard<std::mutex> lock(mutex_);
    active_ = id; ++generation_;
    pending_.erase(std::remove_if(pending_.begin(), pending_.end(), [](const auto& item) {
        return !item.openPath && !item.close;
    }), pending_.end());
    results_.erase(std::remove_if(results_.begin(), results_.end(), [](const auto& item) {
        return item.operation == RenderOperation::Render;
    }), results_.end());
}
std::vector<RenderResult> PdfRenderer::takeResults() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<RenderResult> result;
    result.swap(results_);
    return result;
}
void PdfRenderer::run() {
    std::unique_ptr<PdfiumLibrary> api;
    struct Entry { std::unique_ptr<PdfDocument> doc; std::wstring path, identity; };
    // Declared after api so all documents are destroyed before the library.
    std::map<DocumentId, Entry> documents;
    while (true) {
        RenderRequest request;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            ready_.wait(lock, [&] { return stopping_ || !pending_.empty(); });
            if (stopping_) return;
            request = std::move(pending_.front()); pending_.pop_front();
        }
        RenderResult result;
        result.documentId = request.documentId;
        result.generation = request.generation;
        result.operation = request.close ? RenderOperation::Close :
            request.openPath ? RenderOperation::Open : RenderOperation::Render;
        try {
            if (request.close) { documents.erase(request.documentId); result.closed = true; }
            else {
                if (!api) api = std::make_unique<PdfiumLibrary>();
                if (request.openPath) {
                    auto candidate = std::make_unique<PdfDocument>(*api, *request.openPath, request.password);
                    const auto identity = candidate->identity();
                    for (const auto& entry : documents) {
                        if (!identity.empty() && entry.second.identity == identity) {
                            result.duplicateOf = entry.first; break;
                        }
                    }
                    result.pageCount = candidate->pageCount();
                    result.fileBytes = candidate->byteSize();
                    result.path = *request.openPath;
                    if (!result.duplicateOf)
                        documents.emplace(request.documentId, Entry{std::move(candidate), *request.openPath, identity});
                    result.opened = true;
                } else {
                    const auto found = documents.find(request.documentId);
                    if (found == documents.end() || active_ != request.documentId || generation_ != request.generation) continue;
                    result = renderPage(*found->second.doc, request, [&] {
                        return stopping_ || active_ != request.documentId || generation_ != request.generation;
                    });
                    result.path = found->second.path;
                }
            }
        } catch (const PdfError& error) { result.error = error.message; result.errorCode = error.code; }
          catch (const std::bad_alloc&) { result.error = L"Nincs elég memória a dokumentum megjelenítéséhez."; }
          catch (const std::exception&) { result.error = L"A PDFium nem indítható. Ellenőrizze az alkalmazás melletti pdfium.dll fájlt."; }
        SecureZeroMemory(request.password.data(), request.password.size());
        if (!stopping_) {
            std::lock_guard<std::mutex> lock(mutex_);
            if (result.operation == RenderOperation::Render &&
                (active_ != request.documentId || generation_ != request.generation)) continue;
            // Control results must never be coalesced: multi-file opening and closing
            // are ordered independently of disposable viewport render requests.
            results_.push_back(std::move(result));
            PostMessageW(window_, ResultMessage, 0, 0);
        }
    }
}
}
