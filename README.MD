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

Kiekvienas vartotojas turi:
- Unikalų identifikatorių / vardą.
- Viešojo rakto imitaciją (`public_key`).
- Pradinį lėšų kiekį.
- Išvesčių vektorių

Transakcija apima:
- Siuntėją.
- Gavėją.
- Perduodamą sumą.
- Transakcijos identifikatorių (`transaction_id`).
- Sekos numerį (`sequence`).

Viename įprastame bloke numatyta iki 100 transakcijų. `difficulty = 3`
reiškia, kad bloko maiša turi prasidėti trimis šešioliktainiais nuliais
(`000`).

Bloko antraštėje saugoma:
- `prev_hash` – ankstesnio bloko maišos reikšmė.
- `timestamp` – bloko sukūrimo laikas.
- `version` – bloko versija.
- `tx_root` – Merkle medžio šakninė maišos reikšmė.
- `nonce` – kasimo metu keičiamas skaičius.
- `difficulty` – kasimo sudėtingumo parametras.

Bloke taip pat saugomas jo transakcijų vektorius.

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


# Versija: `v0.2`

## Merkle medis

Kiekvieno bloko transakcijoms sudaromas Merkle medis.

Pirmiausia apskaičiuojamos atskirų transakcijų maišos reikšmės. Tuomet jos poromis jungiamos ir pakartotinai maišomos, kol gaunama viena galutinė reikšmė – Merkle šaknis (`tx_root`).

Merkle medžio lapai yra transakcijų ID, išdėstyti ta tvarka, kuria jie įtraukti į bloką.

Vidinis mazgas skaičiuojamas taip:

``` cpp
parent = H(left_child + "|" + right_child)
```

Jei lygyje lieka nelyginis skaičius mazgų, paskutinis mazgas poruojamas pats su savimi:

``` cpp
H(last_node + "|" + last_node)
```

Jei bloke yra tik viena transakcija, jos ID yra Merkle šaknis. Tuščiam Genesis blokui naudojama atskira taisyklė:

``` cpp
Genesis.tx_root = funkcija("");
```

## Kandidatinių blokų konkurencija

Realizuojama penkių kandidatinių blokų konkurencijos simuliacija.

Kiekvienam kandidatui sudaromas atskiras bloko variantas. Kandidatai bando rasti tinkamą `nonce` reikšmę ir gauti reikalavimus atitinkančią maišos reikšmę.

Sėkmingai iškastų kandidatų rezultatai palyginami. Pasirenkamas greičiausiai iškastas blokas, o nelaimėjusių kandidatų duomenys ir būsenos nėra naudojami kaip patvirtintos grandinės būsena.

## Kandidatų bandymų ir laiko suvestinė

Kandidatų kasimo rezultatai (vieno bloko pavyzdys)

| Kandidatas | Kasimo laikas | Nonce | Rezultatas |
|---|---:|---:|---|
| 1 | 1.34631 s | 5941 | Iškastas |
| 2 | 2.79008 | 12383 | Iškastas |
| 3 | 0.138947 | 606 | Iškastas |
| 4 | 4.52112 | 21424 | Iškastas |
| 5 | 1.29145 | 21424 | Iškastas |

Kasimo laikas matuojamas naudojant `std::chrono`.

## Teisingumo testų vykdymas

Atliekami bendri teisingumo testai, skirti patikrinti Merkle medžio sudarymą, transakcijų apdorojimą ir blokų validavimą.

![alt text](image.png)

## Kodėl tai nėra tikras P2P konsensusas?

Šiame projekte realizuojama blokų kasimo konkurencijos simuliacija, tačiau ji nėra lygiavertė tikram P2P blokų grandinės konsensusui.

Pagrindiniai skirtumai:

1. **Nėra P2P tinklo.** Programa veikia viename procese, todėl kandidatai nėra atskiri tinklo mazgai, bendraujantys per tinklą.

2. **Nėra nepriklausomų grandinės kopijų.** Visi kandidatai veikia bendroje programos aplinkoje, o ne palaiko atskiras, savarankiškai sinchronizuojamas blokų grandinės kopijas.

3. **Nėra tikro konsensuso protokolo.** Kandidatų rezultatai palyginami programos viduje pagal iš anksto nustatytą taisyklę. Nėra atskiro mazgų susitarimo dėl galiojančios grandinės.

4. **Nerealizuojami šakų persitvarkymai.** Jei skirtingi tinklo mazgai iškastų skirtingus blokus tame pačiame aukštyje, tikrame tinkle galėtų susidaryti skirtingos grandinės šakos. Šiame projekte toks procesas neimituojamas.

5. **Nerealizuojamas tikras Bitcoin kasimas.** Naudojamas supaprastintas Proof of Work mechanizmas, kuriame tikrinamas nustatytas maišos pradžios nulių skaičius. Nenaudojamas tikras Bitcoin tinklo sudėtingumo reguliavimas, atlygis ar kitos protokolo taisyklės.

# Versija: `v0.3`

Dirbtiniu intelektu bandiau tobulinti transakcijų atrinkimą ir kasimo ciklą, testus.

Nors patobulinus kodą viskas veikia tesisingai, sparta kaip tik sumažėjo.

Pavyzdys prieš patobulinimą:
![alt text](<Screenshot 2026-11-05 042420.png>)

Pavyzdys po patobulinimo:
![alt text](<Screenshot 2026-11-05 055002.png>)

Svarbu akcentuoti tai, jog SEED naudojimas užtikrina tos pačios skaičių sekos generavimą kiekvieno programos paleidimo metu, tačiau dėl laiko matavimo skirtumo kandidatų kasime kiekvieną kartą paleidus programą su tais pačiais duomenimis, galime gauti skirtingus blokus grandinėse.

### Dirbtinis intelektas (chatGPT, Perplexity) padėjo:
- Suprasti užduoties reikalavimus, išsiaiškinti principus
- Generuoti abstrakčias idėjas
- Padaryti v0.3 versiją
- Pildyti README failą

