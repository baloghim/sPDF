#include "PdfDocument.h"
#include <limits>

namespace spdf {
int PdfDocument::readBlock(void* context, unsigned long offset, unsigned char* buffer, unsigned long size) {
    auto* self = static_cast<PdfDocument*>(context);
    if (offset > self->access_.m_FileLen || size > self->access_.m_FileLen - offset) return 0;
    LARGE_INTEGER position{};
    position.QuadPart = offset;
    DWORD received = 0;
    return SetFilePointerEx(self->file_, position, nullptr, FILE_BEGIN) &&
        ReadFile(self->file_, buffer, size, &received, nullptr) && received == size;
}
PdfDocument::PdfDocument(PdfiumLibrary& api, const std::wstring& path, const std::string& password) : api_(api) {
    // Retain an immutable read-only handle; no write, truncate, or save path exists here.
    file_ = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS, nullptr);
    if (file_ == INVALID_HANDLE_VALUE)
        throw PdfError(L"A fájl nem nyitható meg. Ellenőrizze az elérési utat és az olvasási jogosultságot.");
    try {
        LARGE_INTEGER length{};
        if (!GetFileSizeEx(file_, &length) || length.QuadPart <= 0 ||
            static_cast<unsigned long long>(length.QuadPart) > std::numeric_limits<unsigned long>::max())
            throw PdfError(L"A fájl üres, nem olvasható vagy 4 GiB-nál nagyobb.");
        access_.m_FileLen = static_cast<unsigned long>(length.QuadPart);
        access_.m_Param = this;
        access_.m_GetBlock = &PdfDocument::readBlock;
        document_ = api_.FPDF_LoadCustomDocument(&access_, password.empty() ? nullptr : password.c_str());
        if (!document_) {
            const auto error = api_.FPDF_GetLastError();
            if (error == FPDF_ERR_PASSWORD) throw PdfError(L"A dokumentum jelszóval védett, vagy a megadott jelszó hibás.", error);
            throw PdfError(L"A PDF sérült, nem támogatott vagy nem olvasható.", error);
        }
        count_ = api_.FPDF_GetPageCount(document_);
        if (count_ <= 0) throw PdfError(L"A dokumentum nem tartalmaz megjeleníthető oldalt.");
    } catch (...) {
        if (document_) api_.FPDF_CloseDocument(document_);
        CloseHandle(file_); file_ = INVALID_HANDLE_VALUE;
        throw;
    }
}
PdfDocument::~PdfDocument() {
    if (document_) api_.FPDF_CloseDocument(document_);
    if (file_ != INVALID_HANDLE_VALUE) CloseHandle(file_);
}
std::wstring PdfDocument::identity() const {
    BY_HANDLE_FILE_INFORMATION info{};
    if (!GetFileInformationByHandle(file_, &info) || (!info.nFileIndexHigh && !info.nFileIndexLow)) return {};
    return std::to_wstring(info.dwVolumeSerialNumber) + L":" +
        std::to_wstring(info.nFileIndexHigh) + L":" + std::to_wstring(info.nFileIndexLow);
}
FPDF_PAGE PdfDocument::loadPage(int index) {
    if (index < 0 || index >= count_) throw PdfError(L"Érvénytelen oldalszám.");
    auto page = api_.FPDF_LoadPage(document_, index);
    if (!page) throw PdfError(L"Az oldal nem tölthető be.");
    return page;
}
}
