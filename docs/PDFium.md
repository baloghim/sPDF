# PDFium – rögzítés és kompatibilitási állapot

- Verzió: **134.0.6996.0**, `chromium/6996` kiadás.
- Bináris előállító: [bblanchon/pdfium-binaries](https://github.com/bblanchon/pdfium-binaries).
- Archívum: https://github.com/bblanchon/pdfium-binaries/releases/download/chromium/6996/pdfium-win-x64.tgz
- SHA-256: `83e714c302ceacccf403826d5cb57ea39b77f393d83b8d5781283012774a9378`.
- Windows x64. HTTPS-letöltés, kötelező SHA-256 ellenőrzés.
- Szállítói `args.gn`: `pdf_enable_v8=false`, `pdf_enable_xfa=false`, `is_debug=false`, `pdf_is_standalone=true`, `pdf_use_partition_alloc=false`.
- Közvetlen DLL-importok: `KERNEL32.dll`, `ADVAPI32.dll`, `GDI32.dll`, `USER32.dll`. Nincs külön MSVC runtime DLL.
- A teljes szállítói licenc- és harmadikfél-csomagot változtatás nélkül mellékeljük.

Ez a technológiai alap rögzített, V8/XFA nélküli motorverziója, nem a mindenkori legfrissebb PDFium. A stabil kiadás előtt a frissítési és biztonsági döntést újra kell értékelni. Az alkalmazás a C API-n keresztül használja, így tudatos verzió- és ellenőrzőösszeg-módosítással frissíthető.

## Windows 10 1607

**A `v0.1.0-preview` kiadás működését a felhasználó a Windows 10 1607 célrendszeren visszaigazolta, 2026. november 5-én.** Visszajelzése: „A kompatibilitás pipa, a kiadás működik a célrendszeren.” Ez tényleges célrendszeri használatról szóló felhasználói visszajelzés.

A visszaigazolás a GitHubon publikált MSVC x64 kiadásra vonatkozik, a mellékelt PDFium 134.0.6996.0 DLL-lel. A tesztelt ZIP SHA-256 azonosítója: `12813a5b1f36ee5fd958ebc839e7bc3be6a4d0bae4b07c0121ca64294398457f`. A külön kipróbált funkciók részletes listáját a visszajelzés nem tartalmazza.

A szállítói csomag továbbra sem tartalmaz külön Windows 10 1607 garanciát. A célrendszeri visszaigazolás az adott alkalmazáskiadásra érvényes; későbbi PDFium- vagy fordítóverzió cseréjénél a kompatibilitást újra ellenőrizni kell.

Az alkalmazás `_WIN32_WINNT=0x0A00`, `WINVER=0x0A00`, x64 és `asInvoker` beállítással készül. A manifest újabb DPI-viselkedést és PerMonitor visszaesést deklarál; közvetlenül nem hívunk újabb DPI API-t, például `GetDpiForWindow`-t. Az EXE melletti DLL abszolút útvonalról, korlátozott függőségkereséssel töltődik be.

A használt Win32 alap API-k elérhetők a célrendszeren. A ZIP-ellenőrzés x64 PE-képet és mellékelt DLL-eket ellenőriz; a célrendszeri működést a fenti felhasználói visszajelzés igazolja.

A további kiadások célrendszeri ellenőrzéseihez: indítás, szöveges/szkennelt/titkosított PDF, Unicode-fájlnév, lapozás, renderelés, több DPI-beállítás és az importkészlet. Az indulási idő és memóriahasználat mérését külön kell dokumentálni; ilyen méréseket a mostani visszajelzés nem tartalmaz.
