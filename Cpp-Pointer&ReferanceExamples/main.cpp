#include <iostream>
#include <string>
using namespace std;

class Kitap
{
public:
    string ad;
    Kitap(string a) : ad(a) {}
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

    cout << eleman(2)<< endl;
    eleman(2) = 99;
    cout << eleman(2)<< endl;

    Kitap kitap("Suç ve Ceza");
    yazdir(kitap);






    return 0;
}
