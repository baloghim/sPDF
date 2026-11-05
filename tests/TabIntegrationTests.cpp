#include "TabManager.h"
#include "PdfRenderer.h"
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <functional>

namespace {
int checks = 0;
void require(bool condition, const char* name) {
    if (!condition) throw std::runtime_error(name);
    ++checks; std::cout << "PASS: " << name << '\n';
}
spdf::RenderResult waitFor(spdf::PdfRenderer& worker, spdf::DocumentId id, spdf::RenderOperation operation,
    std::uint64_t generation = 0) {
    const auto deadline = GetTickCount64() + 15000;
    while (GetTickCount64() < deadline) {
        for (auto& result : worker.takeResults()) {
            if (result.documentId == id && result.operation == operation && (!generation || result.generation == generation))
                return std::move(result);
        }
        MSG message{}; while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE)) DispatchMessageW(&message);
        MsgWaitForMultipleObjects(0, nullptr, FALSE, 20, QS_ALLINPUT);
    }
    throw std::runtime_error("Worker result timed out");
}
}
int main(int argc, char** argv) {
    if (argc != 2) return 1;
    try {
        const auto fixtures = std::filesystem::u8path(argv[1]);
        spdf::TabManager tabs;
        const auto first = tabs.open((fixtures / "sample.pdf").wstring());
        require(first.created && tabs.items().size() == 1, "first tab created");
        auto* state = tabs.active();
        state->page = 2; state->zoom = 2.25; state->zoomMode = spdf::ZoomMode::Percent;
        state->rotation = 3; state->scrollX = 47; state->scrollY = 321;
        const auto second = tabs.open((fixtures / "encrypted.pdf").wstring());
        require(second.created && second.id != first.id, "independent stable tab IDs");
        tabs.active()->page = 1;
        tabs.select(first.id);
        require(tabs.active()->page == 2, "page number survives tab switch");
        require(tabs.active()->zoom == 2.25 && tabs.active()->zoomMode == spdf::ZoomMode::Percent, "zoom and fit mode survive switch");
        require(tabs.active()->rotation == 3, "view rotation survives switch");
        require(tabs.active()->scrollX == 47 && tabs.active()->scrollY == 321, "scroll position survives switch");
        const auto duplicate = tabs.open((fixtures / "." / "SAMPLE.PDF").wstring());
        require(!duplicate.created && duplicate.id == first.id && tabs.items().size() == 2, "case and normalized-path duplicate activates existing tab");
        tabs.cycle(1); require(tabs.activeId() == second.id, "Ctrl+Tab ordering");
        tabs.cycle(-1); require(tabs.activeId() == first.id, "reverse tab ordering");
        require(!tabs.select(9000) && tabs.activeId() == first.id, "invalid tab selection is harmless");
        tabs.close(second.id);
        require(tabs.activeId() == first.id && tabs.active()->page == 2, "closing background tab preserves active state");
        tabs.close(first.id); require(tabs.items().empty() && !tabs.active(), "last tab can close");
        const auto third = tabs.open((fixtures / "sample.pdf").wstring());
        require(third.id > second.id, "closed tab IDs are never reused");

        HWND notify = CreateWindowExW(0, L"STATIC", L"sPDF worker tests", 0, 0, 0, 0, 0, HWND_MESSAGE, nullptr, GetModuleHandleW(nullptr), nullptr);
        require(notify != nullptr, "notification window created");
        {
            spdf::PdfRenderer worker(notify);
            auto open = [&](spdf::DocumentId id, const wchar_t* name, const std::string& password = {}) {
                spdf::RenderRequest request; request.documentId = id; request.openPath = (fixtures / name).wstring(); request.password = password;
                worker.submit(std::move(request));
            };
            worker.activate(101); open(101, L"sample.pdf");
            auto firstOpen = waitFor(worker, 101, spdf::RenderOperation::Open);
            require(firstOpen.opened && firstOpen.pageCount == 3 && firstOpen.pixels.empty(), "opening reads metadata without rendering");
            open(102, L"encrypted.pdf", "spdf-test");
            auto secondOpen = waitFor(worker, 102, spdf::RenderOperation::Open);
            require(secondOpen.opened && secondOpen.pixels.empty(), "background document opens without bitmap allocation");
            spdf::RenderRequest request; request.documentId = 101; request.mode = spdf::ZoomMode::Percent; request.zoom = 0.75;
            auto generation = worker.submit(request);
            auto rendered = waitFor(worker, 101, spdf::RenderOperation::Render, generation);
            require(rendered.error.empty() && rendered.width == 200 && !rendered.pixels.empty(), "first document renders from retained handle");
            request.documentId = 102;
            require(worker.submit(request) == 0, "inactive tabs cannot enqueue rendering");
            worker.activate(102); request.page = 1;
            generation = worker.submit(request);
            auto blue = waitFor(worker, 102, spdf::RenderOperation::Render, generation);
            const size_t center = (static_cast<size_t>(150) * 200 + 100) * 4;
            require(blue.pixels.size() > center && blue.pixels[center] > 200 && blue.pixels[center + 2] < 50, "switch renders correct document and page");
            open(103, L"broken.pdf");
            auto broken = waitFor(worker, 103, spdf::RenderOperation::Open);
            require(!broken.error.empty(), "bad document rejected independently");
            worker.activate(101); request.documentId = 101; request.page = 0;
            generation = worker.submit(request);
            auto red = waitFor(worker, 101, spdf::RenderOperation::Render, generation);
            require(red.pixels.size() > center && red.pixels[center + 2] > 200, "failed opening does not replace other document handles");
            open(104, L"sample.pdf");
            auto sameFile = waitFor(worker, 104, spdf::RenderOperation::Open);
            require(sameFile.duplicateOf == 101 && sameFile.pixels.empty(), "file identity deduplication returns existing document");
            open(105, L"encrypted.pdf", "wrong");
            auto wrong = waitFor(worker, 105, spdf::RenderOperation::Open);
            require(wrong.errorCode == FPDF_ERR_PASSWORD, "wrong password does not disturb other tabs");
            spdf::RenderRequest close; close.documentId = 102; close.close = true; worker.submit(close);
            require(waitFor(worker, 102, spdf::RenderOperation::Close).closed, "background close acknowledged");
            request.page = 2; generation = worker.submit(request);
            require(!waitFor(worker, 101, spdf::RenderOperation::Render, generation).pixels.empty(), "remaining tab still renders after background close");
            open(106, L"encrypted.pdf", "spdf-test");
            close.documentId = 106; worker.submit(close);
            // Control operations must remain ordered even when submitted back-to-back.
            bool sawOpen = false, sawClose = false;
            const auto deadline = GetTickCount64() + 15000;
            while (!sawClose && GetTickCount64() < deadline) {
                for (const auto& result : worker.takeResults()) if (result.documentId == 106) {
                    if (result.operation == spdf::RenderOperation::Open) sawOpen = result.opened;
                    if (result.operation == spdf::RenderOperation::Close) { require(sawOpen, "queued opening is not lost before close"); sawClose = true; }
                }
                MSG message{}; while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE)) DispatchMessageW(&message);
                MsgWaitForMultipleObjects(0, nullptr, FALSE, 20, QS_ALLINPUT);
            }
            require(sawClose, "rapid open/close completes without coalescing controls");
        }
        DestroyWindow(notify);
        std::cout << checks << " checks passed; 0 failed\n"; return 0;
    } catch (const std::exception& error) {
        std::cerr << "FAIL after " << checks << " checks: " << error.what() << '\n'; return 1;
    }
}
