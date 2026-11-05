# CODEX FEJLESZTÉSI FELADAT
# sPDF Megtekintő – 1.0

## 1. A projekt meghatározása

Fejlessz egy **sPDF Megtekintő** nevű, natív Windows-alkalmazást, amely PDF-dokumentumok gyors megtekintésére, nyomtatására és alapvető oldalrendezésére szolgál.

A program elsősorban régi, lassú, merevlemezes (HDD) munkahelyi számítógépekre készül. A dokumentumok jelentős része szkennelt, vagyis képként tartalmazza az oldalakat.

A fejlesztés során a legfontosabb szempontok:

1. Gyors programindítás.
2. Alacsony memória- és processzorigény.
3. Minimális lemezművelet és függőség.
4. Egyszerű, magyar nyelvű kezelőfelület.
5. Megbízható PDF-megjelenítés és nyomtatás.
6. Az eredeti dokumentumok teljes védelme.
7. Telepítés nélküli, hordozható működés.
8. Könnyen tesztelhető GitHub-kiadások.

A cél nem egy Adobe Acrobat Reader méretű és összetettségű alkalmazás, hanem egy célorientált, könnyű és gyors PDF-eszköz.

### Projektadatok

- **Alkalmazás neve:** sPDF Megtekintő
- **Projekt neve:** sPDFViewer
- **Futtatható állomány:** sPDF.exe
- **GitHub repository:** https://github.com/baloghim/sPDF
- **Alapértelmezett ág:** main
- **Első tervezett verzió:** 1.0.0
- **Felület nyelve:** magyar
- **Célarchitektúra:** x64
- **Minimum operációs rendszer:** Windows 10 Version 1607, build 14393
- **Terjesztés:** hordozható ZIP
- **Fejlesztési nyelv:** C++
- **Kezelőfelület:** natív Win32 API
- **PDF-motor:** PDFium

A repositoryban dolgozz, tartsd meg a már meglévő fájlokat és előzményeket, ha vannak. A változtatásokat logikus, ellenőrizhető egységekben végezd.

---

## 2. Technológiai követelmények

A program natív C++ Windows-alkalmazás legyen.

Követelmények:

- Win32 alapú ablakkezelés.
- PDFium könyvtár használata.
- x64 fordítás.
- Unicode-fájlnevek és magyar ékezetek támogatása.
- CMake-alapú fordítási konfiguráció.
- Lehetőleg minél kevesebb külső DLL.
- Ne igényeljen külön telepített .NET Frameworköt vagy modern .NET-futtatókörnyezetet.
- Ne igényeljen rendszergazdai jogosultságot.
- Ne igényeljen internetkapcsolatot.

Ne használj Electront, WebView-t, beágyazott böngészőt vagy más, indokolatlanul nagy futtatókörnyezetet.

A Visual C++ futtatókönyvtárainak szükséges részeit statikusan linkeld vagy mellékeld az alkalmazáscsomaghoz, a licenc- és kompatibilitási feltételek betartásával.

### Windows 10 1607 kompatibilitás

Ez kritikus követelmény.

Nem elegendő, hogy az alkalmazás Windows 11 alatt vagy egy modern GitHub Actions runneren működik.

Ellenőrizd:

- A PDFium kiválasztott verziójának minimális Windows-követelményeit.
- A használt Windows API-k elérhetőségét.
- A fordítási célrendszer beállításait.
- A DLL-függőségeket.
- A program futtatható állományának Windows-kompatibilitását.

A PDFium verzióját rögzítsd reprodukálható módon, dokumentált forrásból, verzió- vagy commitazonosítóval és lehetőség szerint ellenőrzőösszeggel.

Ha a legújabb PDFium-verzió nem működik a célrendszeren, keress korábbi, megfelelő verziót, és dokumentáld a döntést.

A kompatibilitást tényleges Windows 10 1607 környezetben is tesztelni kell. Ha ilyen környezet nem áll rendelkezésedre, ezt kifejezetten jelezd. Ne állítsd, hogy a kompatibilitás bizonyított pusztán sikeres fordítás alapján.

---

## 3. A grafikus kezelőfelület

A program modern, világos, letisztult Windows 10-es megjelenést kapjon.

Vizuális alapelv:

- WPF-szerűen tiszta, modern desktop megjelenés.
- Natív C++/Win32 megvalósítás.
- Világos háttér.
- Visszafogott szürke felületi elemek.
- Kék kijelölések és aktív állapotok.
- Színes KDE Modern Classic ikonok.
- Kompakt, jól áttekinthető elrendezés.
- A dokumentum megjelenítésére fordított terület legyen a lehető legnagyobb.

Ne használj Windows XP-szerű vagy KDE 3-as ablakkeretet. A KDE Modern Classic kizárólag az ikonok vizuális stílusát határozza meg.

Használj megfelelő Windows DPI-kezelést, hogy a felület nagy felbontású monitorokon és eltérő skálázási beállítások mellett is olvasható legyen.

### 3.1. A főablak szerkezete

Felülről lefelé:

1. Windows címsor.
2. Hagyományos menüsor.
3. Dokumentumfülek.
4. Kompakt, ikonos eszköztár.
5. Dokumentummegjelenítő munkaterület.
6. Alsó állapotsor.

A keresőmező a felső sáv jobb oldalán helyezkedjen el.

A bal oldalon opcionálisan megjeleníthető, elrejthető navigációs oldalsáv legyen. Alapértelmezésben ne foglaljon helyet.

### 3.2. Dokumentumfülek

A felhasználó több PDF-et nyithat meg egyszerre.

Minden dokumentum külön fülön jelenjen meg.

Funkciók:

- Új dokumentum megnyitása új fülön.
- Fülváltás.
- Fül bezárása.
- Aktív fül egyértelmű jelölése.
- Hosszú fájlnevek rövidítése.
- Teljes fájlnév megjelenítése súgóbuborékban.
- Megnyitás drag & drop segítségével.
- Megnyitás Windows parancssori fájlargumentummal.

Minden fülnek saját dokumentumállapota legyen, beleértve a lapozási pozíciót, nagyítást, ideiglenes szerkesztéseket és kijelöléseket.

Egy már megnyitott PDF újabb megnyitása lehetőleg a meglévő fülre váltson.

Az alkalmazás támogassa, hogy egy újabb fájlmegnyitási kérés a már futó példány új fülén jelenjen meg, ha ez megbízhatóan és biztonságosan megvalósítható.

---

## 4. PDF-megjelenítés

Három külön megjelenítési módot kell megvalósítani.

### 4.1. Egyoldalas nézet

Egyszerre egy PDF-oldalt jelenít meg.

Támogatandó műveletek:

- Előző oldal.
- Következő oldal.
- Első és utolsó oldal.
- Oldalszám közvetlen megadása.
- Egérgörgős navigáció.
- Nagyítás és kicsinyítés.
- Teljes oldal megjelenítése.
- Oldalszélességhez igazítás.
- 100%-os méret.
- Csak megjelenítést érintő ideiglenes forgatás.

### 4.2. Folyamatos nézet

A PDF oldalai egymás alatt, folyamatosan görgethető módon jelenjenek meg.

Követelmények:

- Egyenletes görgetés.
- Oldalak közötti kis térköz.
- Aktuális oldalszám követése.
- Nagyítás és kicsinyítés.
- Ablakhoz és oldalszélességhez illesztés.
- Gyors görgetésnél se renderelje előre az összes oldalt teljes felbontásban.

### 4.3. Oldalrendező nézet

A PDF valamennyi oldala kicsinyített előnézetként, rácsszerű elrendezésben jelenjen meg, a PowerPoint diarendező nézetéhez hasonlóan.

Ez önálló munkaterület, nem pusztán bal oldali bélyegképsáv.

A bélyegképek mérete állítható:

- Ctrl + egérgörgővel.
- Alsó nagyításcsúszkával.

A rács automatikusan alkalmazkodjon az ablak szélességéhez. Nagy felbontású monitorokon több oldal férjen el egymás mellett.

Az Oldalrendező első verzióban támogatja:

- Egyszeres kijelölés.
- Többes kijelölés.
- Oldalak húzással történő átrendezése.
- Oldalak ideiglenes törlése.
- Törölt oldalak megjelenítése és elrejtése.
- Törölt oldalak visszaállítása.
- Kijelölt oldalak 90 fokos forgatása jobbra és balra.
- Visszavonás és újra.
- Kijelölt oldalak nyomtatása.
- Kijelölt oldalak mentése külön PDF-be.
- A teljes munkaváltozat mentése új PDF-be.

A rendezés kizárólag egérhúzással is elvégezhető legyen, külön átrendező gomb nélkül.

Dupla kattintással az adott oldal nyíljon meg egyoldalas nézetben.

---

## 5. Az Oldalrendező kezelési szabályai

### 5.1. Kijelölés

- Bal kattintás: egy oldal kijelölése.
- Ctrl + kattintás: több, nem szomszédos oldal kijelölése.
- Shift + kattintás: összefüggő oldaltartomány kijelölése.
- Ctrl + A: az összes aktív oldal kijelölése.
- Jobb kattintás: helyi menü megnyitása.

Ha több oldal van kijelölve, a kijelölt oldalak egyikén végzett jobb kattintás ne szüntesse meg a kijelölést.

### 5.2. Oldalak átrendezése

Az oldalakat egérrel lehessen húzni.

A program vizuális jelöléssel mutassa az új beszúrási pozíciót.

Több kijelölt oldal együttes húzásakor a kijelölt oldalak egymáshoz viszonyított sorrendje maradjon meg.

Az átrendezés kizárólag a munkaváltozat sorrendjét módosítsa.

### 5.3. Oldalak törlése

Oldalt törölni kizárólag az Oldalrendező nézetben lehessen.

Törlési lehetőségek:

- Delete billentyű.
- Oldalrendező saját eszköztárának törlésgombja.
- Jobb kattintás → Kijelölt oldalak törlése.

Normál megjelenítési módban a törlési művelet ne legyen elérhető.

A törlés:

- Ne módosítsa az eredeti PDF-et.
- Ne igényeljen minden alkalommal megerősítést.
- Legyen visszavonható.
- Azonnal tüntesse el a törölt oldalakat az alapnézetből.
- Az utolsó megmaradt oldal törlését tiltsa meg.

### 5.4. Törölt oldalak megjelenítése

Az Oldalrendező eszköztárába kerüljön egy kétállapotú gomb:

**Törölt oldalak mutatása**

Működése:

- Az első törlés előtt inaktív.
- Az első törlés után aktiválódik.
- Kikapcsolva a törölt oldalak nem láthatók.
- Bekapcsolva a törölt oldalak újra megjelennek a rácsban, de továbbra is törölt állapotban maradnak.

A törölt oldal képe halványítva jelenjen meg, rajta jól látható piros X jelöléssel.

A törölt oldal az utolsó ismert pozícióján jelenjen meg.

Ha az oldalsorrend közben megváltozott, az alkalmazás következetes, dokumentált szabály alapján őrizze meg vagy számítsa ki a törölt oldal megjelenítési pozícióját.

A törölt oldal nem húzható és nem vehető figyelembe a nyomtatásban vagy exportban.

### 5.5. Törölt oldalak visszaállítása

A törölt oldalon jobb kattintva jelenjen meg:

- Oldal visszaállítása.
- Kijelölt törölt oldalak visszaállítása.
- Összes törölt oldal visszaállítása.

Több törölt oldal is kijelölhető legyen.

A visszaállított oldal ismét a munkaváltozat részévé váljon.

Ha nincs törölt oldal, a „Törölt oldalak mutatása” gomb legyen inaktív.

### 5.6. Forgatás

Az Oldalrendezőben a kijelölt oldalakat külön-külön vagy csoportosan lehessen 90 fokkal jobbra, illetve balra forgatni.

A forgatás:

- A munkaváltozatban tárolódjon.
- Legyen visszavonható.
- Azonnal látszódjon az előnézetben.
- A nyomtatásnál érvényesüljön.
- A Mentés másként művelet után megmaradjon az új PDF-ben.

A megjelenítési nézetekben végzett ideiglenes forgatás ettől független művelet.

---

## 6. A munkaváltozat kezelése – kritikus követelmény

Az eredeti PDF-et a program soha ne módosítsa automatikusan.

A dokumentum megnyitása után a program tartson fenn egy belső, ideiglenes munkaváltozatot.

Az eredeti PDF-oldalak tartalma ne másolódjon vagy renderelődjön újra minden szerkesztési műveletnél.

A munkaváltozat egy oldalhoz kapcsolódóan legalább a következőket tárolja:

- Stabil oldalazonosító.
- Eredeti oldalszám vagy forrásazonosító.
- Aktuális sorrend.
- Törölt állapot.
- Oldalforgatás.
- A visszaállításhoz szükséges pozícióinformáció.

Az oldalazonosító maradjon stabil akkor is, ha a felhasználó átrendezi vagy törli az oldalt.

A visszavonási és újra végrehajtási rendszer kezelje a törlést, visszaállítást, forgatást és átrendezést.

### Kötelező működés

A munkaváltozatot valamennyi nézet közösen használja.

Ha egy 12 oldalas dokumentumból három oldalt ideiglenesen törlünk, a normál megjelenítési módokban már csak kilenc oldal jelenjen meg.

A nyomtatás és a PDF-export szintén a munkaváltozatot használja.

Az eredeti PDF csak tudatos, külön mentési művelettel eredményezhet új, módosított PDF-állományt.

### Mentés

A dokumentummódosításokhoz alapértelmezésben kizárólag **Mentés másként…** műveletet biztosíts.

A felhasználó adja meg az új PDF nevét és helyét.

Az eredeti PDF véletlen felülírását akadályozd meg.

A mentés:

- Tartsa meg az aktív oldalak sorrendjét.
- Hagyja ki a törölt oldalakat.
- Őrizze meg az Oldalrendezőben elvégzett forgatásokat.
- Lehetőleg ne renderelje képpé a PDF oldalait.
- Őrizze meg az eredeti képek és a szövegréteg minőségét.

Az export dokumentumszerkezeti PDF-műveletekkel történjen, amikor ez támogatott. Ne készíts feleslegesen képalapú új PDF-et.

A fájlírás legyen biztonságos: hiba esetén ne maradjon érvényesnek látszó, félkész célfájl. Használj megfelelő ideiglenes fájlos és ellenőrzött lezárási megoldást.

Sikeres mentés után az új PDF automatikusan nyíljon meg egy új fülön.

Az eredeti dokumentum maradjon megnyitva.

### Bezárás

Ha a munkaváltozat nem mentett módosításokat tartalmaz, a bezárás előtt jelenjen meg:

- Mentés másként…
- Módosítások elvetése
- Mégse

Egyébként a dokumentum közvetlenül bezárható.

A fül neve mellett csillag jelezze a nem mentett dokumentummódosításokat.

---

## 7. PDF-megnyitás és keresés

### Megnyitás

Támogatott módszerek:

- Fájl → Megnyitás…
- Ctrl + O
- Drag & drop
- PDF-fájl megadása parancssori argumentummal
- Opcionális Windows-fájltársítás

Jelszóval védett PDF esetén jelenjen meg jelszókérő ablak.

A hibás vagy nem támogatott dokumentumokhoz érthető magyar nyelvű hibaüzenet tartozzon.

### Keresés

A keresőmező a főablak felső részének jobb oldalán helyezkedjen el.

A keresés:

- Ctrl + F billentyűvel aktiválható.
- A PDF meglévő szövegrétegében keressen.
- A találatokat emelje ki.
- Biztosítson előző/következő találat navigációt.
- A keresést ne végezze a fő felületi szálon, ha az hosszabb ideig tarthat.

Szkennelt, kereshető szövegréteget nem tartalmazó PDF-eknél jelenjen meg megfelelő tájékoztatás.

OCR nem része az első verziónak.

---

## 8. Nyomtatási rendszer

Készíts saját, egyszerűsített nyomtatási párbeszédablakot.

Ne a teljes, hagyományos Windows-nyomtatási felület legyen az elsődleges felület.

A nyomtatási ablak támogassa:

- Telepített nyomtatók felsorolása.
- Alapértelmezett nyomtató kiválasztása.
- Példányszám.
- Összes oldal.
- Aktuális oldal.
- Egyéni oldaltartomány.
- Oldalrendezőben kijelölt oldalak.
- Papírméret.
- Tájolás.
- Méretezés.
- Színes/fekete-fehér beállítás, ha támogatott.
- Nyomtató tulajdonságainak megnyitása.
- Nyomtatási előnézet.

A nyomtató illesztőprogramjának speciális beállításait a Windows megfelelő nyomtatókezelő API-jain keresztül tedd elérhetővé.

### Nyomtatási szabályok

A nyomtatás a munkaváltozat alapján történjen.

A törölt oldalak soha ne kerüljenek nyomtatásra, akkor sem, ha éppen láthatók az Oldalrendezőben.

Az átrendezett oldalak módosított sorrendben kerüljenek nyomtatásra.

A véglegesen elforgatott oldalak a megfelelő tájolással jelenjenek meg.

Az Oldalrendezőben kijelölt oldalak közvetlen nyomtatása legyen elérhető helyi menüből.

Ha nincs kijelölés, ne jelenjen meg aktívként a kijelölt oldalak nyomtatása művelet.

Az egyéni oldaltartomány megadása az aktuális munkaváltozat oldalszámaira vonatkozzon.

A nyomtatási előnézet csak a szükséges oldalakat renderelje.

A nyomtatás minősége ne függjön a képernyőn látható bélyegképek felbontásától.

Kezeld a nyomtatóhibákat, a megszakítást és az érvénytelen oldaltartományokat.

A nyomtatási feladat ne fagyassza le tartósan az alkalmazás felületét.

---

## 9. Eszköztár, menük és billentyűparancsok

### Általános eszköztár

A fő eszköztár elsődlegesen a következő műveleteket tartalmazza:

- PDF megnyitása.
- Nyomtatás.
- Előző oldal.
- Oldalszám kijelzése és közvetlen módosítása.
- Következő oldal.
- Nagyítás.
- Kicsinyítés.
- Oldalhoz illesztés.
- Oldalszélességhez illesztés.
- Megjelenítési forgatás.
- Nézetválasztás.
- Oldalak mentése PDF-be, másodlagos helyen.

Ne szerepeljen a fő eszköztár elején általános Mentés vagy floppyikon.

### Oldalrendező saját eszköztára

Az Oldalrendezőben jelenjenek meg az oda tartozó műveletek:

- Visszavonás.
- Újra.
- Kijelölt oldalak törlése.
- Törölt oldalak mutatása/elrejtése.
- Forgatás balra.
- Forgatás jobbra.
- Kijelöltek mentése PDF-be.
- Mentés másként…

A húzásos átrendezéshez nem szükséges külön gomb.

### Menüstruktúra

**Fájl**
- Megnyitás…
- Oldalak mentése PDF-be…
- Mentés másként…
- Nyomtatás…
- Fül bezárása
- Kilépés

**Szerkesztés**
- Visszavonás
- Úra végrehajtás
- Kijelölt oldalak törlése
- Kijelölt oldalak visszaállítása
- Összes törölt oldal visszaállítása
- Összes kijelölése

**Nézet**
- Egyoldalas
- Folyamatos
- Oldalrendező
- Nagyítás
- Kicsinyítés
- Teljes oldal
- Oldalszélességhez illesztés
- 100%
- Forgatás balra
- Forgatás jobbra
- Navigációs oldalsáv mutatása/elrejtése

**Eszközök**
- Keresés
- Beállítások…

**Súgó**
- Billentyűparancsok
- Névjegy…
- Licencek és köszönetnyilvánítás

Az egyes műveletek csak abban a nézetben legyenek aktívak, ahol értelmezhetők.

### Billentyűparancsok

- Ctrl + O: megnyitás.
- Ctrl + P: nyomtatás.
- Ctrl + F: keresés.
- Ctrl + W: fül bezárása.
- Ctrl + Shift + S: Mentés másként.
- Ctrl + Z: visszavonás.
- Ctrl + Y: újra.
- Ctrl + A: az aktuális nézet szerinti kijelölés.
- Delete: oldalak törlése kizárólag az Oldalrendezőben.
- Ctrl + egérgörgő: nagyítás, az Oldalrendezőben a bélyegképméret állítása.
- PgUp/PgDn: dokumentumnavigáció.
- Alt + F4: kilépés.

A billentyűparancsok a menükben is jelenjenek meg.

---

## 10. Állapotsor

A főablak alsó részén folyamatosan látható állapotsor legyen.

Mutassa az aktuálisan releváns információkat:

- Aktuális oldalszám.
- Munkaváltozat oldalszáma.
- Eredeti dokumentum oldalszáma, ha eltér.
- Nagyítás mértéke.
- Az aktuális oldal mérete.
- PDF fájlmérete.
- Módosított állapot.
- Ideiglenesen törölt oldalak száma.
- Aktuális művelet vagy betöltési állapot.

Az információkat kompaktan jelenítsd meg. Az állapotsor ne foglaljon indokolatlanul nagy helyet.

Az Oldalrendezőben a kijelölt oldalak száma is jelenjen meg.

---

## 11. Beállítások

A program jegyezze meg legalább a következőket:

- Ablak mérete és pozíciója.
- Legutóbb használt megjelenítési mód.
- Nagyítási preferenciák.
- Oldalrendező bélyegképmérete.
- Navigációs oldalsáv megjelenítése.
- Nyomtatási alapbeállítások, amennyiben biztonságosan alkalmazhatók.

A beállításokat lehetőleg az alkalmazás mellett elhelyezett `sPDF.ini` fájlban tárold.

A program legyen hordozható. Ne igényeljen telepítést, rendszergazdai jogosultságot vagy kötelező Windows Registry-bejegyzéseket.

Ha a program könyvtára nem írható, kezelje ezt kulturáltan, ne hibázzon végzetesen.

Ne tárold alapértelmezésben a megnyitott dokumentumok tartalmát, előnézeti képeit, keresési előzményeit vagy más érzékeny adatokat.

Ne legyen telemetria és automatikus adatküldés.

---

## 12. Ikonkészlet és vizuális erőforrások

Az alkalmazás ikonrendszeréhez a **KDE Modern Classic, 2026.10.27-i** készletet használjuk.

Referencia:
https://store.kde.org/p/2379533

A rendelkezésre bocsátott archívum neve:

`KDE-Modern-Classic-20261027.tar.gz`

Az archívumot a fejlesztés során ténylegesen használd fel, amennyiben hozzáférhető.

Ne generálj helyette hasonló stílusú, de eltérő ikonokat.

A szükséges ikonokat válogasd ki és megfelelő méretben integráld a Windows-alkalmazásba.

A fő eszköztárhoz 22–24 pixeles, kis méretben is jól olvasható ikonokat használj.

Ügyelj a vizuális következetességre:

- Egy funkció minden felületen ugyanazt az ikont használja.
- A nagyítás és kicsinyítés ikonjai egymáshoz illő párt alkossanak.
- A főablak, az Oldalrendező és a párbeszédablakok ikonvilága egységes legyen.

A teljes ikonkészletet ne csomagold be, csak a ténylegesen szükséges erőforrásokat.

### Licencelés

A KDE Modern Classic ikonkészlethez tartozó LGPL-3.0-or-later licencfeltételeket ellenőrizd.

Ellenőrizd az archívumban szereplő valamennyi releváns licencfájlt és az esetleges ellentmondásokat.

A terjesztési csomag tartalmazza az előírt licencszövegeket, forrás- és szerzői hivatkozásokat, valamint a szükséges megfelelőségi információkat.

A Névjegy ablakból a licencinformációk legyenek elérhetők.

Ha az archívum nem található meg a fejlesztési környezetben, ne helyettesítsd észrevétlenül más készlettel. Jelezd a hiányzó erőforrást, és folytasd azokat a fejlesztési feladatokat, amelyek nélküle is elvégezhetők.

---

## 13. Névjegy ablak

A program Névjegy ablakában szerepeljen:

**sPDF Megtekintő**

Az aktuális verziószám.

Leírás:

*Könnyű, gyors PDF-megtekintő Windowsra.*

Közvetlenül alatta diszkréten megjelenő, kattintható hivatkozás:

**www.edutech.hu**

Cél URL: https://www.edutech.hu

A hivatkozás az alapértelmezett böngészőben nyíljon meg, kizárólag felhasználói kattintásra.

Alatta jelenjen meg a „Licencek és köszönetnyilvánítás” hivatkozás.

---

## 14. Teljesítményoptimalizálás

Az alkalmazásnak gyenge hardveren is használhatónak kell lennie.

Fő célhardver:

- Régi x64 processzor.
- Windows 10 1607.
- HDD.
- Korlátozott RAM.
- Jellemzően 1–20 oldalas PDF-ek.
- Gyakori szkennelt dokumentumok.

Követelmények:

- Gyors indulás.
- Minimális indításkori lemezművelet.
- Dokumentumoldalak igény szerinti renderelése.
- Korlátozott méretű memóriagyorsítótár.
- Háttérben végzett renderelés.
- A felület maradjon válaszképes.
- Nagyításnál a szükségtelen renderelések megszakítása vagy elavult eredményeinek eldobása.
- A háttérben lévő dokumentumfülek ne terheljék feleslegesen a rendszert.
- Oldalrendezőben kis felbontású bélyegképek.
- Nyomtatáskor az eredeti PDF minőségének felhasználása.
- Felesleges lemezgyorsítótár elkerülése.

A PDFium objektumainak több szálról való elérését a könyvtár tényleges szálbiztonsági szabályainak megfelelően szervezd. A kezelőfelület ne blokkolódjon, de ne vezess be veszélyes párhuzamos könyvtárhívásokat.

A memóriahasználat legyen mérhető, a gyorsítótár pedig szükség esetén felszabadítható.

A gyorsaságra vonatkozó állításokat mérési eredményekkel támasszd alá, ne becslésekkel.

---

## 15. Hibakezelés

A program kezelje a következő helyzeteket:

- Sérült PDF.
- Jelszóval védett PDF.
- Nem létező fájl.
- Nem elérhető hálózati fájl.
- Érvénytelen oldaltartomány.
- Sikertelen export.
- Nem írható célkönyvtár.
- Nyomtatóhiba.
- Nyomtatás megszakítása.
- Kevés rendelkezésre álló memória.
- Fájl bezárása nem mentett módosításokkal.
- Nem elérhető vagy hibás PDFium-függőség.

A hibaüzenetek magyarul, egyértelműen fogalmazzanak.

Egy dokumentum hibája ne okozza az összes többi megnyitott dokumentum elvesztését.

---

## 16. Projektstruktúra

Hozz létre áttekinthető, moduláris projektstruktúrát.

A következő felépítés kiindulópontként szolgálhat:

```text
sPDFViewer/
  src/
    main.cpp
    MainWindow.cpp
    PdfDocument.cpp
    PdfRenderer.cpp
    PdfWorkspace.cpp
    PageOrganizer.cpp
    TabManager.cpp
    PrintManager.cpp
    PdfExport.cpp
    Settings.cpp

  include/
  resources/
  tests/
  scripts/
  .github/
    workflows/

  CMakeLists.txt
  README.md
  LICENSES/
```

A tényleges architektúrát a megvalósítás követelményei alapján alakítsd ki.

Ne szervezd az egész programot egyetlen óriási forrásfájlba.

Különítsd el a dokumentumkezelést, a munkaváltozat állapotát, a renderelést, a nyomtatást és a felhasználói felületet.

A kód legyen karbantartható, dokumentált és később bővíthető.

---

## 17. GitHub Actions és automatikus fordítás

A repositoryhoz készíts Windows x64 GitHub Actions workflow-t.

A workflow feladata:

1. Forráskód letöltése.
2. Függőségek ellenőrzése vagy előállítása.
3. CMake-konfiguráció.
4. Release típusú fordítás.
5. Automatizált tesztek futtatása.
6. Függőségek összegyűjtése.
7. Hordozható ZIP elkészítése.
8. A ZIP ellenőrzése.
9. Letölthető build artifact publikálása.

Sikertelen fordítás vagy teszt esetén a workflow ne publikáljon sikeres kiadásként hibás csomagot.

A workflow manuálisan is indítható legyen.

A hivatalos verziókiadás és a fejlesztői tesztbuild elkülönüljön.

---

## 18. Hordozható Windows-csomag

A hivatalos kiadások fájlneve verziószámozott legyen.

Példa:

`sPDF-1.0.0.zip`

A ZIP egyetlen felső szintű `sPDF` mappát tartalmazzon.

Példa:

```text
sPDF-1.0.0.zip
  sPDF/
    sPDF.exe
    pdfium.dll
    sPDF.ini
    README.txt
    licenses/
```

A pontos tartalom a tényleges függőségektől függ.

Az INI-fájlt csak akkor csomagold be alapértelmezett beállításokkal, ha szükséges; egyébként első induláskor is létrehozható.

Minden szükséges futtatási függőség kerüljön bele.

Külön telepítőre nincs szükség.

A felhasználó a ZIP kibontása után az `sPDF.exe` fájlt közvetlenül indíthassa.

### Állandó letöltési hivatkozás

A hivatalos verziószámozott ZIP mellett a legutóbbi kiadás ugyanazon tartalmú példánya jelenjen meg:

`sPDF-latest.zip`

A GitHub README elején szerepeljen:

**Legfrissebb Windows x64 verzió letöltése**

Állandó cím:

https://github.com/baloghim/sPDF/releases/latest/download/sPDF-latest.zip

A README-ben ne szerepeljen megtévesztő, nem működő letöltési link.

Ha még nincs kész Release, a README ezt egyértelműen jelezze.

A stabil kiadások mindig legyenek verziószámozottak és reprodukálhatók.

A fejlesztői build artifactok külön, könnyen megtalálható módon legyenek elérhetők a teszteléshez.

A GitHub Release automatikus létrehozása kizárólag sikeres ellenőrzések után történjen, megfelelő tag vagy kifejezett kiadási művelet hatására.

---

## 19. README és dokumentáció

A GitHub repository főoldalának README-je legyen magyar nyelvű.

Az első képernyőn szerepeljen:

- sPDF Megtekintő név.
- Rövid programleírás.
- Legfrissebb hordozható Windows-kiadás letöltési linkje.
- Minimális Windows-követelmény.
- Telepítés nélküli használat.

További fejezetek:

- Főbb funkciók.
- Képernyőképek.
- Használat.
- Oldalrendező ismertetése.
- Billentyűparancsok.
- Fordítás.
- Kiadások.
- Licencinformációk.

A README-ben ne állíts nem ellenőrzött kompatibilitást vagy teljesítményeredményt.

A képernyőképek lehetőség szerint a tényleges futó alkalmazásból készüljenek, ne mesterséges látványtervekből.

---

## 20. Automatizált tesztek

Készíts automatizált teszteket a dokumentumkezelési és szerkesztési műveletekre.

Különösen ellenőrizd:

### Dokumentumkezelés
- PDF megnyitása.
- Több dokumentum kezelése.
- Hibás fájl elutasítása.
- Oldalszámok kezelése.

### Oldalrendezés
- Egy oldal áthelyezése.
- Több oldal áthelyezése.
- Egy oldal törlése.
- Több oldal törlése.
- Utolsó megmaradt oldal törlésének megakadályozása.
- Törölt oldalak megjelenítése.
- Törölt oldalak visszaállítása.
- Visszavonás és újra.
- Forgatás és visszavonása.
- Törlés és rendezés kombinációja.

### Mentés
- Új PDF létrehozása.
- Eredeti PDF változatlansága.
- Oldalsorrend megőrzése.
- Törölt oldalak kihagyása.
- Oldalforgatások megőrzése.
- Képek és szövegréteg megőrzése.
- Új fül megnyitása mentés után.
- Sikertelen mentés kezelése.

### Nyomtatás
- Oldaltartományok értelmezése.
- Kijelölt oldalak meghatározása.
- Törölt oldalak kizárása.
- Oldalsorrend helyessége.
- Forgatás figyelembevétele.
- Nyomtatóhiba kezelése.

A nyomtatás tesztelésénél különítsd el az automatizált logikai ellenőrzéseket a tényleges nyomtatót igénylő integrációs tesztektől.

A tesztekhez ne használj bizalmas, munkahelyi vagy személyes adatot tartalmazó dokumentumokat.

---

## 21. Fejlesztési mérföldkövek

A fejlesztést egymásra épülő szakaszokban valósítsd meg.

**1. mérföldkő – Technológiai alap**

- Fordítható natív alkalmazás.
- PDFium-integráció.
- Egy PDF-oldal megjelenítése.
- Windows-kompatibilitási ellenőrzés.

**2. mérföldkő – PDF-megjelenítő**

- Megnyitás.
- Lapozás.
- Nagyítás.
- Egyoldalas nézet.
- Folyamatos nézet.

**3. mérföldkő – Főablak és dokumentumkezelés**

- Fülek.
- Menü.
- Eszköztár.
- Keresés.
- Beállítások.
- Ikonok integrálása.

**4. mérföldkő – Oldalrendező**

- Bélyegképes rács.
- Kijelölés.
- Húzásos rendezés.
- Törlés.
- Törölt oldalak mutatása.
- Visszaállítás.
- Forgatás.
- Visszavonás/újra.

**5. mérföldkő – PDF-export**

- Munkaváltozat mentése.
- Kijelölt oldalak exportja.
- Minőségmegőrzés.
- Új dokumentumfül megnyitása.

**6. mérföldkő – Nyomtatás**

- Saját nyomtatási párbeszédablak.
- Nyomtatási előnézet.
- Oldaltartományok.
- Kijelölt oldalak.
- Nyomtatóintegráció.

**7. mérföldkő – Optimalizálás és kiadás**

- Teljesítménymérés.
- Hibakezelés.
- Kompatibilitás.
- Hordozható ZIP.
- GitHub Actions.
- README.
- Release.

Minden mérföldkő végén ellenőrizd a fordíthatóságot és a már elkészült funkciók működését.

Ne jelölj késznek olyan funkciót, amely csak részben működik.

Ha egy részfeladat nehezebb a vártnál, dokumentáld a problémát, és lehetőség szerint működő, ellenőrizhető részmegoldást készíts. Ne alkalmazz félrevezető, látszólag működő helyettesítő megoldásokat.

---

## 22. Kötelező elfogadási feltételek

Az 1.0.0 verzió akkor tekinthető késznek, ha:

1. A natív Windows-alkalmazás sikeresen fordul.
2. A PDFium megbízhatóan működik.
3. Az alkalmazás nem igényel .NET-telepítést.
4. A Windows 10 1607 kompatibilitását tényleges futtatási teszt igazolja.
5. A három megjelenítési mód működik.
6. A dokumentumfülek használhatók.
7. A szövegkeresés működik a szövegréteget tartalmazó PDF-ekben.
8. Az Oldalrendező teljes alapfunkcionalitása elkészült.
9. Az eredeti dokumentumok nem módosulnak automatikusan.
10. A mentett PDF megfelelő oldalsorrendet, tartalmat és forgatásokat tartalmaz.
11. A saját nyomtatási ablak működik.
12. A törölt oldalak nyomtatáskor és mentéskor kimaradnak.
13. A program valódi KDE Modern Classic ikonokat használ.
14. A Névjegy tartalmazza a www.edutech.hu hivatkozást.
15. A licencinformációk rendelkezésre állnak.
16. Elkészül a hordozható ZIP.
17. A GitHub Actions megbízhatóan előállítja a kiadási csomagot.
18. A README-ben szerepel az aktuális letöltési lehetőség.
19. Az alapvető tesztek sikeresen lefutnak.
20. A program a célhardveren használható teljesítményt nyújt.

Ha valamelyik elfogadási feltétel nem igazolt, egyértelműen jelezd a hiányosságot.

---

## 23. A fejlesztés megkezdése és beszámolás

A munka megkezdésekor:

1. Vizsgáld meg a GitHub repository állapotát.
2. Ellenőrizd a rendelkezésre álló forrásokat és fejlesztési környezetet.
3. Ellenőrizd a PDFium, a fordítólánc és a cél-Windows kompatibilitását.
4. Ellenőrizd a KDE Modern Classic ikoncsomag elérhetőségét.
5. Állítsd fel a projekt modulszerkezetét.
6. Kezdd meg a tényleges implementációt a mérföldkövek sorrendjében.

A specifikációt tekintsd a fejlesztés alapjának, és ne változtass önkényesen a rögzített funkcionális döntéseken.

Ahol technikai részlet nincs meghatározva, válassz egyszerű, karbantartható és erőforrás-takarékos megoldást.

A fejlesztés során ténylegesen készíts kódot, teszteket, fordítási konfigurációt és automatizált buildfolyamatot. Ne állj meg pusztán egy fejlesztési terv elkészítésénél.

A mérföldkövek végén készíts rövid jelentést:

- Mely funkciók készültek el?
- Milyen tesztek futottak le?
- Milyen hibákat javítottál?
- Mi maradt nyitva?
- Mely követelményeket nem sikerült még igazolni?
- Hol érhető el a legutóbbi ténylegesen elkészült Windows-build?

**A legfontosabb végső cél: egy valóban használható, gyors, hordozható sPDF Megtekintő létrehozása, amely a régi Windows 10-es, HDD-s gépeken is kényelmesen alkalmas PDF-dokumentumok áttekintésére, oldalrendezésére és nyomtatására.**