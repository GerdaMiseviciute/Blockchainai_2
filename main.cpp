#include <iostream>
#include <boost/multiprecision/cpp_dec_float.hpp>
#include <boost/multiprecision/cpp_int.hpp>
#include <openssl/evp.h>
#include <ctime>
#include <limits>
#include <random>
#include<array>
#include <fstream>
#include <chrono>
#include <string>
#include <vector>
#include <deque>
#include <set>
#include <map>
#include <sstream>
#include <iomanip>
#include <windows.h>
using namespace std;
using namespace chrono;
using namespace boost::multiprecision;

const uint64_t SEED = 123456789ULL;

string funkcija(const string& s)
{
    if (s.empty())
        return "Failas yra tuščias";

    cpp_int mask = (cpp_int(1) << 256) - 1;

    cpp_int hash = 0x6a09e667f3bcc908ULL;

    for (unsigned char c : s)
    {
        hash ^= c;

        hash = (hash * 0x100000001B3ULL) & mask;

        hash ^= hash >> 33;
        hash = (hash * 0xff51afd7ed558ccdULL) & mask;

        hash ^= hash << 17;
        hash &= mask;

        hash ^= hash >> 29;
    }

    hash &= mask;

    stringstream ss;
    ss << hex << setfill('0') << setw(64) << hash;

    return ss.str();
}

struct UTXO
{
    string txId;
    size_t suma;
};
struct Vartotojas
{
    string vardas;
    string public_key;
    size_t pradinis_balansas;
    vector<UTXO> isvestys;
    size_t balansas()
    {
        size_t suma=0;
        for(int i=0; i<isvestys.size(); i++)
        {
            suma+=isvestys[i].suma;
        }
        return suma;
    }
};
struct Transakcija
{
    string sender;
    string receiver;
    size_t amount;
    size_t sequence;
    string transaction_id; //=originali_funkcija(sender+receiver+std::to_string(amount)+std::to_string(sequence));
    bool operator==(const Transakcija& kita) const
    {
        return sender == kita.sender &&
               receiver == kita.receiver &&
               amount == kita.amount &&
               sequence == kita.sequence &&
               transaction_id == kita.transaction_id;
    }
};
struct Blokas
{
    string prev_hash;
    size_t timestamp;
    size_t version;
    string tx_root;
    size_t nonce=0;
    size_t difficulty=3;

    vector<Transakcija>bloko_transakcijos;

    string hash()
    {
        return funkcija(prev_hash+"|"+std::to_string(timestamp)+"|"+std::to_string(version)+"|"+tx_root+"|"+std::to_string(nonce)+"|"+std::to_string(difficulty));
    }
};
string merkle_root(vector<string>hashes)
{
    if(hashes.empty())
        return "";
    while(hashes.size()>1)
    {
        vector<string>naujas_lygis;
        for(int i=0; i<hashes.size(); i+=2)
        {
            if(i+1==hashes.size())
                naujas_lygis.push_back(funkcija(hashes[i]+"|"+hashes[i]));
            else naujas_lygis.push_back(funkcija(hashes[i]+"|"+hashes[i+1]));
        }
        hashes=naujas_lygis;
    }
    return hashes[0];
}
void transakciju_atrinkimas(Blokas& naujas_blokas, vector<Blokas>&blockchain, deque<Transakcija>&transakciju_kopija, map<string, Vartotojas>&vartotoju_kopija, vector<string>&lapai)
{
    naujas_blokas.prev_hash=blockchain[blockchain.size()-1].hash();
    naujas_blokas.version=1;
    while(naujas_blokas.bloko_transakcijos.size()<100 && !transakciju_kopija.empty())
    {
        size_t transakcijos_nr=rand()%transakciju_kopija.size();
        Transakcija t=transakciju_kopija[transakcijos_nr];
        transakciju_kopija.erase(transakciju_kopija.begin()+transakcijos_nr);
        if(vartotoju_kopija.find(t.sender)==vartotoju_kopija.end() || vartotoju_kopija.find(t.receiver)==vartotoju_kopija.end())
            continue;
        if(t.sender==t.receiver || t.amount<=0)
            continue;
        // if(vartotoju_kopija[t.sender].pradinis_balansas<t.amount)
        //     continue;
        if(vartotoju_kopija[t.sender].balansas()<t.amount)
            continue;
        lapai.push_back(t.transaction_id);
        vartotoju_kopija[t.receiver].isvestys.push_back({t.transaction_id, t.amount});
        size_t suma=t.amount;
        while(suma>0)
        {
            if(vartotoju_kopija[t.sender].isvestys.back().suma>=suma)
            {
                size_t graza=vartotoju_kopija[t.sender].isvestys.back().suma-suma;
                vartotoju_kopija[t.sender].isvestys.pop_back();
                if(graza>0)
                    vartotoju_kopija[t.sender].isvestys.push_back({"Grąža", graza});
                suma=0;
            }
            else
            {
                suma-=vartotoju_kopija[t.sender].isvestys.back().suma;
                vartotoju_kopija[t.sender].isvestys.pop_back();
            }
        }
        // vartotoju_kopija[t.sender].pradinis_balansas-=t.amount;
        // vartotoju_kopija[t.receiver].pradinis_balansas+=t.amount;
        naujas_blokas.bloko_transakcijos.push_back(t);
    }
}
void kasimas(Blokas& naujas_blokas, bool&iskastas, chrono::seconds trukme, double&visas_laikas)
{
    if(iskastas==false)
    {
        auto pradzia = chrono::steady_clock::now();
        double laikas;
        while(!naujas_blokas.hash().starts_with(std::string(naujas_blokas.difficulty,'0')))
        {
            naujas_blokas.nonce++;
            laikas = chrono::duration<double>(chrono::steady_clock::now() - pradzia).count();
            if(chrono::steady_clock::now() - pradzia>=trukme)
                break;
        }
        if(naujas_blokas.hash().starts_with(std::string(naujas_blokas.difficulty,'0')))
            iskastas=true;
        visas_laikas+=laikas;
    }
}
// Patikrina, ar tekstinis skaičius yra teigiamas sveikasis skaičius.
// Tai leidžia patikrinti ir neigiamą sumą, nors Transakcija.amount yra size_t.
bool tinkamaSuma(const string& tekstas)
{
    if (tekstas.empty())
        return false;

    for (char c : tekstas)
        if (c < '0' || c > '9')
            return false;

    try
    {
        size_t pozicija = 0;
        unsigned long long suma = stoull(tekstas, &pozicija);

        return pozicija == tekstas.size() &&
               suma > 0 &&
               suma <= numeric_limits<size_t>::max();
    }
    catch (...)
    {
        return false;
    }
}
// Apskaičiuoja visų vartotojų turimų UTXO sumą.
size_t visuLesuSuma(const map<string, Vartotojas>& vartotojai)
{
    size_t suma = 0;

    for (const auto& [key, vartotojas] : vartotojai)
    {
        for (const UTXO& u : vartotojas.isvestys)
            suma += u.suma;
    }

    return suma;
}
// Pritaiko vieną transakciją UTXO modeliui.
// Jeigu transakcija negalioja, pradinė būsena nepakeičiama.
bool vykdytiTransakcija(
    const Transakcija& t,
    map<string, Vartotojas>& vartotojai)
{
    auto siuntejas = vartotojai.find(t.sender);
    auto gavejas = vartotojai.find(t.receiver);

    // Turi egzistuoti abu dalyviai.
    if (siuntejas == vartotojai.end() ||
        gavejas == vartotojai.end())
        return false;

    // Negalima siųsti sau arba siųsti nulinės sumos.
    if (t.sender == t.receiver || t.amount == 0)
        return false;

    // Patikrinamas transakcijos ID.
    string teisingas_id = funkcija(
        t.sender + "|" +
        t.receiver + "|" +
        to_string(t.amount) + "|" +
        to_string(t.sequence)
    );

    if (t.transaction_id != teisingas_id)
        return false;

    // Patikrinamas gavėjo balanso perpildymas.
    size_t gavejo_balansas = gavejas->second.balansas();

    if (gavejo_balansas > SIZE_MAX - t.amount)
        return false;

    // Patikrinama, ar siuntėjas turi pakankamai lėšų.
    size_t siuntejo_balansas = siuntejas->second.balansas();

    if (siuntejo_balansas < t.amount)
        return false;

    // Dirbama su kopija, kad nesėkmės atveju būsena nepasikeistų.
    map<string, Vartotojas> kopija = vartotojai;

    auto s = kopija.find(t.sender);
    auto g = kopija.find(t.receiver);

    // Gavėjui sukuriama nauja išvestis.
    g->second.isvestys.push_back({t.transaction_id, t.amount});

    // Sunaudojamos siuntėjo išvestys.
    size_t likusi_suma = t.amount;

    while (likusi_suma > 0)
    {
        if (s->second.isvestys.empty())
            return false;

        size_t isvesties_suma = s->second.isvestys.back().suma;

        if (isvesties_suma >= likusi_suma)
        {
            size_t graza = isvesties_suma - likusi_suma;

            s->second.isvestys.pop_back();

            if (graza > 0)
                s->second.isvestys.push_back({"Grąža", graza});

            likusi_suma = 0;
        }
        else
        {
            likusi_suma -= isvesties_suma;
            s->second.isvestys.pop_back();
        }
    }

    vartotojai = kopija;
    return true;
}
// Patikrina visą bloką.
// Būsena pakeičiama tik tada, jeigu visas blokas galioja.
bool tikrintiBloka(
    const Blokas& blokas,
    const map<string, Vartotojas>& pradine_busena,
    const string& ankstesnio_bloko_hash,
    const set<string>& patvirtinti_id,
    map<string, Vartotojas>& galutine_busena)
{
    Blokas kopija_bloko = blokas;

    // Ankstesnio bloko patikra.
    if (blokas.prev_hash != ankstesnio_bloko_hash)
        return false;

    // Formato versijos ir laiko patikra.
    if (blokas.version != 1 || blokas.timestamp == 0)
        return false;

    // Transakcijų skaičiaus patikra.
    if (blokas.bloko_transakcijos.empty() ||
        blokas.bloko_transakcijos.size() > 100)
        return false;

    // Proof of Work patikra.
    if (blokas.difficulty == 0 ||
        blokas.difficulty > 64)
        return false;

    if (!kopija_bloko.hash().starts_with(
            string(blokas.difficulty, '0')))
        return false;

    // Tikrinama, ar bloke nėra pasikartojančių ID.
    set<string> bloke_esantys_id;

    for (const Transakcija& t : blokas.bloko_transakcijos)
    {
        if (!bloke_esantys_id.insert(t.transaction_id).second)
            return false;

        // Tikrinama, ar ID jau nėra patvirtintoje grandinėje.
        if (patvirtinti_id.count(t.transaction_id) > 0)
            return false;
    }

    // Kiekviena transakcija vykdoma eilės tvarka.
    map<string, Vartotojas> laikina_busena = pradine_busena;

    for (const Transakcija& t : blokas.bloko_transakcijos)
    {
        if (!vykdytiTransakcija(t, laikina_busena))
            return false;
    }

    // Tikrinama Merkle šaknis.
    vector<string> lapai;

    for (const Transakcija& t : blokas.bloko_transakcijos)
        lapai.push_back(t.transaction_id);

    if (blokas.tx_root != merkle_root(lapai))
        return false;

    // Tikrinama, ar Merkle šaknis yra tinkamo formato.
    if (blokas.tx_root.size() != 64 ||
        !all_of(blokas.tx_root.begin(), blokas.tx_root.end(),
            [](char c)
            {
                return (c >= '0' && c <= '9') ||
                       (c >= 'a' && c <= 'f');
            }))
        return false;

    // Tik jeigu visos patikros sėkmingos, perduodama nauja būsena.
    galutine_busena = laikina_busena;

    return true;
}
// Sukuria testinę transakciją su teisingu ID.
Transakcija sukurtiTestineTransakcija(
    string siuntejas,
    string gavejas,
    size_t suma,
    size_t sequence)
{
    string id = funkcija(
        siuntejas + "|" +
        gavejas + "|" +
        to_string(suma) + "|" +
        to_string(sequence)
    );

    return {siuntejas, gavejas, suma, sequence, id};
}
// Sukuria du vartotojus su nurodytais pradiniais balansais.
map<string, Vartotojas> sukurtiTestiniusVartotojus()
{
    map<string, Vartotojas> vartotojai;

    vartotojai["A"] = {"Jonas", "A", 100};
    vartotojai["B"] = {"Petras", "B", 50};
    vartotojai["C"] = {"Ona", "C", 20};

    vartotojai["A"].isvestys.push_back({"Genesis", 100});
    vartotojai["B"].isvestys.push_back({"Genesis", 50});
    vartotojai["C"].isvestys.push_back({"Genesis", 20});

    return vartotojai;
}
// Suranda nonce, atitinkantį Proof of Work reikalavimą.
bool testinisKasimas(Blokas& blokas)
{
    blokas.nonce = 0;

    while (blokas.nonce < 10000000)
    {
        if (blokas.hash().starts_with(
                string(blokas.difficulty, '0')))
            return true;

        blokas.nonce++;
    }

    return false;
}
// Sukuria testinį bloką.
Blokas sukurtiTestiniBloka(
    const vector<Transakcija>& transakcijos,
    const string& prev_hash)
{
    Blokas blokas;

    blokas.prev_hash = prev_hash;
    blokas.version = 1;
    blokas.timestamp = 1000;
    blokas.difficulty = 1;

    blokas.bloko_transakcijos = transakcijos;

    vector<string> lapai;

    for (const Transakcija& t : transakcijos)
        lapai.push_back(t.transaction_id);

    blokas.tx_root = merkle_root(lapai);

    testinisKasimas(blokas);

    return blokas;
}
// Išveda testo rezultatą.
void testoRezultatas(const string& pavadinimas, bool sekmingas)
{
    cout << (sekmingas ? "[PASS] " : "[FAIL] ")
         << pavadinimas << endl;
}
int main()
{
    srand(static_cast<unsigned>(SEED));
    SetConsoleCP(CP_UTF8);        // konsolės įvestis → UTF-8
    SetConsoleOutputCP(CP_UTF8);
    int choice;
    cout<<"===Meniu==="<<endl;
    cout<<"1. Generuoti vartotojus ir transakcijas"<<endl;
    cout<<"2. Kasti blokus"<<endl;
    cout<<"3. Vykdyti teisingumo testus"<<endl;
    while(true)
    {
        try
        {
            cin>>choice;
            if(cin.fail() || choice<1 || choice>3)
                throw std::runtime_error("Iveskite skaiciu 1-3: ");

            cin.ignore(1000, '\n');
            break;
        }
        catch(const std::exception& e)
        {
            cerr<<"Klaida! "<<e.what()<<endl;
            cin.clear();
            cin.ignore(1000, '\n');
        }
    }
    switch(choice)
    {
        case 1:
        {
            //  Vartotojų kūrimas
            ofstream fr("vartotojai.txt");
            map<string, Vartotojas>vartotojai;
            fr<<left<<setw(25)<<"Vardas"<<setw(20)<<"Balansas"<<setw(20)<<"Viešas raktas"<<endl;
            for(size_t i=0; i<1000; i++)
            {
                string vardas;
                int x=rand()%10+1;
                switch (x)
                {
                    case 1:
                    {
                        vardas="Jonas";
                        break;
                    }
                    case 2:
                    {
                        vardas="Petras";
                        break;
                    }
                    case 3:
                    {
                        vardas="Ona";
                        break;
                    }
                    case 4:
                    {
                        vardas="Elija";
                        break;
                    }
                    case 5:
                    {
                        vardas="Gabrielius";
                        break;
                    }
                    case 6:
                    {
                        vardas="Patricija";
                        break;
                    }
                    case 7:
                    {
                        vardas="Tomas";
                        break;
                    }
                    case 8:
                    {
                        vardas="Emilija";
                        break;
                    }
                    case 9:
                    {
                        vardas="Aurimas";
                        break;
                    }
                    case 10:
                    {
                        vardas="Lina";
                        break;
                    }
                }
                size_t balansas=rand()%(1000000-100+1)+100;
                vartotojai["PUB"+std::to_string(i+1)]={vardas, "PUB"+std::to_string(i+1), balansas};
                fr<<left<<setw(25)<<vardas<<setw(20)<<balansas<<setw(20)<<"PUB"+std::to_string(i+1)<<endl;
            }
            fr.close();
            //Transakciju kurimas
            deque<Transakcija>transakcijos;
            ofstream fr1("transakcijos.txt");
            fr1<<left<<setw(25)<<"Siuntėjas"<<setw(20)<<"Gavėjas"<<setw(20)<<"Suma"<<setw(10)<<"Sequence"<<setw(20)<<"Transaction_id"<<endl;
            for(size_t i=0; i<10000; i++)
            {
                string sender, receiver;
                do
                {
                    sender="PUB"+std::to_string(rand()%1000+1);
                    receiver="PUB"+std::to_string(rand()%1000+1);
                } while (sender==receiver);
                size_t amount=rand()%vartotojai[sender].pradinis_balansas+1;
                string transakcijos_id=funkcija(sender+"|"+receiver+"|"+std::to_string(amount)+"|"+std::to_string(i+1));
                transakcijos.push_back({sender, receiver, amount, i+1, transakcijos_id});
                fr1<<left<<setw(25)<<sender<<setw(20)<<receiver<<setw(20)<<amount<<setw(10)<<i+1<<setw(20)<<transakcijos_id<<endl;
            }
            fr1.close();
            break;
        }
        case 2:
        {
            ifstream fd("vartotojai.txt");
            ifstream fd1("transakcijos.txt");
            size_t pradine_balansu_suma=0, galutine_balansu_suma=0;
            deque<Transakcija>transakcijos;
            map<string, Vartotojas>vartotojai;
            string eil;
            getline(fd, eil);
            while(getline(fd, eil))
            {
                if (eil.empty()) 
                    continue;
                istringstream ss(eil);
                string vardas, key;
                size_t balansas;
                ss>>vardas>>balansas>>key;
                pradine_balansu_suma+=balansas;
                vartotojai[key]={vardas, key, balansas};
                vartotojai[key].isvestys.push_back({"Genesis", balansas});
            }
            fd.close();

            getline(fd1, eil);
            while(getline(fd1, eil))
            {
                if (eil.empty()) 
                    continue;
                istringstream ss1(eil);
                string siunt, gav, transaction_id;
                size_t suma, seq;
                ss1>>siunt>>gav>>suma>>seq>>transaction_id;
                transakcijos.push_back({siunt, gav, suma, seq, transaction_id});
            }
            fd1.close();
            size_t is_viso_transakciju=transakcijos.size();
            //Blokų sudarymas
            
            auto start1 = high_resolution_clock::now();
            vector<Blokas>blockchain;
            Blokas Genesis;
            Genesis.prev_hash="0000000000000000000000000000000000000000000000000000000000000000";
            Genesis.tx_root=funkcija("");
            Genesis.nonce=0;
            Genesis.version=1;
            Genesis.timestamp=chrono::duration_cast<chrono::seconds>(chrono::system_clock::now().time_since_epoch()).count();;
            blockchain.push_back(Genesis);

            cout<<"Genesis blokas"<<endl;
            cout<<"Aukštis: "<<blockchain.size()<<endl;
            cout<<"Transakcijų skaičius: 0"<<endl;;
            cout<<"prev_hash: "<<Genesis.prev_hash<<endl;
            cout<<"tx_root: "<<Genesis.tx_root<<endl;
            cout<<"Bloko hash: "<<Genesis.hash()<<endl;
            cout<<"Difficulty: "<<Genesis.difficulty<<endl;
            cout<<"Nonce: "<<Genesis.nonce<<endl; 
            cout<<"Bandymų skaičius: 0"<<endl<<endl;
            size_t itrauktos_transakcijos=0;
            while(!transakcijos.empty())
            {
                deque<Transakcija>transakciju_kopija1=transakcijos;
                deque<Transakcija>transakciju_kopija2=transakcijos;
                deque<Transakcija>transakciju_kopija3=transakcijos;
                deque<Transakcija>transakciju_kopija4=transakcijos;
                deque<Transakcija>transakciju_kopija5=transakcijos;
                map<string, Vartotojas>vartotoju_kopija1=vartotojai;
                map<string, Vartotojas>vartotoju_kopija2=vartotojai;
                map<string, Vartotojas>vartotoju_kopija3=vartotojai;
                map<string, Vartotojas>vartotoju_kopija4=vartotojai;
                map<string, Vartotojas>vartotoju_kopija5=vartotojai;
                Blokas naujas_blokas1;
                Blokas naujas_blokas2;
                Blokas naujas_blokas3;
                Blokas naujas_blokas4;
                Blokas naujas_blokas5;
                vector<string>lapai1;
                vector<string>lapai2;
                vector<string>lapai3;
                vector<string>lapai4;
                vector<string>lapai5;
                auto start = high_resolution_clock::now();
                // naujas_blokas.prev_hash=blockchain[blockchain.size()-1].hash();
                // naujas_blokas.version=1;
                // while(naujas_blokas1.bloko_transakcijos.size()<100 && !transakciju_kopija.empty())
                // {
                //     size_t transakcijos_nr=rand()%transakciju_kopija.size();
                //     Transakcija t=transakciju_kopija[transakcijos_nr];
                //     transakciju_kopija.erase(transakciju_kopija.begin()+transakcijos_nr);
                //     if(vartotoju_kopija.find(t.sender)==vartotoju_kopija.end() || vartotoju_kopija.find(t.receiver)==vartotoju_kopija.end())
                //         continue;
                //     if(t.sender==t.receiver || t.amount<=0)
                //         continue;
                //     // if(vartotoju_kopija[t.sender].pradinis_balansas<t.amount)
                //     //     continue;
                //     if(vartotoju_kopija[t.sender].balansas()<t.amount)
                //         continue;
                //     lapai.push_back(t.transaction_id);
                //     vartotoju_kopija[t.receiver].isvestys.push_back({t.transaction_id, t.amount});
                //     size_t suma=t.amount;
                //     while(suma>0)
                //     {
                //         if(vartotoju_kopija[t.sender].isvestys.back().suma>=suma)
                //         {
                //             size_t graza=vartotoju_kopija[t.sender].isvestys.back().suma-suma;
                //             vartotoju_kopija[t.sender].isvestys.pop_back();
                //             if(graza>0)
                //                 vartotoju_kopija[t.sender].isvestys.push_back({"Grąža", graza});
                //             suma=0;
                //         }
                //         else
                //         {
                //             suma-=vartotoju_kopija[t.sender].isvestys.back().suma;
                //             vartotoju_kopija[t.sender].isvestys.pop_back();
                //         }
                //     }
                //     // vartotoju_kopija[t.sender].pradinis_balansas-=t.amount;
                //     // vartotoju_kopija[t.receiver].pradinis_balansas+=t.amount;
                //     naujas_blokas.bloko_transakcijos.push_back(t);
                // }    
                
                do
                {
                    transakciju_atrinkimas(naujas_blokas1, blockchain, transakciju_kopija1, vartotoju_kopija1, lapai1);
                    transakciju_atrinkimas(naujas_blokas2, blockchain, transakciju_kopija2, vartotoju_kopija2, lapai2);
                    transakciju_atrinkimas(naujas_blokas3, blockchain, transakciju_kopija3, vartotoju_kopija3, lapai3);
                    transakciju_atrinkimas(naujas_blokas4, blockchain, transakciju_kopija4, vartotoju_kopija4, lapai4);
                    transakciju_atrinkimas(naujas_blokas5, blockchain, transakciju_kopija5, vartotoju_kopija5, lapai5);
                }while(naujas_blokas1.bloko_transakcijos==naujas_blokas2.bloko_transakcijos ||
                        naujas_blokas1.bloko_transakcijos==naujas_blokas3.bloko_transakcijos ||
                        naujas_blokas1.bloko_transakcijos==naujas_blokas4.bloko_transakcijos ||
                        naujas_blokas1.bloko_transakcijos==naujas_blokas5.bloko_transakcijos ||
                        naujas_blokas2.bloko_transakcijos==naujas_blokas3.bloko_transakcijos ||
                        naujas_blokas2.bloko_transakcijos==naujas_blokas4.bloko_transakcijos ||
                        naujas_blokas2.bloko_transakcijos==naujas_blokas5.bloko_transakcijos ||
                        naujas_blokas3.bloko_transakcijos==naujas_blokas4.bloko_transakcijos ||
                        naujas_blokas3.bloko_transakcijos==naujas_blokas5.bloko_transakcijos ||
                        naujas_blokas4.bloko_transakcijos==naujas_blokas5.bloko_transakcijos);

                // string visu_transakciju_id;
                // for(int i=0; i<naujas_blokas.bloko_transakcijos.size(); i++)
                // {
                //     if(i>0)
                //         visu_transakciju_id+="|";
                //     visu_transakciju_id+=naujas_blokas.bloko_transakcijos[i].transaction_id;
                // }
                // naujas_blokas.tx_root=funkcija(visu_transakciju_id);
                // cout<<merkle_root(lapai)<<endl;
                naujas_blokas1.tx_root=merkle_root(lapai1);
                naujas_blokas2.tx_root=merkle_root(lapai2);
                naujas_blokas3.tx_root=merkle_root(lapai3);
                naujas_blokas4.tx_root=merkle_root(lapai4);
                naujas_blokas5.tx_root=merkle_root(lapai5);
                naujas_blokas1.timestamp=chrono::duration_cast<chrono::seconds>(chrono::system_clock::now().time_since_epoch()).count();
                naujas_blokas2.timestamp=chrono::duration_cast<chrono::seconds>(chrono::system_clock::now().time_since_epoch()).count();
                naujas_blokas3.timestamp=chrono::duration_cast<chrono::seconds>(chrono::system_clock::now().time_since_epoch()).count();
                naujas_blokas4.timestamp=chrono::duration_cast<chrono::seconds>(chrono::system_clock::now().time_since_epoch()).count();
                naujas_blokas5.timestamp=chrono::duration_cast<chrono::seconds>(chrono::system_clock::now().time_since_epoch()).count();
                
                bool iskastas1=false, iskastas2=false, iskastas3=false, iskastas4=false, iskastas5=false;
                auto trukme=chrono::seconds(0);
                double laikas1=0, laikas2=0, laikas3=0, laikas4=0, laikas5=0;
                do
                {
                    kasimas(naujas_blokas1, iskastas1, chrono::seconds(5), laikas1);
                    kasimas(naujas_blokas2, iskastas2, chrono::seconds(5), laikas2);
                    kasimas(naujas_blokas3, iskastas3, chrono::seconds(5), laikas3);
                    kasimas(naujas_blokas4, iskastas4, chrono::seconds(5), laikas4);
                    kasimas(naujas_blokas5, iskastas5, chrono::seconds(5), laikas5);
                    trukme+=chrono::seconds(5);
                } while((iskastas5==false || iskastas4==false || iskastas3==false || iskastas2==false || iskastas1==false) && trukme<chrono::seconds(100));
                cout<<laikas1<<" "<<naujas_blokas1.nonce<<endl;
                cout<<laikas2<<" "<<naujas_blokas2.nonce<<endl;
                cout<<laikas3<<" "<<naujas_blokas3.nonce<<endl;
                cout<<laikas4<<" "<<naujas_blokas4.nonce<<endl;
                cout<<laikas5<<" "<<naujas_blokas5.nonce<<endl;
                Blokas naujas_blokas;
                deque<Transakcija>transakciju_kopija_originali;
                map<string, Vartotojas>vartotoju_kopija_originali;
                if(laikas1<laikas2 && laikas1<laikas3 && laikas1<laikas4 && laikas1<laikas5)
                {
                    naujas_blokas=naujas_blokas1;
                    transakciju_kopija_originali=transakciju_kopija1;
                    vartotoju_kopija_originali=vartotoju_kopija1;
                }
                else if(laikas2<laikas1 && laikas2<laikas3 && laikas2<laikas4 && laikas2<laikas5)
                {
                    naujas_blokas=naujas_blokas2;
                    transakciju_kopija_originali=transakciju_kopija2;
                    vartotoju_kopija_originali=vartotoju_kopija2;
                }
                else if(laikas3<laikas1 && laikas3<laikas2 && laikas3<laikas4 && laikas3<laikas5)
                {
                    naujas_blokas=naujas_blokas3;
                    transakciju_kopija_originali=transakciju_kopija3;
                    vartotoju_kopija_originali=vartotoju_kopija3;
                }
                else if(laikas4<laikas1 && laikas4<laikas2 && laikas4<laikas3 && laikas2<laikas5)
                {
                    naujas_blokas=naujas_blokas4;
                    transakciju_kopija_originali=transakciju_kopija4;
                    vartotoju_kopija_originali=vartotoju_kopija4;

                }
                else 
                {
                    naujas_blokas=naujas_blokas5;
                    transakciju_kopija_originali=transakciju_kopija5;
                    vartotoju_kopija_originali=vartotoju_kopija5;
                }
                
                // naujas_blokas.nonce=0;
                // size_t bandymai=0;
                // while(!naujas_blokas.hash().starts_with(std::string(naujas_blokas.difficulty,'0')))
                // {
                //     naujas_blokas.nonce++;
                //     bandymai++;
                // }
                auto end = high_resolution_clock::now();
                duration<double> laikas=end-start;

                //patikrinimas
                vector<string>kopijos_lapai;
                if(naujas_blokas.prev_hash==blockchain[blockchain.size()-1].hash() && naujas_blokas.hash().starts_with(std::string(naujas_blokas.difficulty,'0')) && naujas_blokas.bloko_transakcijos.size()<=100)
                {
                    deque<Transakcija>transakciju_kopija=transakcijos;
                    map<string, Vartotojas>vartotoju_kopija=vartotojai;
                    bool galioja = true;
                    for(int i=0; i<naujas_blokas.bloko_transakcijos.size(); i++)
                    {
                        const Transakcija t=naujas_blokas.bloko_transakcijos[i];
                        auto siuntejas = vartotoju_kopija.find(t.sender);
                        auto gavejas = vartotoju_kopija.find(t.receiver);
                        if(count_if(naujas_blokas.bloko_transakcijos.begin(), naujas_blokas.bloko_transakcijos.end(), [&t](const Transakcija& l) { return l.transaction_id==t.transaction_id; })!=1)
                        {
                            galioja = false;
                            break;
                        }
                        if(t.transaction_id.size()!=64 && all_of(t.transaction_id.begin(), t.transaction_id.end(), [](char c) { return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'); })==0)
                        {
                            galioja = false;
                            break;
                        }
                        if (siuntejas == vartotoju_kopija.end() || gavejas == vartotoju_kopija.end())
                        {
                            galioja = false;
                            break;
                        }
                        if (t.sender == t.receiver || t.amount == 0)
                        {
                            galioja = false;
                            break;
                        }
                        if(siuntejas->second.balansas()<t.amount && siuntejas->second.balansas()+t.amount<=SIZE_MAX && naujas_blokas.version==1 && naujas_blokas.timestamp>=0)
                        {
                            galioja=false;
                            break;
                        }
                        // if (siuntejas->second.pradinis_balansas < t.amount)
                        // {
                        //     galioja = false;
                        //     break;
                        // }
                        gavejas->second.isvestys.push_back({t.transaction_id, t.amount});
                        size_t suma=t.amount;
                        while(suma>0)
                        {
                            if(siuntejas->second.isvestys.back().suma>=suma)
                            {
                                size_t graza=siuntejas->second.isvestys.back().suma-suma;
                                siuntejas->second.isvestys.pop_back();
                                if(graza>0)
                                    siuntejas->second.isvestys.push_back({"Grąža", graza});
                                suma=0;
                            }
                            else
                            {
                                suma-=siuntejas->second.isvestys.back().suma;
                                siuntejas->second.isvestys.pop_back();
                            }
                        }
                        // siuntejas->second.pradinis_balansas -= t.amount;
                        // gavejas->second.pradinis_balansas += t.amount;
                        
                        // if(i>0)
                        //     visu_transakciju_id1+="|";
                        // visu_transakciju_id1+=t.transaction_id;
                        kopijos_lapai.push_back(t.transaction_id);
                    }
                    if(galioja && naujas_blokas.tx_root==merkle_root(kopijos_lapai) && all_of(naujas_blokas.tx_root.begin(), naujas_blokas.tx_root.end(), [](char c) { return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'); })==1)
                    {
                        itrauktos_transakcijos+=naujas_blokas.bloko_transakcijos.size();
                        blockchain.push_back(naujas_blokas);
                        transakcijos=transakciju_kopija_originali;//kazkuris tai 1, 2, 3, 4, 5
                        vartotojai=vartotoju_kopija_originali;//irgi kazkuris specifinis
                        cout<<"Priimtas naujas blokas!"<<endl;
                        cout<<"Aukštis: "<<blockchain.size()<<endl;
                        cout<<"Transakcijų skaičius: "<<naujas_blokas.bloko_transakcijos.size()<<endl;;
                        cout<<"prev_hash: "<<naujas_blokas.prev_hash<<endl;
                        cout<<"tx_root: "<<naujas_blokas.tx_root<<endl;
                        cout<<"Bloko hash: "<<naujas_blokas.hash()<<endl;
                        cout<<"Difficulty: "<<naujas_blokas.difficulty<<endl;
                        cout<<"Nonce: "<<naujas_blokas.nonce<<endl;
                        cout<<"Bandymų skaičius: "<<naujas_blokas.nonce<<endl;
                        cout<<"Laikas: "<<laikas.count()<<endl<<endl;
                    }
                }
            }
            
            auto end1 = high_resolution_clock::now();
            duration<double> pilnas_laikas=end1-start1;
            cout<<"Vartotojų: "<<vartotojai.size()<<endl;
            cout<<"Transakcijų: "<<is_viso_transakciju<<endl;
            cout<<"Įtrauktos transakcijos: "<<itrauktos_transakcijos<<endl;
            cout<<"Neįtrauktos transakcijos: "<<is_viso_transakciju-itrauktos_transakcijos<<endl;
            cout<<"Į blokų grandinę itraukta "<<blockchain.size()<<" blokai"<<endl;
            cout<<"Pradinė lėšų suma: "<<pradine_balansu_suma<<endl;
            for(int i=0; i<vartotojai.size(); i++)
                galutine_balansu_suma+=vartotojai["PUB"+std::to_string(i+1)].balansas();
            // for(int i=0; i<vartotojai.size(); i++)
            //     galutine_balansu_suma+=vartotojai["PUB"+std::to_string(i+1)].pradinis_balansas;
            cout<<"Galutinė lėšų suma: "<<galutine_balansu_suma<<endl;
            cout<<"Bendras laikas: "<<pilnas_laikas.count()<<endl;
            break;
        }
        case 3:
        {
            cout << "\n========== TEISINGUMO TESTAI ==========\n\n";

            const string prev_hash = funkcija("ankstesnis blokas");

            // ==========================================
            // 1. GALIOJANTIS MOKEJIMAS
            // ==========================================

            {
                auto vartotojai = sukurtiTestiniusVartotojus();
                auto pradine_busena = vartotojai;

                size_t pradzios_suma = visuLesuSuma(vartotojai);

                Transakcija t = sukurtiTestineTransakcija("A", "B", 30, 1);

                Blokas blokas = sukurtiTestiniBloka({t}, prev_hash);

                map<string, Vartotojas> galutine_busena;
                set<string> patvirtinti_id;

                bool galioja = tikrintiBloka(
                    blokas,
                    vartotojai,
                    prev_hash,
                    patvirtinti_id,
                    galutine_busena
                );

                bool teisingi_balansai =
                    galioja &&
                    galutine_busena["A"].balansas() == 70 &&
                    galutine_busena["B"].balansas() == 80;

                bool nekinta_lesu_suma =
                    galioja &&
                    visuLesuSuma(galutine_busena) == pradzios_suma;

                bool vienas_itraukimas =
                    galioja &&
                    blokas.bloko_transakcijos.size() == 1;

                testoRezultatas(
                    "Galiojantis mokėjimas, teisingi balansai, nekintanti lėšų suma",
                    teisingi_balansai && nekinta_lesu_suma && vienas_itraukimas
                );
            }


            // ==========================================
            // 2. NEZINOMAS GAVEJAS, NULINE / NEIGIAMA SUMA,
            //    PAKEISTAS ID
            // ==========================================

            {
                auto pradine_busena = sukurtiTestiniusVartotojus();

                auto vartotojai = pradine_busena;

                Transakcija t1 = sukurtiTestineTransakcija("A", "NEZINOMAS", 10, 1);
                Transakcija t2 = sukurtiTestineTransakcija("A", "B", 0, 2);
                Transakcija t3 = sukurtiTestineTransakcija("A", "B", 10, 3);

                Transakcija t4 = sukurtiTestineTransakcija("A", "B", 10, 4);
                t4.transaction_id = "pakeistas_ID";

                bool nezinomas_gavejas = !vykdytiTransakcija(t1, vartotojai);
                bool nuline_suma = !vykdytiTransakcija(t2, vartotojai);

                // size_t negali saugoti neigiamo skaičiaus.
                // Todėl neigiamą sumą tikriname prieš konvertuojant į size_t.
                bool neigiama_suma = !tinkamaSuma("-10");

                bool pakeistas_id = !vykdytiTransakcija(t4, vartotojai);

                bool busena_nepasikeite =
                    visuLesuSuma(vartotojai) == visuLesuSuma(pradine_busena) &&
                    vartotojai["A"].balansas() == pradine_busena["A"].balansas() &&
                    vartotojai["B"].balansas() == pradine_busena["B"].balansas();

                testoRezultatas(
                    "Nežinomas gavėjas, nulinė / neigiama suma, pakeistas ID",
                    nezinomas_gavejas &&
                    nuline_suma &&
                    neigiama_suma &&
                    pakeistas_id &&
                    busena_nepasikeite
                );
            }


            // ==========================================
            // 3. DU TO PATIES SIUNTEJO MOKEJIMAI VIRSIJA BALANSA
            // ==========================================

            {
                auto pradine_busena = sukurtiTestiniusVartotojus();

                Transakcija t1 = sukurtiTestineTransakcija("A", "B", 70, 1);
                Transakcija t2 = sukurtiTestineTransakcija("A", "C", 60, 2);

                auto laikina_busena = pradine_busena;

                bool pirmas_pavyko = vykdytiTransakcija(t1, laikina_busena);
                bool antras_atmestas = !vykdytiTransakcija(t2, laikina_busena);

                // Pirmas mokėjimas palieka A tik 30.
                bool laikomasi_eiles =
                    laikina_busena["A"].balansas() == 30 &&
                    laikina_busena["B"].balansas() == 120 &&
                    laikina_busena["C"].balansas() == 20;

                // Visas blokas turi būti atmestas, nes antroji transakcija negalioja.
                Blokas blokas = sukurtiTestiniBloka({t1, t2}, prev_hash);

                map<string, Vartotojas> galutine_busena;
                set<string> patvirtinti_id;

                bool blokas_atmestas = !tikrintiBloka(
                    blokas,
                    pradine_busena,
                    prev_hash,
                    patvirtinti_id,
                    galutine_busena
                );

                // Atmetus bloką, pradinė būsena lieka nepakeista.
                bool busena_nepasikeite =
                    pradine_busena["A"].balansas() == 100 &&
                    pradine_busena["B"].balansas() == 50 &&
                    pradine_busena["C"].balansas() == 20;

                testoRezultatas(
                    "Du mokėjimai viršija siuntėjo balansą, laikomasi vykdymo tvarkos",
                    pirmas_pavyko &&
                    antras_atmestas &&
                    laikomasi_eiles &&
                    blokas_atmestas &&
                    busena_nepasikeite
                );
            }


            // ==========================================
            // 4. DUBLIUOTAS ID IR JAU PATVIRTINTA TRANSAKCIJA
            // ==========================================

            {
                auto vartotojai = sukurtiTestiniusVartotojus();

                Transakcija t = sukurtiTestineTransakcija("A", "B", 10, 1);

                // Tas pats ID du kartus tame pačiame bloke.
                Blokas dubliuotas_blokas = sukurtiTestiniBloka(
                    {t, t},
                    prev_hash
                );

                map<string, Vartotojas> galutine_busena;
                set<string> patvirtinti_id;

                bool dubliuotas_atmestas = !tikrintiBloka(
                    dubliuotas_blokas,
                    vartotojai,
                    prev_hash,
                    patvirtinti_id,
                    galutine_busena
                );

                // ID jau yra patvirtintų transakcijų rinkinyje.
                patvirtinti_id.insert(t.transaction_id);

                Blokas pakartotas_blokas = sukurtiTestiniBloka(
                    {t},
                    prev_hash
                );

                bool pakartotas_atmestas = !tikrintiBloka(
                    pakartotas_blokas,
                    vartotojai,
                    prev_hash,
                    patvirtinti_id,
                    galutine_busena
                );

                testoRezultatas(
                    "Dubliuotas ID ir jau patvirtintos transakcijos pakartojimas",
                    dubliuotas_atmestas && pakartotas_atmestas
                );
            }


            // ==========================================
            // 5. PAKEISTAS BLOKO TURINYS, PREV_HASH, POW
            // ==========================================

            {
                auto pradine_busena = sukurtiTestiniusVartotojus();

                Transakcija t = sukurtiTestineTransakcija("A", "B", 10, 1);

                set<string> patvirtinti_id;

                // Teisingas blokas.
                Blokas geras_blokas = sukurtiTestiniBloka({t}, prev_hash);

                map<string, Vartotojas> galutine_busena;

                // Pakeičiamas transakcijos turinys, bet Merkle šaknis nekeičiama.
                Blokas pakeistas_turinys = geras_blokas;
                pakeistas_turinys.bloko_transakcijos[0].amount = 20;

                bool turinys_atmestas = !tikrintiBloka(
                    pakeistas_turinys,
                    pradine_busena,
                    prev_hash,
                    patvirtinti_id,
                    galutine_busena
                );

                // Pakeičiamas ankstesnio bloko hash.
                Blokas pakeistas_prev_hash = geras_blokas;
                pakeistas_prev_hash.prev_hash = funkcija("kitas blokas");

                bool prev_hash_atmestas = !tikrintiBloka(
                    pakeistas_prev_hash,
                    pradine_busena,
                    prev_hash,
                    patvirtinti_id,
                    galutine_busena
                );

                // Pakeičiamas nonce taip, kad Proof of Work nebegaliotų.
                Blokas netinkamas_pow = geras_blokas;

                do
                {
                    netinkamas_pow.nonce++;
                }
                while (netinkamas_pow.hash().starts_with(
                    string(netinkamas_pow.difficulty, '0')));

                bool pow_atmestas = !tikrintiBloka(
                    netinkamas_pow,
                    pradine_busena,
                    prev_hash,
                    patvirtinti_id,
                    galutine_busena
                );

                bool busena_nepasikeite =
                    pradine_busena["A"].balansas() == 100 &&
                    pradine_busena["B"].balansas() == 50;

                testoRezultatas(
                    "Pakeistas bloko turinys, prev_hash ir netinkamas PoW",
                    turinys_atmestas &&
                    prev_hash_atmestas &&
                    pow_atmestas &&
                    busena_nepasikeite
                );
            }


            // ==========================================
            // 6. MERKLE MEDIS: 1 / 2 / 3 / 5 TRANSAKCIJOS
            // ==========================================

            {
                vector<string> lapai = {
                    funkcija("T1"),
                    funkcija("T2"),
                    funkcija("T3"),
                    funkcija("T4"),
                    funkcija("T5")
                };

                bool vienas = merkle_root({lapai[0]}) == lapai[0];

                bool du =
                    merkle_root({lapai[0], lapai[1]}) ==
                    funkcija(lapai[0] + "|" + lapai[1]);

                string triju_tikras =
                    funkcija(
                        funkcija(lapai[0] + "|" + lapai[1]) +
                        "|" +
                        funkcija(lapai[2] + "|" + lapai[2])
                    );

                bool trys =
                    merkle_root({lapai[0], lapai[1], lapai[2]}) ==
                    triju_tikras;

                string penki_tikras =
                    funkcija(
                        funkcija(
                            funkcija(lapai[0] + "|" + lapai[1]) +
                            "|" +
                            funkcija(lapai[2] + "|" + lapai[3])
                        ) +
                        "|" +
                        funkcija(
                            funkcija(lapai[4] + "|" + lapai[4]) +
                            "|" +
                            funkcija(lapai[4] + "|" + lapai[4])
                        )
                    );

                bool penki =
                    merkle_root({
                        lapai[0], lapai[1], lapai[2], lapai[3], lapai[4]
                    }) == penki_tikras;

                testoRezultatas(
                    "Merkle medis su 1, 2, 3 ir 5 transakcijomis",
                    vienas && du && trys && penki
                );
            }


            // ==========================================
            // 7. KANDIDATU BUSENOS ATSKYRIMAS
            // ==========================================

            {
                auto pradine_busena = sukurtiTestiniusVartotojus();

                // Du kandidatai gauna atskiras tos pačios būsenos kopijas.
                auto kandidatas1 = pradine_busena;
                auto kandidatas2 = pradine_busena;

                Transakcija t1 = sukurtiTestineTransakcija("A", "B", 30, 1);
                Transakcija t2 = sukurtiTestineTransakcija("A", "C", 40, 2);

                bool pirmas_pavyko = vykdytiTransakcija(t1, kandidatas1);
                bool antras_pavyko = vykdytiTransakcija(t2, kandidatas2);

                // Kandidatų būsenos turi skirtis.
                bool busenos_skiriasi =
                    kandidatas1["A"].balansas() != kandidatas2["A"].balansas() &&
                    kandidatas1["B"].balansas() != kandidatas2["B"].balansas();

                // Pradinė būsena negali būti pakeista.
                bool pradine_nepakito =
                    pradine_busena["A"].balansas() == 100 &&
                    pradine_busena["B"].balansas() == 50 &&
                    pradine_busena["C"].balansas() == 20;

                testoRezultatas(
                    "Nelaimėjusių kandidatų būsenos nesusimaišo",
                    pirmas_pavyko &&
                    antras_pavyko &&
                    busenos_skiriasi &&
                    pradine_nepakito
                );
            }

            cout << "\n========== TESTAI BAIGTI ==========\n";
            break;
        }
    }
    return 0;
}