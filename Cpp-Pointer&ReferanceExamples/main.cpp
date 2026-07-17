#include <iostream>
#include <string>
#include <memory>
using namespace std;

class Kitap
{
public:
    string ad;

    Kitap(string a) : ad(a)
    {
    }
};

void yazdir(const Kitap& k)
{
    cout << k.ad << endl;
}

void ikiyeKatla(int& sayi) // Refernas parametre
{
    sayi = sayi * 2;
}

void degistirWithReferance(int& a, int& b)
{
    int temp = a;
    a = b;
    b = temp;
}

void degistirWithPointer(int* a, int* b)
{
    int temp = *a;
    *a = *b;
    *b = temp;
}

int dizi[5] = {10, 20, 30, 40, 50};

int& eleman(int i)
{
    return dizi[i];
}

//Deneme
void ucKatla(int& x)
{
    x *= 3;
}

void ucKatlaPtr(int* x)
{
    *x *= 3;
}

class Ogrenci
{
    string ogrenciAd;
    int ogrenciNot;

public:
    Ogrenci(string a, int x) : ogrenciAd(a), ogrenciNot(x)
    {
    }

    string getOgrenciad() const
    {
        return ogrenciAd;
    }

    int getOgrencinot() const
    {
        return ogrenciNot;
    }
};

void bilgiYazdir(const Ogrenci& o)
{
    cout << "Öğrenci Adı: " << o.getOgrenciad() << " Öğrenci Notu: " << o.getOgrencinot() << endl;
}

//Pointer ile erişimler

class Dog
{
public:
    string ad;
    void havla() { cout << ad << ": Hav" << endl; }

    void setAd(string ad)
    {
        this->ad = ad;
    }

    Dog* kendisi()
    {
        return this;
    }
};

class Motor
{
public:
    void calis()
    {
        cout << "Vroom" << endl;
    }
};

class Araba
{
    Motor* motor;

public:
    Araba(Motor* m) : motor(m)
    {
    }

    void surus()
    {
        motor->calis();
    }
};

class Hesap
{
    int bakiye = 0;

public:
    Hesap* yatir(int m)
    {
        bakiye += m;
        return this;
    }

    Hesap* cek(int m)
    {
        bakiye -= m;
        return this;
    }

    void goster() { cout << bakiye << endl; }
};



//Polimorfizm + pointer

class Hayvan
{
public:
    virtual void sesCikar() { cout << "Ses" << endl; }

    virtual ~Hayvan()
    {
    }
};

class Kopek : public Hayvan
{
    public:
    void sesCikar() override { cout << "Hav" << endl; }
};

class Car
{
    unique_ptr<Motor> motor;    // Araba motoru sahiplenir, otomatik siler
public:
    Car(): motor(make_unique<Motor>()){}
    void surus() { motor->calis(); }

};


int main()

{
    int x = 10;


    int* p = &x; // pointer: x'in adresini tut
    int& r = x; // referans: x'e takma ad

    cout << *p << endl; // 10  → pointer'da yıldız (*) ile değere ulaş
    cout << &p << endl;
    cout << p << endl;
    cout << &x << endl;


    cout << r << endl; // 10  → referansta yıldız YOK, direkt kullan

    *p = 20; // pointer üzerinden değiştir
    cout << x << endl; // 20

    r = 30; // referans üzerinden değiştir (direkt)
    cout << x << endl; // 30

    r = 2; // x'i değiştirdi!
    cout << x << endl;

    int* nullpointer = nullptr; // pointer boş olabilir
    // int& r;             // HATA: referans mutlaka bir şeye bağlanmalı

    int deneme = 5;
    ikiyeKatla(deneme);
    cout << deneme << endl;

    int swap1 = 5;
    int swap2 = 6;
    cout << swap1 << " " << swap2 << endl;
    degistirWithReferance(swap1, swap2);
    cout << swap1 << " " << swap2 << endl;

    int swap3 = 8;
    int swap4 = 9;

    cout << swap3 << " " << swap4 << endl;

    degistirWithPointer(&swap3, &swap4);

    cout << eleman(2) << endl;
    eleman(2) = 99;
    cout << eleman(2) << endl;

    Kitap kitap("Suç ve Ceza");
    yazdir(kitap);

    int deneme1 = 4;
    ucKatla(deneme1);
    cout << deneme1 << endl;

    int deneme2 = 5;
    ucKatlaPtr(&deneme2);
    cout << deneme2 << endl;

    Ogrenci ogrenci1("Recep", 60);
    bilgiYazdir(ogrenci1);


    //Pointer ile erişimler
    cout << "---------------------" << endl;
    cout << "Pointer ile erişimler" << endl;
    cout << endl;


    Dog d;
    d.ad = "Karabaş";
    d.havla();

    Dog* dogPointer = &d;
    dogPointer->ad = "Garip Kont";
    dogPointer->havla();

    Motor m;
    Araba a(&m);
    a.surus();

    dogPointer->setAd("ScoobyDoo");
    dogPointer->havla();
    cout << dogPointer->kendisi() << endl;

    cout << endl;

    Hesap hesapDeneme;
    hesapDeneme.yatir(100)->cek(30)->yatir(50)->goster(); //120

    //Polimorfizm + pointer

    Hayvan* h = new Kopek(); // base pointer, türetilmiş nesne
    h->sesCikar();
    delete h;

    //Pointer's Pointer

    cout << endl;
    cout << "------------------" << endl;

    int integer = 10;
    int* integerPointer = &integer;
    int** pointerPointer = &integerPointer;

    cout << integer << endl;
    cout << *integerPointer << endl;
    cout << **pointerPointer << endl;

    **pointerPointer = 5;
    cout << integer << endl;
    *integerPointer = 9;
    cout << integer << endl;



    return 0;
}
