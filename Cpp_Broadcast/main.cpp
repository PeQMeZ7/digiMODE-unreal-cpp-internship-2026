#include <iostream>
#include <vector>
using namespace std;

// ---- Arayüz: dinleyici sözleşmesi ----
class IHealthListener
{
public:
    virtual void OnHealthChanged(int hp) = 0;
    virtual ~IHealthListener() = default; // derleyiciye standart prosedürü uygula dersin!
};


// ---- YAYINCI (Publisher) ----

class Event
{
    // Abone fonksiyonların listesi
    vector<IHealthListener*> listeners;

public:
    // Abone ol
    void AddListener(IHealthListener* l)
    {
        if (!l) // null koruması

        {
            return;
        }
        // ─────────────── 2. SATIR: arama (varlık kontrolü) ───────────────

if (find(                               // ① Baştan sona tarar, ilk eşleşmede durur
        listeners.begin(),              //   nereden → ilk eleman
        listeners.end(),                //   nereye kadar → son elemanın bir sonrası
        l                               //   neyi ara → bu değeri
    )                                   // ② Dönüş: BULDUYSA bulduğu elemanın imleci
                                        //           BULAMADIYSA end()

    == listeners.end())                 // ③ "Dönen şey end() mi?" diye soruyoruz.
                                        //   end() hiçbir elemanı göstermez (rafın bittiği
                                        //   yerdeki tabela), o yüzden "yok" sinyali olarak
                                        //   kullanılır — başka dillerdeki -1 / null gibi.
                                        //   == end()  →  BULUNAMADI
                                        //   != end()  →  BULUNDU
{
    listeners.push_back(l);             // Listede yoktu → şimdi ekle.
}                                       // Vardıysa hiçbir şey yapma → çift kayıt engellenmiş
                                        // olur. (Senin UE'deki AddDynamic bugunun panzehiri.)

// C++20 karşılığı: if (ranges::find(listeners, l) == listeners.end())
    }

    void RemoveListener(IHealthListener* l)
    {


        // ─────────────── 1. SATIR: silme (erase-remove deyimi) ───────────────

        listeners.erase(                        // ④ vector'ün KENDİ metodu. Konteyneri tanıdığı için
                                                //   boyut değiştirebilir. Asıl silen bu.
                                                //   İki imleç alır: "şuradan şuraya kadar olanı at"

            remove(                          // ① Bu önce çalışır. Bir ALGORİTMA — konteyneri
                                                //   görmez, sadece imleç alır, o yüzden boyutu
                                                //   DEĞİŞTİREMEZ. Adı yalan söylüyor: silmiyor.
                                                //   Yaptığı: kalacakları öne kaydırıp üzerine yazmak.

                listeners.begin(),              //   nereden taramaya başla → ilk eleman
                listeners.end(),                //   nereye kadar → son elemanın BİR SONRASI
                l                               //   neyi ayıkla → bu değere eşit olan her eleman
            ),                                  // ② Dönüş: kalanların bittiği yerin imleci
                                                //   ("yeni mantıksal son"). Buradan sonrası çöp.
                                                //   Bu imleç, erase'in 1. argümanı oluyor.

            listeners.end()                     // ③ erase'in 2. argümanı: fiziksel son.
                                                //   Yani "yeni son"dan "eski son"a kadar olan
                                                //   çöp kuyruğu kesilip atılıyor.
        );
        // Özet: remove SIRALAR, erase KESER. İkisi birlikte gerçek silme yapar.
        // C++20 karşılığı tek satır: erase(listeners, l);
    }

    // Herkese haber ver
    void Broadcast(int hp)
    {
        for (int i = 0; i < listeners.size(); i++)
        {
            IHealthListener* l = listeners[i];

            l->OnHealthChanged(hp);
        }
    }
};

// ---- DINLEYICILER ----

class UI : public IHealthListener
{
public:
    void OnHealthChanged(int hp) override
    {
        cout << "[UI] " << hp << endl;
    }
};

class SoundSystem : public IHealthListener
{
public:
    void OnHealthChanged(int hp) override
    {
        cout << "[Ses] Ah! " << hp << endl;
    }
};

class Player
{
public:
    Event OnHealthChanged;
    int health = 100;

    void TakeDamage(int d)
    {
        health -= d;
        OnHealthChanged.Broadcast(health);
    }
};


int main()
{
    Player p;
    UI ui;
    SoundSystem sound;

    p.OnHealthChanged.AddListener(&ui);
    p.OnHealthChanged.AddListener(&sound);
    p.TakeDamage(30);

    return 0;
}
