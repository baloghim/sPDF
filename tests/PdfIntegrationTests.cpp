#include "PdfRenderer.h"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

namespace {
int checks = 0;
void require(bool condition, const char* name) {
    if (!condition) throw std::runtime_error(name);
    ++checks; std::cout << "PASS: " << name << '\n';
}
std::string bytes(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    std::ostringstream contents; contents << file.rdbuf(); return contents.str();
}
template<class Operation> void rejects(Operation op, const char* name) {
    bool rejected = false;
    try { op(); } catch (const spdf::PdfError&) { rejected = true; }
    require(rejected, name);
}
}
int main(int argc, char** argv) {
    if (argc != 2) { std::cerr << "Expected fixture directory\n"; return 1; }
    try {
        const auto fixtures = std::filesystem::u8path(argv[1]);
        const auto sample = fixtures / "sample.pdf";
        const auto original = bytes(sample);
        spdf::PdfiumLibrary api;
        {
            spdf::PdfDocument doc(api, sample.wstring());
            require(doc.pageCount() == 3, "open real PDF; three pages");
            require(doc.byteSize() == original.size(), "read-only custom file access");
            auto page = doc.loadPage(0);
            auto text = api.FPDFText_LoadPage(page);
            require(text && api.FPDFText_CountChars(text) > 10, "text layer is retained and readable");
            api.FPDFText_ClosePage(text); api.FPDF_ClosePage(page);
            spdf::RenderRequest request;
            request.mode = spdf::ZoomMode::Percent; request.zoom = 0.75;
            auto first = spdf::renderPage(doc, request, [] { return false; });
            require(first.width == 200 && first.height == 300, "100% point scale renders expected size");
            const size_t center = (static_cast<size_t>(150) * first.width + 100) * 4;
            require(first.pixels[center + 2] > 200 && first.pixels[center] < 50, "first page really rendered red content");
            request.deviceScale = 1.5;
            auto scaled = spdf::renderPage(doc, request, [] { return false; });
            require(scaled.width == 300 && scaled.height == 450 && scaled.zoom == request.zoom, "150 percent DPI scales pixels while preserving logical zoom");
            request.deviceScale = 1;
            request.page = 1;
            auto second = spdf::renderPage(doc, request, [] { return false; });
            require(second.pixels[center] > 200 && second.pixels[center + 2] < 50, "second page really rendered blue content");
            request.rotation = 1;
            auto rotated = spdf::renderPage(doc, request, [] { return false; });
            require(rotated.width == 300 && rotated.height == 200, "view rotation swaps bitmap dimensions");
            request.rotation = 0; request.mode = spdf::ZoomMode::FitPage;
            request.viewportWidth = 500; request.viewportHeight = 500;
            auto fitted = spdf::renderPage(doc, request, [] { return false; });
            require(fitted.width <= 460 && fitted.height <= 460, "fit page stays within viewport");
            request.mode = spdf::ZoomMode::FitWidth;
            auto width = spdf::renderPage(doc, request, [] { return false; });
            require(width.width == 460, "fit width uses viewport width");
            auto cancelled = spdf::renderPage(doc, request, [] { return true; });
            require(cancelled.pixels.empty(), "obsolete render is cancelled");
            request.mode = spdf::ZoomMode::Percent; request.zoom = 100;
            auto bounded = spdf::renderPage(doc, request, [] { return false; });
            require(bounded.limited && bounded.pixels.size() <= 48 * 1024 * 1024, "bitmap allocation is bounded");
            rejects([&] { doc.loadPage(-1); }, "negative page rejected");
            rejects([&] { doc.loadPage(3); }, "out-of-range page rejected");
        }
        require(bytes(sample) == original, "original PDF unchanged byte for byte");
        rejects([&] { spdf::PdfDocument d(api, (fixtures / "broken.pdf").wstring()); }, "corrupt PDF rejected");
        rejects([&] { spdf::PdfDocument d(api, (fixtures / "missing.pdf").wstring()); }, "missing file rejected");
        bool asksPassword = false;
        try { spdf::PdfDocument d(api, (fixtures / "encrypted.pdf").wstring()); }
        catch (const spdf::PdfError& error) { asksPassword = error.code == FPDF_ERR_PASSWORD; }
        require(asksPassword, "encrypted PDF reports password requirement");
        rejects([&] { spdf::PdfDocument d(api, (fixtures / "encrypted.pdf").wstring(), "wrong"); }, "wrong password rejected");
        spdf::PdfDocument encrypted(api, (fixtures / "encrypted.pdf").wstring(), "spdf-test");
        require(encrypted.pageCount() == 3, "correct password opens document");
        const auto unicode = std::filesystem::temp_directory_path() / L"sPDF-\u00e1rv\u00edzt\u0171r\u0151-\u6d4b\u8bd5.pdf";
        std::filesystem::copy_file(sample, unicode, std::filesystem::copy_options::overwrite_existing);
        {
            spdf::PdfDocument doc(api, unicode.wstring());
            require(doc.pageCount() == 3, "Unicode and Hungarian filename opens");
            spdf::PdfDocument other(api, sample.wstring());
            require(other.pageCount() == doc.pageCount(), "independent document handles coexist");
        }
        std::filesystem::remove(unicode);
        std::cout << checks << " checks passed; 0 failed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FAIL after " << checks << " checks: " << error.what() << '\n'; return 1;
    }
}
