# 0.1.0 fejlesztői előzetes – jelentés

2026. november 5. Az 1.0 specifikáció első technológiai mérföldkövéből működő, kipróbálható alap készült. Nem állítjuk késznek az 1.0 verziót vagy a teljes első mérföldkövet: a cél-Windows kompatibilitási próbája nyitott.

## Előállított helyi MinGW-csomag

`dist/sPDF-0.1.0-preview-win-x64.zip`, 3 927 872 bájt. Egyetlen felső szintű `sPDF` mappa, benne `sPDF.exe`, `pdfium.dll`, `README.txt`, licencek és a használt ikonok forrásai. Telepítő, .NET vagy külön GCC runtime DLL nincs benne és nem szükséges. SHA-256:

```text
10272a9cb9d4e428f4d9fa2829536824b68a6703c3e8df4bc3a1952b0ec97e08
```

Ez a korábbi helyi MinGW keresztfordítás csomagja. A később publikált GitHub-kiadás natív MSVC buildből készült, ezért a két ZIP mérete és ellenőrzőösszege eltér.

## GitHubon publikált Windows-kiadás

A [v0.1.0-preview fejlesztői kiadás](https://github.com/baloghim/sPDF/releases/tag/v0.1.0-preview) elérhető. A [Windows x64 ZIP](https://github.com/baloghim/sPDF/releases/download/v0.1.0-preview/sPDF-0.1.0-preview-win-x64.zip) **3 712 618 bájt**; SHA-256:

```text
12813a5b1f36ee5fd958ebc839e7bc3be6a4d0bae4b07c0121ca64294398457f
```

A [42031609343 számú GitHub Actions futás](https://github.com/baloghim/sPDF/actions/runs/42031609343) Windows Server 2022 runneren, MSVC x64 Release konfigurációban sikeres fordítást, PDFium-integrációs tesztet, főablak-megjelenítési próbát, ZIP- és függőségellenőrzést végzett. Csak ezek után publikálta a fejlesztői kiadást és a ZIP-et az ellenőrzőösszegével. A kiadási címke a `0f8808847232efeb65364e144eb1ce0bb2b43dbc` forráscommitot jelöli.

A GitHubról visszatöltött ZIP SHA-256 ellenőrzése és a `check-package.py` vizsgálata is sikeres. A csomag 39 bejegyzést tartalmaz, az EXE és a PDFium x64 architektúrájú, közvetlenül csak Windows rendszer-DLL-eket importál. Ez a natív Windows build ellenőrzése; a Windows 10 1607 kompatibilitását továbbra sem igazolja.

## Végrehajtott ellenőrzések

- Tiszta Release keresztfordítás a repó `cmake/mingw-x64.cmake` toolchainjével: **sikeres**, x64 PE EXE és erőforrások; CMake 3.31.6, Ninja, Debian MinGW-w64 GCC 14 POSIX.
- A PDFium CMake-en keresztüli HTTPS-letöltése, TLS- és rögzített SHA-256 ellenőrzése: **sikeres**.
- A Windows teszt-EXE a mellékelt Windows PDFium DLL-lel, Wine 10.0 alatt: **21 ellenőrzés sikeres, 0 hiba**, kilépési kód 0.
- A tényleges Win32 főablak `--smoke-test` próbája: **sikeres**; megnyitotta, háttérszálon renderelte és a dokumentumterületen kirajzolta a PDF első oldalát, majd kilépett 0 kóddal.
- A ZIP kibontott példányából indított `sPDF.exe` megjelenítési próbája: **sikeres**, kilépési kód 0.
- ZIP CRC, egyetlen `sPDF` gyökérmappa, szükséges állományok/licencek, EXE és DLL x64 architektúra, közvetlen DLL-függőségek: **sikeres**, 39 archívumbejegyzés. Az EXE csak rendszer-DLL-eket importál; a PDFiumot futáskor az EXE mellől tölti be.
- A futó alkalmazás tényleges képe: `docs/screenshots/preview-wine.png`.

A 21 integrációs ellenőrzés: háromoldalas PDF megnyitása; fájlhossz; szövegréteg; bitmapméret; az első oldal valódi piros és a második valódi kék képtartalma; forgatás; teljesoldal- és szélességillesztés; renderelés megszakítása; 48 MiB bitképkorlát; negatív és túlfutó oldalszám; eredeti fájl bájtonkénti változatlansága; sérült és hiányzó fájl; jelszókérés, hibás és helyes jelszó; Unicode/Hungarian fájlnév; különálló dokumentumkezelők.

## Javított hibák

- A helyileg áthelyezett MinGW toolchain windres előfeldolgozóját explicit beállítottuk, így a Windows fejléceket használja.
- A Windows `LONG` és C++ `int` közötti eltérést a felületi elrendezésben javítottuk.
- A teszt tényleges futása során talált lebegőpontos illesztési kerekítést javítottuk: például a 200 × 2,3 eredménye nem veszít el egy pixelt.

## Működő felület

Magyar menük, világos Win32 főablak, valódi KDE ikonok, oldalszámmező és állapotsor. Egy PDF nyitható meg párbeszédablakból, behúzással vagy parancssorból. Lapozás, nagyítás, illesztés, görgetés és ideiglenes forgatás működik. A PDFium egyetlen háttérszálra korlátozott; az elavult renderelések megszakíthatók/eldobódnak. Az eredeti dokumentum írására nincs kódút.

A mesterséges teszt-PDF-ek ReportLab és pypdf használatával készültek; nincs bennük személyes adat. A `sample.pdf` három, eltérő színű és feliratú oldal; az `encrypted.pdf` ennek RC4-128 jelszavas változata (`spdf-test`); a `broken.pdf` szándékosan nem PDF.

## Nyitott és nem végrehajtott ellenőrzések

- **Tényleges Windows 10 1607 futtatás nem történt.** A PDFium szállítója nem ad erre a buildre konkrét garanciát. Lásd `PDFium.md`.
- A natív MSVC fordítás és tesztek GitHub Actions Windows Server 2022 runneren már sikeresek; a Windows 10 1607 és a célhardver külön ellenőrzése maradt nyitva.
- Fizikai nyomtató, HDD-s célhardver, indulási idő és teljes processzmemória nincs tesztelve/mérve. A jelenlegi változat nem tartalmaz nyomtatási funkciót.
- A jelszókérő párbeszéd UI-kezelése, a többmonitoros DPI-váltás és a nagy valós dokumentumkészlet manuális Windows-próbája nyitott. A titkosított PDF motoroldali megnyitása tesztelt.
- A rögzített PDFium történeti alapverzió; stabil kiadás előtt frissítési és biztonsági felülvizsgálat szükséges.

## Következő funkcionális mérföldkövek

Dokumentumfülek és közös munkaváltozat, folyamatos nézet, keresés, INI-beállítások; majd Oldalrendező stabil oldalazonosítóval, visszavonás/újra, minőségmegőrző PDF-export és saját nyomtatási felület. Végül célhardvermérés, Windows 10 1607 teszt és hivatalos 1.0 kiadás. A jelenlegi kiadás egyiket sem tünteti fel elkészültként.

A létrehozott fájlok teljes felsorolása: `FILES.txt`. A korábbi README tartalmát kibővítettük, az eredeti ikontéma-archívumot változatlanul megtartottuk.
