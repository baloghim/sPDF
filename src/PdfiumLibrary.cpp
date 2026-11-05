#include "PdfiumLibrary.h"
#include <cstring>
#include <stdexcept>
#include <vector>

namespace spdf {
std::wstring PdfiumLibrary::executableDirectory() {
    std::vector<wchar_t> buffer(32768);
    const DWORD length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    if (!length || length >= buffer.size()) throw std::runtime_error("Executable path unavailable");
    const std::wstring path(buffer.data(), length);
    return path.substr(0, path.find_last_of(L"\\/"));
}
PdfiumLibrary::PdfiumLibrary() {
    // Never load a DLL from the document directory or the current directory.
    const auto path = executableDirectory() + L"\\pdfium.dll";
    module_ = LoadLibraryExW(path.c_str(), nullptr,
        LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (!module_) throw std::runtime_error("pdfium.dll could not be loaded");
    try {
#define SPDF_BIND(name) { auto address = GetProcAddress(module_, #name); \
        if (!address) throw std::runtime_error("Missing PDFium function: " #name); \
        static_assert(sizeof(address) == sizeof(name)); std::memcpy(&name, &address, sizeof(name)); }
        SPDF_BIND(FPDF_InitLibraryWithConfig); SPDF_BIND(FPDF_DestroyLibrary);
        SPDF_BIND(FPDF_LoadCustomDocument); SPDF_BIND(FPDF_GetLastError);
        SPDF_BIND(FPDF_CloseDocument); SPDF_BIND(FPDF_GetPageCount);
        SPDF_BIND(FPDF_LoadPage); SPDF_BIND(FPDF_ClosePage);
        SPDF_BIND(FPDF_GetPageWidth); SPDF_BIND(FPDF_GetPageHeight);
        SPDF_BIND(FPDFBitmap_Create); SPDF_BIND(FPDFBitmap_Destroy);
        SPDF_BIND(FPDFBitmap_FillRect); SPDF_BIND(FPDFBitmap_GetBuffer);
        SPDF_BIND(FPDFBitmap_GetStride); SPDF_BIND(FPDF_RenderPageBitmap_Start);
        SPDF_BIND(FPDF_RenderPage_Continue); SPDF_BIND(FPDF_RenderPage_Close);
        SPDF_BIND(FPDFText_LoadPage); SPDF_BIND(FPDFText_ClosePage); SPDF_BIND(FPDFText_CountChars);
#undef SPDF_BIND
        FPDF_LIBRARY_CONFIG config{};
        config.version = 2;
        FPDF_InitLibraryWithConfig(&config);
    } catch (...) { FreeLibrary(module_); module_ = nullptr; throw; }
}
PdfiumLibrary::~PdfiumLibrary() {
    if (module_) { FPDF_DestroyLibrary(); FreeLibrary(module_); }
}
}
