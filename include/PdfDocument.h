#pragma once
#include "PdfiumLibrary.h"
#include <cstdint>
#include <string>
#include <stdexcept>

namespace spdf {
class PdfError : public std::runtime_error {
public:
    PdfError(std::wstring message, unsigned long code = 0)
        : std::runtime_error("PDF operation failed"), message(std::move(message)), code(code) {}
    std::wstring message;
    unsigned long code;
};
class PdfDocument {
public:
    PdfDocument(PdfiumLibrary& api, const std::wstring& path, const std::string& password = {});
    ~PdfDocument();
    PdfDocument(const PdfDocument&) = delete;
    PdfDocument& operator=(const PdfDocument&) = delete;
    int pageCount() const { return count_; }
    std::uint64_t byteSize() const { return access_.m_FileLen; }
    std::wstring identity() const;
    FPDF_PAGE loadPage(int index);
    PdfiumLibrary& api() { return api_; }
private:
    static int readBlock(void* context, unsigned long offset, unsigned char* buffer, unsigned long size);
    PdfiumLibrary& api_;
    HANDLE file_ = INVALID_HANDLE_VALUE;
    FPDF_FILEACCESS access_{};
    FPDF_DOCUMENT document_ = nullptr;
    int count_ = 0;
};
}
