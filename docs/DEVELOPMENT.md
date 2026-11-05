# 0.2.0-preview – fejlesztési és tesztjelentés

2026. november 5. A meglévő Win32/PDFium alapra épült tovább a program; a dokumentumolvasó, a PDFium C API és a renderelési műveletek megmaradtak. Az új mérföldkő a főablakot és a többfüles dokumentumkezelést valósítja meg. A teljes 1.0 követelményei a `SPECIFICATION.md`, a mostani feladat a `REQUEST-0.2.0.md` fájlban olvashatók.

## Elkészült

- Világos, átméretezhető főablak: menüsor, jobb oldali letiltott keresőmező, csoportosított eszköztár, fülsor, szükség esetén külön nézetvezérlősor, dokumentumterület és állandó állapotsor.
- Az eredeti KDE készletből 11 ténylegesen használt ikon, köztük a nyomtató és a keresés. Az eszköztári képek 24×24 pixelesek 96 DPI-n, más DPI-n arányosan skálázódnak.
- Több fájl megnyitása fájlválasztóból, behúzással és parancssorból. A már megnyitott útvonal aktiválja a meglévő fület; a worker fájlazonosító alapján is deduplikál.
- X-es bezárás, Ctrl+W, Ctrl+Tab / Ctrl+Shift+Tab, rövidített fájlnevek, teljes útvonalas eszköztipp, aktív fül kék jelölése; a natív tab control túlcsordulási és billentyűzetkezelése.
- Önálló dokumentumállapot: oldal, zoom, illesztési mód, megjelenítési forgatás, vízszintes/függőleges görgetési pozíció.
- Hat külön állapotsormező: oldal, nagyítás, papírméret, fájlméret, nézet, aktuális állapot.
- A hibás fájl és a jelszó megszakítása csak az érintett új fület zárja; a többi dokumentum nyitva marad.
- Névjegy 0.2.0 verzióval, magyar leírással, kattintható edutech.hu és licenchivatkozással.

A keresés, nyomtatás, Folyamatos és Oldalrendező nézet, szerkesztés, export és beállításmentés letiltott előkészítés. Ezeket nem jelöljük működő funkciónak. A teljes 1.0 megvalósítása továbbra is nyitott.

## Architektúra és erőforrások

A `TabManager`/`DocumentState` modell nem tartalmaz ablakvezérlőket vagy PDFium-handle-öket. A `DocumentTabBar` a natív tab control könnyű megjelenítési rétege; a `MainWindow` a `spdf_ui` könyvtárban maradt, így a teszt ugyanazt a felületkódot használja.

Minden PDFium-művelet egyetlen worker szálon történik, az inicializálástól a dokumentumok és a könyvtár lezárásáig. A worker dokumentumtérképet és külön vezérlőkérés-sort tart fenn: a megnyitás és a bezárás nem veszhet el renderkérések összevonása miatt. Fülváltáskor az előző render megszakad/eldobódik; az inaktív fül renderkérését a worker elutasítja. A háttérben megnyitott dokumentumhoz csak metaadatot tölt, bitképet nem készít.

A felület csak az aktív nézet teljes felbontású bitképét tartja. Nincs lemezes rendercache, PDF-tartalom vagy előzmény automatikus mentése. A 48 MiB bitképkorlát megmaradt; ez nem a teljes folyamat memóriahatára. Teljesítmény- és RAM-mérést nem helyettesít ez a tervezési korlát.

## Végrehajtott helyi ellenőrzések

MinGW-w64 x64 Release fordítás: sikeres. CMake/CTest, Wine 10.0 és Xvfb alatt **3 tesztcsomag, összesen 66 sikeres ellenőrzés**, 0 hiba, 0 kihagyott teszt:

- `pdfium_integration`: **22 ellenőrzés**, beleértve a tényleges színes képtartalmat, szövegréteget, jelszót, Unicode-útvonalat, eredeti fájl változatlanságát és 150%-os DPI melletti pixelskálázást.
- `document_tabs`: **27 ellenőrzés**; stabil azonosítók, állapotmegőrzés, duplikáció, fülbezárás, két PDFium-dokumentum, inaktív render tiltása, hibás dokumentum elkülönítése és gyors egymás utáni nyitás/bezárás.
- `window_tabs`: **17 ellenőrzés** a valódi megosztott Win32 felületen, külön GUI-szálon. Valódi fülkattintás, X és Ctrl+W, hat státuszmező, 1366×768 és szűk ablak, nagyított oldal görgetése, visszaálló képtartalom képpontos összehasonlítása, hibás és jelszavas PDF párbeszéde.

A Win32 felület tesztje ugyanabban a tesztfolyamatban futtatja a megosztott ablakmodult, mert a Wine folyamatközi `WM_PRINT`/GDI támogatása korlátozott. A fő EXE-t külön `--smoke-test` is ellenőrzi; a kibontott ZIP-ből két Unicode-nevű PDF megnyitása és megjelenítése sikeres, kilépési kód 0.

A helyi `sPDF-0.2.0-preview-win-x64.zip` CPack-csomag egyetlen `sPDF` mappát és 43 bejegyzést tartalmaz. ZIP CRC, x64 PE, EXE/DLL-függőségek, szükséges licencek és a tesztprogramok kizárása: sikeres. Nincs külön GCC vagy VC runtime DLL a csomagban.

## Kijavított hibák

- A tab control natív elrendezési lépését megtartottuk az egyedi kirajzolás előtt; így a fülek és a túlcsordulási vezérlő helyzete helyes marad.
- A natív toolbar címkézett és csak ikonos gombjainak szélessége külön beállítást kapott, így a szöveges Megnyitás gomb nem növeli meg az összes ikongombot.
- A felületi teszt tényleges görgetési hibát talált: fülváltáskor az újra megjelenő görgetősáv néhány pixellel csökkenthette a visszaállított pozíciót. A sávok láthatósága most a tárolt pozíció korlátozása előtt rendeződik. A javítást a kirajzolt oldal képpontos visszaállítási tesztje ellenőrzi.

## Képernyőkép

`docs/screenshots/preview-0.2.0-wine.png`: a tényleges alkalmazás három PDF-füllel, a virtuális képernyőn látható felületről készített képernyőkép. A bemutató PDF-ek mesterségesek és nem tartalmaznak személyes adatot. Ez nem látványterv és nem Windows 10 1607 futtatási bizonyíték.

## Kiadás és fennmaradó ellenőrzések

A `.github/workflows/windows.yml` a 0.2.0 verziót konfigurálja: natív MSVC x64 Release build, mindhárom CTest-csomag, fő EXE megjelenítési próba, csomag-/DLL-ellenőrzés, majd artifact és a `v0.2.0-preview` címkén fejlesztői Release. A natív szerveroldali eredmény a sikeres futás után kerül ide.

A 0.1.0 Windows 10 1607 célrendszeri működését a felhasználó már visszaigazolta; ez a feljegyzés megmaradt a `PDFium.md` és `DEVELOPMENT-0.1.0.md` fájlokban. A 0.2.0 külön célrendszeri tesztet igényel. A 150%-os bitmapteszt nem helyettesíti a többmonitoros, fizikai DPI-váltás tesztjét. HDD-s indulási idő és RAM-mérés, nyomtatóintegráció, keresés, folyamatos nézet, Oldalrendező és PDF-export későbbi mérföldkő.
