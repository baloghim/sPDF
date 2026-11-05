#include "PdfRenderer.h"
#include <algorithm>
#include <cmath>
#include <cstring>

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
    double scale = request.zoom * 96.0 / 72.0;
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
    result.zoom = scale * 72.0 / 96.0;
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
    request.generation = ++generation_;
    pending_ = std::move(request);
    ready_.notify_one();
    return generation_;
}
std::vector<RenderResult> PdfRenderer::takeResults() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<RenderResult> result;
    result.swap(results_);
    return result;
}
void PdfRenderer::run() {
    std::unique_ptr<PdfiumLibrary> api;
    std::unique_ptr<PdfDocument> doc;
    std::wstring currentPath;
    while (true) {
        RenderRequest request;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            ready_.wait(lock, [&] { return stopping_ || pending_.has_value(); });
            if (stopping_) return;
            request = std::move(*pending_); pending_.reset();
        }
        RenderResult result;
        result.generation = request.generation;
        try {
            if (request.close) { doc.reset(); currentPath.clear(); result.closed = true; }
            else {
                if (!api) api = std::make_unique<PdfiumLibrary>();
                if (request.openPath) {
                    // Keep the previous document intact if opening/rendering fails.
                    auto candidate = std::make_unique<PdfDocument>(*api, *request.openPath, request.password);
                    result = renderPage(*candidate, request, [&] { return stopping_ || generation_ != request.generation; });
                    if (generation_ != request.generation || stopping_) continue;
                    doc = std::move(candidate);
                    currentPath = *request.openPath;
                    result.opened = true;
                } else if (doc) {
                    result = renderPage(*doc, request, [&] { return stopping_ || generation_ != request.generation; });
                } else continue;
                result.path = currentPath;
            }
        } catch (const PdfError& error) { result.error = error.message; result.errorCode = error.code; }
          catch (const std::bad_alloc&) { result.error = L"Nincs elég memória a dokumentum megjelenítéséhez."; }
          catch (const std::exception&) { result.error = L"A PDFium nem indítható. Ellenőrizze az alkalmazás melletti pdfium.dll fájlt."; }
        if (generation_ == request.generation && !stopping_) {
            std::lock_guard<std::mutex> lock(mutex_);
            results_.clear(); results_.push_back(std::move(result));
            PostMessageW(window_, ResultMessage, 0, 0);
        }
    }
}
}
