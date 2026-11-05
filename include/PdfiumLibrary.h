#pragma once
#include <windows.h>
#include <fpdfview.h>
#include <fpdf_progressive.h>
#include <fpdf_text.h>
#include <string>

namespace spdf {
// All PDFium calls, including initialization/destruction, belong to one thread.
class PdfiumLibrary {
public:
    PdfiumLibrary();
    ~PdfiumLibrary();
    PdfiumLibrary(const PdfiumLibrary&) = delete;
    PdfiumLibrary& operator=(const PdfiumLibrary&) = delete;
    static std::wstring executableDirectory();
#define SPDF_API(name) decltype(&::name) name = nullptr
    SPDF_API(FPDF_InitLibraryWithConfig);
    SPDF_API(FPDF_DestroyLibrary);
    SPDF_API(FPDF_LoadCustomDocument);
    SPDF_API(FPDF_GetLastError);
    SPDF_API(FPDF_CloseDocument);
    SPDF_API(FPDF_GetPageCount);
    SPDF_API(FPDF_LoadPage);
    SPDF_API(FPDF_ClosePage);
    SPDF_API(FPDF_GetPageWidth);
    SPDF_API(FPDF_GetPageHeight);
    SPDF_API(FPDFBitmap_Create);
    SPDF_API(FPDFBitmap_Destroy);
    SPDF_API(FPDFBitmap_FillRect);
    SPDF_API(FPDFBitmap_GetBuffer);
    SPDF_API(FPDFBitmap_GetStride);
    SPDF_API(FPDF_RenderPageBitmap_Start);
    SPDF_API(FPDF_RenderPage_Continue);
    SPDF_API(FPDF_RenderPage_Close);
    SPDF_API(FPDFText_LoadPage);
    SPDF_API(FPDFText_ClosePage);
    SPDF_API(FPDFText_CountChars);
#undef SPDF_API
private:
    HMODULE module_ = nullptr;
};
}
