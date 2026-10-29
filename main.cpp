#include <iostream>
#include <boost/multiprecision/cpp_dec_float.hpp>
#include <boost/multiprecision/cpp_int.hpp>
#include <openssl/evp.h>
#include <ctime>
#include <random>
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
std::mt19937_64 generatorius(SEED);

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

struct Vartotojas
{
    string vardas;
    string public_key;
    size_t balance;
    // deque<Transakcija>vartotojo_transakcijos;
};
struct Transakcija
{
    string sender;
    string receiver;
    size_t amount;
    size_t sequence;
    string transaction_id; //=originali_funkcija(sender+receiver+std::to_string(amount)+std::to_string(sequence));
};

struct Blokas
{
    string prev_hash;
    size_t timestamp;
    size_t version;
    string tx_root;
    size_t nonce;
    size_t difficulty=3;

    vector<Transakcija>bloko_transakcijos;

    string hash()
    {
        return funkcija(prev_hash+"|"+std::to_string(timestamp)+"|"+std::to_string(version)+"|"+tx_root+"|"+std::to_string(nonce)+"|"+std::to_string(difficulty));
    }
};
int main()
{
    srand(static_cast<unsigned>(SEED));
    SetConsoleCP(CP_UTF8);        // konsolės įvestis → UTF-8
    SetConsoleOutputCP(CP_UTF8);
    int choice;
    cout<<"===Meniu==="<<endl;
    cout<<"1. Generuoti vartotojus ir transakcijas"<<endl;
    cout<<"2. Kasti blokus"<<endl;
    while(true)
    {
        try
        {
            cin>>choice;
            if(cin.fail() || choice<1 || choice>2)
                throw std::runtime_error("Iveskite skaiciu 1-2: ");

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
                size_t amount=rand()%vartotojai[sender].balance+1;
                string transakcijos_id=funkcija(sender+"|"+receiver+"|"+std::to_string(amount)+"|"+std::to_string(i+1));
                transakcijos.push_back({sender, receiver, amount, i+1, transakcijos_id});
                fr1<<left<<setw(25)<<sender<<setw(20)<<receiver<<setw(20)<<amount<<setw(10)<<i+1<<setw(20)<<transakcijos_id<<endl;
            }
            fr1.close();
            exit(0);
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
                    deque<Transakcija>transakciju_kopija=transakcijos;
                    map<string, Vartotojas>vartotoju_kopija=vartotojai;
                    Blokas naujas_blokas;
                    auto start = high_resolution_clock::now();
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
                        if(vartotoju_kopija[t.sender].balance<t.amount)
                            continue;
                        vartotoju_kopija[t.sender].balance-=t.amount;
                        vartotoju_kopija[t.receiver].balance+=t.amount;
                        naujas_blokas.bloko_transakcijos.push_back(t);
                    }
                    naujas_blokas.version=1;
                    string visu_transakciju_id;
                    for(int i=0; i<naujas_blokas.bloko_transakcijos.size(); i++)
                    {
                        if(i>0)
                            visu_transakciju_id+="|";
                        visu_transakciju_id+=naujas_blokas.bloko_transakcijos[i].transaction_id;
                    }
                    naujas_blokas.tx_root=funkcija(visu_transakciju_id);
                    naujas_blokas.timestamp=chrono::duration_cast<chrono::seconds>(chrono::system_clock::now().time_since_epoch()).count();
                    naujas_blokas.nonce=0;
                    size_t bandymai=0;
                    while(!naujas_blokas.hash().starts_with(std::string(naujas_blokas.difficulty,'0')))
                    {
                        naujas_blokas.nonce++;
                        bandymai++;
                    }
                    auto end = high_resolution_clock::now();
                    duration<double> laikas=end-start;

                    //patikrinimas

                    if(naujas_blokas.prev_hash==blockchain[blockchain.size()-1].hash() && naujas_blokas.hash().starts_with(std::string(naujas_blokas.difficulty,'0')))
                    {
                        deque<Transakcija>transakciju_kopija1=transakcijos;
                        map<string, Vartotojas>vartotoju_kopija1=vartotojai;
                        string visu_transakciju_id1="";
                        // while(naujas_blokas.bloko_transakcijos.size()>0)
                        bool galioja = true;
                        for(int i=0; i<naujas_blokas.bloko_transakcijos.size(); i++)
                        {
                            const Transakcija t=naujas_blokas.bloko_transakcijos[i];
                            
                            auto siuntejas = vartotoju_kopija1.find(t.sender);
                            auto gavejas = vartotoju_kopija1.find(t.receiver);

                            if (siuntejas == vartotoju_kopija1.end() || gavejas == vartotoju_kopija1.end())
                            {
                                galioja = false;
                                break;
                            }
                            if (t.sender == t.receiver || t.amount == 0)
                            {
                                galioja = false;
                                break;
                            }
                            if (siuntejas->second.balance < t.amount)
                            {
                                galioja = false;
                                break;
                            }
                            siuntejas->second.balance -= t.amount;
                            gavejas->second.balance += t.amount;
                            // auto it = find_if(transakciju_kopija1.begin(), transakciju_kopija1.end(), [&t](const Transakcija& x){return x.transaction_id == t.transaction_id;});
                            // if(it!=transakciju_kopija1.end())
                            //     transakciju_kopija1.erase(it);
                            if(i>0)
                                visu_transakciju_id1+="|";
                            visu_transakciju_id1+=t.transaction_id;
                        }
                        if(galioja && naujas_blokas.tx_root==funkcija(visu_transakciju_id1))
                        {
                            itrauktos_transakcijos+=naujas_blokas.bloko_transakcijos.size();
                            blockchain.push_back(naujas_blokas);
                            transakcijos=transakciju_kopija;
                            vartotojai=vartotoju_kopija;
                            cout<<"Priimtas naujas blokas!"<<endl;
                            cout<<"Aukštis: "<<blockchain.size()<<endl;
                            cout<<"Transakcijų skaičius: "<<naujas_blokas.bloko_transakcijos.size()<<endl;;
                            cout<<"prev_hash: "<<naujas_blokas.prev_hash<<endl;
                            cout<<"tx_root: "<<naujas_blokas.tx_root<<endl;
                            cout<<"Bloko hash: "<<naujas_blokas.hash()<<endl;
                            cout<<"Difficulty: "<<naujas_blokas.difficulty<<endl;
                            cout<<"Nonce: "<<naujas_blokas.nonce<<endl;
                            cout<<"Bandymų skaičius: "<<bandymai<<endl;
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
                    galutine_balansu_suma+=vartotojai["PUB"+std::to_string(i+1)].balance;
                cout<<"Galutinė lėšų suma: "<<galutine_balansu_suma<<endl;
                cout<<"Bendras laikas: "<<pilnas_laikas.count()<<endl;
                break;
        }
    }
    return 0;
}