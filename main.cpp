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

string originali_funkcija(string s)
{
    if(s.empty())
    {
        return "Failas yra tuščias";
    }
    cpp_int maximum = (cpp_int(1) << 256) - 1;
    cpp_int hash=1;
    for(unsigned char c:s)
    {
        hash=hash*257+c+1;
        if(c!='\0')
            hash*=c;
    }
    int l=to_string(hash).length();
    cpp_dec_float_100 Hash=cpp_dec_float_100(hash);
    for(int i=0; i<l; i++)
        Hash/=10;
    Hash *= cpp_dec_float_100(maximum);
    hash=cpp_int(Hash);
    stringstream ss;
    ss<<hex<<hash;
    return ss.str();
}

struct Vartotojas
{
    // int user_id;
    string vardas;
    string public_key;
    int balance;
    
};
struct Transakcija
{
    string sender;
    string receiver;
    int amount;
    int sequence;
    string transaction_id; //=originali_funkcija(sender+receiver+std::to_string(amount)+std::to_string(sequence));
};

// void Generuoti_vartotojus()
// {

// }
int main()
{
    srand(static_cast<unsigned>(time(NULL)));
    SetConsoleCP(CP_UTF8);        // konsolės įvestis → UTF-8
    SetConsoleOutputCP(CP_UTF8);
    // int choice;
    // cout<<"===Meniu==="<<endl;
    // cout<<"1. Generuoti vartotojus"<<endl;
    // // cout<<"2. Suvesti duomenis ranka"<<endl;
    // // cout<<"3. Atlikti pirma eksperimenta"<<endl;
    // // cout<<"4. Atlikti antra eksperimenta"<<endl;
    // // cout<<"5. Atlikti ketvirta eksperimenta"<<endl;
    // // cout<<"6. Atlikti penkta eksperimenta"<<endl;
    // // cout<<"7. Atlikti sesta eksperimenta"<<endl;
    // // cout<<"8. Atlikti septinta eksperimenta"<<endl;
    // // cout<<"9. Atlikti eksperimenta, skirta palyginti savo sukurta algoritma su standartiniu SHA-256 algoritmu"<<endl;
    // while(true)
    // {
    //     try
    //     {
    //         cin>>choice;
    //         if(cin.fail() || choice<1 || choice>1)
    //             throw std::runtime_error("Iveskite skaiciu 1: ");

    //         cin.ignore(1000, '\n');
    //         break;
    //     }
    //     catch(const std::exception& e)
    //     {
    //         cerr<<"Klaida! "<<e.what()<<endl;
    //         cin.clear();
    //         cin.ignore(1000, '\n');
    //     }
    // }
    // switch(choice)
    // {
    //     case 1:
    //     {

    //     }
    // }
    ofstream fr("vartotojai.txt");
    vector<Vartotojas> vartotojai;
    fr<<left<<setw(25)<<"Vardas"<<setw(20)<<"Balansas"<<setw(20)<<"Viešas raktas"<<endl;
    for(int i=0; i<1000; i++)
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
        int balansas=rand()%(1000000-100+1)+100;
        vartotojai.push_back({vardas, "PUB"+std::to_string(i+1), balansas});
        fr<<left<<setw(25)<<vardas<<setw(20)<<balansas<<setw(20)<<"PUB"+std::to_string(i+1)<<endl;

    }



    //Tranzakciju kurimas
    vector<Transakcija>transakcijos;
    ofstream fr1("transakcijos.txt");
    fr1<<left<<setw(25)<<"Siuntėjas"<<setw(20)<<"Gavėjas"<<setw(20)<<"Suma"<<setw(10)<<"Sequence"<<setw(20)<<"Transaction_id"<<endl;
    for(int i=0; i<10000; i++)
    {
        string sender, receiver;
        do
        {
            sender="PUB"+std::to_string(rand()%1000+1);
            receiver="PUB"+std::to_string(rand()%1000+1);
        } while (sender==receiver);
        int amount=rand()%1000000+1;
        string transakcijos_id=originali_funkcija(sender+"|"+receiver+"|"+std::to_string(amount)+"|"+std::to_string(i+1));
        transakcijos.push_back({sender, receiver, amount, i+1, transakcijos_id});
        fr1<<left<<setw(25)<<sender<<setw(20)<<receiver<<setw(20)<<amount<<setw(10)<<i+1<<setw(20)<<transakcijos_id<<endl;
    }
    fr.close();
    fr1.close();
    return 0;
}