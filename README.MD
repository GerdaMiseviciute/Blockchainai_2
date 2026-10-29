# Versija: `v0.1`

## Funkcionalumas

Programa:

1.  Sugeneruoja 1 000 vartotojų su viešaisiais raktais `PUB1`--`PUB1000`
    ir pradiniais balansais.
2.  Sugeneruoja 10 000 transakcijų. Kiekviena transakcija turi siuntėją,
    gavėją, sumą, eilės numerį (`sequence`) ir transakcijos ID.
3.  Sudaro pradinį „Genesis" bloką.
4.  Bando sudėti transakcijas į blokus, kurių maksimalus dydis -- 100
    transakcijų (netinkančios transakcijos iškart yra pašalinamos).
5.  Apskaičiuoja bloko `tx_root` ir kasa bloką, keisdama `nonce`, kol
    bloko maiša atitinka sudėtingumo (`difficulty`) reikalavimą.
6.  Patikrina bloko nuorodą į ankstesnį bloką, įtrauktas transakcijas.
7.  Išsaugo sugeneruotų vartotojų ir transakcijų duomenis tekstiniuose
    failuose.


Viename įprastame bloke numatyta iki 100 transakcijų. `difficulty = 3`
reiškia, kad bloko maiša turi prasidėti trimis šešioliktainiais nuliais
(`000`).

## Maišos funkcija

Naudota pirmo darbo v0.2 dirbtinio intelekto patobulinta hash funkcija, nes savarankiškai įvykdyta nebūtų galėjusi įvykdyti difficulty reikalavimų.

Bloko maiša apskaičiuojama iš šių laukų:

`prev_hash | timestamp | version | tx_root | nonce | difficulty`

## Generavimo seed ir atkuriamumas

Kode apibrėžta konstanta:

``` cpp
const uint64_t SEED = 123456789ULL;
```

Tačiau dabartinėje versijoje vartotojų vardai, balansai ir transakcijos
generuojami naudojant `rand()`, kuris inicializuojamas taip:

``` cpp
srand(static_cast<unsigned>(SEED));
```

Dėl to naudodami tuos pačius duomenis, kiekvieną kartą paleidus programą turime gauti tą patį blokų skaičių (Bloko hash gali skirtis, nes *timestamp* gali skirtis)

## Pradinės būsenos išsaugojimas

Pradinė vartotojų būsena turi būti išsaugota **po vartotojų sukūrimo,
bet prieš pritaikant bet kokias transakcijas**. Tai leidžia palyginti
balansus prieš ir po blokų grandinės apdorojimo.

Programos pradžioje sugeneruotų vartotojų duomenys šiuo metu įrašomi į
`vartotojai.txt` dar prieš pradedant transakcijų apdorojimą.

## Kompiliavimas ir paleidimas

1. Raskite **run.bat** failą (jis turi būti pagrindiniame projekto kataloge)
2. Paleiskite **run.bat** failą
- *1 variantas:* dukart spustelėkite failą
- *2 variantas:* atidarykite PowerShell arba Command Prompt, nueikite į projekto katalogą ir įveskite **.\run.bat**

Programa automatiškai:
- sukompiliuos projektą
- paleis programą
- sugeneruos rezultatus


### Sukuriami failai

Programos dabartinis kodas sukuria:

-   `vartotojai.txt` -- vartotojų vardai, pradiniai balansai ir viešieji
    raktai;
-   `transakcijos.txt` -- sugeneruotos transakcijos.

## Galutinė suvestinė

![alt text](<Screenshot 2026-10-29 063825.png>)