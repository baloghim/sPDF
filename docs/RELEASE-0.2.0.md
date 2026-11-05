Az sPDF Megtekintő **0.2.0-preview** kiadása: új világos Win32 főablak és több PDF-dokumentum kezelése füleken.

**Letöltés:** `sPDF-0.2.0-preview-win-x64.zip`. Bontsa ki, majd az `sPDF` mappában indítsa az `sPDF.exe` fájlt. A `pdfium.dll` maradjon az EXE mellett. Telepítés és .NET nem szükséges.

Újdonságok:

- Rendezett magyar menük, 24×24 pixeles, DPI szerint skálázott eredeti KDE Modern Classic ikonok és hatrészes állapotsor.
- Több PDF megnyitása egy választással, behúzással vagy parancssorból; duplikált fájl esetén a meglévő fül aktiválása.
- Fülenként megmaradó oldal, nagyítás/illesztés, forgatás és görgetési pozíció.
- X-es és Ctrl+W-s bezárás, Ctrl+Tab / Ctrl+Shift+Tab fülváltás, hosszú fájlnevek rövidítése és teljesútvonal-eszköztipp.
- Hibás vagy jelszavas dokumentumok kezelése a többi fül elvesztése nélkül.
- A háttérfülek nem renderelnek, a PDFiumot egyetlen munkaszál használja. Az eredeti PDF-ek továbbra is kizárólag olvashatók.

A kiadás a GitHub Actions **natív Windows x64 MSVC Release buildjéből** készült, csak sikeres PDFium-, dokumentumfül- és felületi integrációs tesztek, elsőoldal-megjelenítési próba és ZIP-/DLL-ellenőrzés után. A `.sha256` fájl a csomag ellenőrzőösszege.

**Még letiltott:** keresés, nyomtatás, Folyamatos és Oldalrendező nézet, PDF-export, szerkesztés és beállításmentés. Ezek a teljes 1.0 későbbi mérföldkövei.

**Windows 10 1607:** a 0.1.0 működését a felhasználó a célrendszeren visszaigazolta. A 0.2.0 új felülete és többfüles működése külön célgépes tesztet igényel; ezt még nem állítjuk igazoltnak. A rögzített PDFium verziója változatlan: 134.0.6996.0.

A csomag az EXE-t, a szükséges PDFium DLL-t, a licenceket és a használt ikonok SVG-forrásait egyetlen `sPDF` mappában tartalmazza. Képernyőkép és részletes tesztjelentés a repository `docs/` könyvtárában.
