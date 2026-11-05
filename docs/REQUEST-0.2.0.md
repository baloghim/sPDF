# sPDF Megtekintő – 0.2.0-preview fejlesztési feladat

Repository: https://github.com/baloghim/sPDF

Folytasd a meglévő alkalmazás fejlesztését. A 0.1.0-preview már rendelkezik működő Win32/PDFium alappal, ezért **ne írd újra a programot nulláról**.

A teljes 1.0 specifikáció a `docs/SPECIFICATION.md` fájlban található. Ez továbbra is irányadó.

## 1. A következő mérföldkő célja

Készítsd el a 0.2.0-preview verziót, amelynek két fő célja:

1. A felhasználói felület közelítése a jóváhagyott modern, világos Windows 10-es látványtervekhez.
2. Több PDF-dokumentum egyidejű megnyitása és kezelése füleken.

A működő PDF-renderelést, lapozást, nagyítást, fájlmegnyitást és jelszókezelést őrizd meg.

## 2. Főablak átalakítása

A végleges alkalmazás elrendezése felülről lefelé:

- Windows 10-hez illeszkedő világos címsor.
- Hagyományos magyar menüsor.
- Jobb oldalon keresőmező.
- Modern, kompakt ikonos eszköztár.
- Több dokumentum kezelésére alkalmas fülsor.
- Szükség esetén megjelenő navigációs és nézetfüggő vezérlősor.
- PDF-megjelenítő munkaterület.
- Alsó állapotsor.

Az aktuális 0.1.0 felület csak prototípus. Az új elrendezést ne korlátozza a jelenlegi vezérlők pozíciója.

A felület legyen rendezett és arányos 1366×768 képernyőfelbontáson is. A szokásos Windows 10 DPI-skálázási beállítások mellett is működjön.

A PDF-megjelenítő terület maradjon a lehető legnagyobb.

## 3. KDE Modern Classic ikonok

A repositoryban már megtalálható:

`KDE-Modern-Classic-20261027.tar.gz`

Valamint több, ebből előállított ikon a `resources/icons/` könyvtárban.

**Kizárólag a tényleges KDE Modern Classic ikonokat használd.**

A jelenlegi eszköztáron az ikonok túl kicsik és nem eléggé hangsúlyosak.

Alakíts ki egységes megjelenést:

- 24×24 pixeles ikonok megfelelő DPI-skálázással.
- Egyenletes térközök.
- Azonos vizuális stílus.
- Átlátható gombcsoportok.
- Jól látható aktív és inaktív állapotok.
- Eszköztippek minden fontos ikonhoz.

A nagyítás és kicsinyítés ikonjai azonos stílusúak legyenek.

Ne helyettesítsd a készletet Windows alapikonokkal vagy generált alternatívákkal.

Ha szükséges, módosítsd a jelenlegi Toolbar megvalósítását, de ne vezess be nehéz grafikus keretrendszert.

## 4. Eszköztár

A fő eszköztár tartalmazza:

- Megnyitás.
- Nyomtatás.
- Előző és következő oldal.
- Nagyítás és kicsinyítés.
- Oldalhoz illesztés.
- Forgatás.
- Nézetválasztás: Egy oldal, Folyamatos, Oldalrendező.

A még nem megvalósított funkciók gombjai lehetnek inaktívak, de ne indítsanak megtévesztő vagy nem működő műveletet.

Az általános Mentés gombot ne helyezd vissza az eszköztár elejére.

Az oldalak PDF-be mentése másodlagos funkció legyen.

## 5. Dokumentumfülek

Valósítsd meg több PDF egyidejű megnyitását.

Minden új dokumentum külön fülön jelenjen meg.

A fülek működése:

- Fájlnév megjelenítése.
- Bezárás X gombbal.
- Aktív fül megkülönböztetése.
- Váltás kattintással.
- Ctrl+W az aktuális fül bezárására.
- Több fájl egyidejű behúzása.
- Hibás dokumentum ne zárja be a többi fület.
- Azonos dokumentum ismételt megnyitásakor a meglévő fül aktiválása.

Minden dokumentum saját oldalszámot, nagyítást, forgatást és görgetési pozíciót tároljon.

A dokumentumállapot kezelését válaszd el a főablak megjelenítésétől, hogy a későbbi Oldalrendező és munkaváltozat megvalósítható legyen.

## 6. Állapotsor

Az állapotsor legyen állandóan látható, és mutassa:

- Aktuális oldal / összes oldal.
- Nagyítás.
- Papírméret.
- Dokumentum fájlmérete.
- Aktuális nézet.
- Betöltési vagy készenléti állapot.

Az információkat elkülönített, rendezett mezőkben jelenítsd meg.

## 7. Menürendszer előkészítése

A menüsor az 1.0 specifikáció szerinti szerkezetre épüljön:

Fájl | Szerkesztés | Nézet | Oldalak | Eszközök | Súgó

A még nem működő menüpontokat tiltsd le, vagy csak akkor jelenítsd meg, ha az adott funkció valóban elkészült.

## 8. Névjegy

Ellenőrizd, hogy a Névjegy tartalmazza:

- sPDF Megtekintő.
- Aktuális verziószám.
- „Könnyű, gyors PDF-megtekintő Windowsra.”
- Kattintható https://www.edutech.hu hivatkozás.
- Licencek és köszönetnyilvánítás elérhetősége.

## 9. Teljesítmény és kompatibilitás

Továbbra is kötelező:

- Natív C++/Win32.
- PDFium.
- .NET nélkül.
- Telepítés nélkül.
- Minimális memóriaigény.
- Régi HDD-s számítógépekhez optimalizálva.
- Windows 10 1607 x64 célrendszer.

A háttérben megnyitott dokumentumfülek ne rendereljenek szükségtelenül.

A PDFium szálbiztonsági korlátait tartsd be.

Ne állítsd igazoltnak a Windows 1607 kompatibilitását tényleges célgépes teszt nélkül.

## 10. Kiadás

A következő verzió legyen:

`0.2.0-preview`

A GitHub Actions készítsen:

`sPDF-0.2.0-preview-win-x64.zip`

A ZIP egyetlen `sPDF` mappát tartalmazzon, benne az EXE-vel, a szükséges DLL-ekkel és licencfájlokkal.

A README elején a ténylegesen letölthető legfrissebb kiadásra mutató hivatkozás szerepeljen.

Az új kiadást csak sikeres fordítás és ellenőrzések után publikáld.

Készíts képernyőképet a ténylegesen futó alkalmazásról, és helyezd el a `docs/screenshots/` könyvtárban.

## 11. Elfogadási feltételek

A mérföldkő akkor teljesült, ha:

1. A meglévő PDF-funkciók továbbra is működnek.
2. A főablak modern, világos, rendezett megjelenést kapott.
3. A tényleges KDE Modern Classic ikonok jól felismerhetők.
4. Több PDF megnyitható külön füleken.
5. A fülek között váltva megmaradnak a dokumentumállapotok.
6. A főablak megfelelően alkalmazkodik az átméretezéshez.
7. Az állapotsor megfelelően működik.
8. A GitHub Actions elkészíti a hordozható ZIP-et.
9. A GitHubon elérhető a tesztelhető kiadás.
10. A README és a fejlesztési dokumentáció tükrözi a tényleges állapotot.

A fejlesztést tényleges kódmódosításokkal végezd. Ne állj meg a tervezésnél.

A végén adj rövid beszámolót az elkészült funkciókról, a tesztek eredményéről, a fennmaradó hiányokról és a letöltési hivatkozásról.

A teljes 1.0 specifikáció későbbi funkcióit ne hagyd el, de ebben a mérföldkőben a főablak és a dokumentumfülek minőségi megvalósítására koncentrálj.