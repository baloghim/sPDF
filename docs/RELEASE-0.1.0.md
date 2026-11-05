Az sPDF Megtekintő első kipróbálható, hordozható Windows x64 fejlesztői változata.

Töltse le az **sPDF-0.1.0-preview-win-x64.zip** fájlt, bontsa ki, majd az `sPDF` mappában indítsa az `sPDF.exe` alkalmazást. A `pdfium.dll` maradjon az EXE mellett. Telepítés és .NET nem szükséges.

Elkészült: magyar Win32 főablak, valódi PDFium-renderelés háttérszálon, PDF-megnyitás, lapozás, nagyítás, illesztés, ideiglenes forgatás, jelszókezelés és Unicode-fájlnevek. Az eredeti PDF kizárólag olvasásra nyílik meg.

Ez a ZIP a GitHub Actions Windows x64 MSVC Release buildjének ellenőrzött eredménye: a PDFium integrációs tesztjei, az első oldal főablakban történő megjelenítési próbája és a hordozható csomag ellenőrzése sikeresen lefutott a publikálás előtt. A mellékelt `.sha256` fájl a ZIP ellenőrzőösszege.

**A kiadás működését a felhasználó a Windows 10 1607 célrendszeren visszaigazolta (2026. november 5.).** Ez a jelenleg publikált `v0.1.0-preview` csomagra vonatkozik. Részletes funkciólista és teljesítménymérés nem szerepelt a visszajelzésben. A rögzített PDFium 134.0.6996.0 történeti alapverzió, stabil kiadás előtt frissítési felülvizsgálat szükséges.

Ez még nem az 1.0 változat. Egyszerre egy PDF kezelhető. Dokumentumfülek, folyamatos nézet, Oldalrendező, szövegkereső felület, PDF-export és nyomtatás még nincs benne.

A licencek és a ténylegesen használt KDE Modern Classic ikonok SVG-forrásai a csomag részei. Részletek a repository README-jében és a `docs/PDFium.md` fájlban.
