#include <iostream>
#include <vector>
#include <algorithm>
using namespace std;

// ============================================================================
//  OBSERVER (GOZLEMCI) DESENI — Arayuz Tabanli Olay Sistemi
// ----------------------------------------------------------------------------
//  AMAC
//  Player'in, kendisini dinleyen siniflari (UI, ses, basarim) HIC TANIMADAN
//  onlara haber verebilmesi. Player'in icinde tek bir UI* bile yok.
//  Yeni bir dinleyici eklemek icin Player'in koduna dokunmuyorsun.
//
//  YAPI — 4 parca
//    1) Arayuzler   (IHealthListener, IDeathListener, IHealListener)
//       Sozlesme: "Beni dinlemek isteyen su fonksiyonu yazmak zorunda."
//       "= 0" saf sanaldir; govdesi yoktur, turemis sinif yazmak ZORUNDADIR.
//
//    2) Event siniflari (HealthEvent, DeathEvent, HealEvent)
//       Yayinci. Icinde dinleyici pointer'larindan olusan bir liste tutar.
//         AddListener    -> listeye ekler (null ve cift kayit korumali)
//         RemoveListener -> listeden cikarir (erase-remove deyimi)
//         Broadcast      -> listedeki herkesi sirayla cagirir
//       Listenin tipi vector<IHealthListener*> — yani vector<UI*> DEGIL.
//       Butun bagimsizlik bu satirda gizli: Event, UI diye bir sinif
//       oldugunu bilmiyor.
//
//    3) Dinleyiciler (UI, SoundSystem, AchievementSystem)
//       Arayuzu miras alip kendi tepkisini yazarlar. Bir sinif birden fazla
//       arayuz alabilir; sadece ilgilendigi olaylari dinler.
//         UI          -> uc olayi da dinler
//         SoundSystem -> can + olum
//         Achievement -> sadece olum
//
//    4) Player
//       Olaylarin sahibi. Durumu degistigi anda ilgili Broadcast'i cagirir.
//       Once durumu gunceller, SONRA haber verir — sira onemli, cunku
//       dinleyiciler haberi alinca guncel degeri okumak isteyebilir.
//
//  CALISMA AKISI   (ornek: p.TakeDamage(30))
//    main        AddListener(&ui)  ->  ui'nin ADRESI listeye girer (kopyasi degil)
//    TakeDamage  health 100 -> 70
//                OnHealthChanged.Broadcast(70)
//    Broadcast   listeyi gezer, her eleman icin  l->OnHealthChanged(70)
//    UI / Ses    kendi metotlari calisir, ekrana yazar
//
//  ISIN SIHRI
//    Broadcast icindeki  l->OnHealthChanged(hp)  satiri TEK satirdir ama
//    UI icin baska, SoundSystem icin baska bir fonksiyon calistirir.
//    Sebep: virtual. Hangi fonksiyonun calisacagina derleme aninda degil,
//    CALISMA aninda nesnenin kendisi karar verir (runtime polymorphism).
//
//  DIKKAT — sarkan pointer
//    Liste ham pointer tutuyor. Bir dinleyici Player'dan once yok olursa
//    listede gecersiz adres kalir ve Broadcast coker. Gercek projede
//    dinleyicinin yikicisinda RemoveListener cagrilmalidir.
//
//  UE KARSILIGI
//    Event sinifi    ->  DECLARE_DYNAMIC_MULTICAST_DELEGATE
//    AddListener     ->  AddDynamic
//    RemoveListener  ->  RemoveDynamic
//    Broadcast       ->  Broadcast
//    Cift kayit kontrolu, AddDynamic'in yanlis yerde cagrilmasindan dogan
//    katlanan cagri hatasinin panzehiridir.
// ============================================================================

// ---- Arayüz: dinleyici sözleşmesi ----
class IHealthListener
{
public:
    virtual void OnHealthChanged(int hp) = 0;
    virtual ~IHealthListener() = default; // derleyiciye standart prosedürü uygula dersin!
};

class IDeathListener
{
public:
    virtual void OnDeath() = 0;
    virtual ~IDeathListener() = default;
};

class IHealListener
{
public:
    virtual void OnHeal(int amount) = 0;
    virtual ~IHealListener() = default;
};

// ---- YAYINCI (Publisher) ----

class HealthEvent
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

        if (find( // ① Baştan sona tarar, ilk eşleşmede durur
                listeners.begin(), //   nereden → ilk eleman
                listeners.end(), //   nereye kadar → son elemanın bir sonrası
                l //   neyi ara → bu değeri
            ) // ② Dönüş: BULDUYSA bulduğu elemanın imleci
            //           BULAMADIYSA end()

            == listeners.end()) // ③ "Dönen şey end() mi?" diye soruyoruz.
        //   end() hiçbir elemanı göstermez (rafın bittiği
        //   yerdeki tabela), o yüzden "yok" sinyali olarak
        //   kullanılır — başka dillerdeki -1 / null gibi.
        //   == end()  →  BULUNAMADI
        //   != end()  →  BULUNDU
        {
            listeners.push_back(l); // Listede yoktu → şimdi ekle.
        } // Vardıysa hiçbir şey yapma → çift kayıt engellenmiş
        // olur. (Senin UE'deki AddDynamic bugunun panzehiri.)

        // C++20 karşılığı: if (ranges::find(listeners, l) == listeners.end())
    }

    void RemoveListener(IHealthListener* l)
    {
        // ─────────────── 1. SATIR: silme (erase-remove deyimi) ───────────────

        listeners.erase( // ④ vector'ün KENDİ metodu. Konteyneri tanıdığı için
            //   boyut değiştirebilir. Asıl silen bu.
            //   İki imleç alır: "şuradan şuraya kadar olanı at"

            remove( // ① Bu önce çalışır. Bir ALGORİTMA — konteyneri
                //   görmez, sadece imleç alır, o yüzden boyutu
                //   DEĞİŞTİREMEZ. Adı yalan söylüyor: silmiyor.
                //   Yaptığı: kalacakları öne kaydırıp üzerine yazmak.

                listeners.begin(), //   nereden taramaya başla → ilk eleman
                listeners.end(), //   nereye kadar → son elemanın BİR SONRASI
                l //   neyi ayıkla → bu değere eşit olan her eleman
            ), // ② Dönüş: kalanların bittiği yerin imleci
            //   ("yeni mantıksal son"). Buradan sonrası çöp.
            //   Bu imleç, erase'in 1. argümanı oluyor.

            listeners.end() // ③ erase'in 2. argümanı: fiziksel son.
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

class DeathEvent
{
    vector<IDeathListener*> listeners;

public:
    void AddListener(IDeathListener* l)
    {
        if (!l)
        {
            return;
        }
        if (find(listeners.begin(), listeners.end(), l) != listeners.end()) return;
        listeners.push_back(l);
    }

    void RemoveListener(IDeathListener* l)
    {
        listeners.erase(remove(listeners.begin(), listeners.end(), l), listeners.end());
    }

    void Broadcast()
    {
        for (int i = 0; i < listeners.size(); i++)
        {
            IDeathListener* l = listeners[i];
            l->OnDeath();
        }
    }
};

class HealEvent
{
    vector<IHealListener*> listeners;

public:
    void AddListener(IHealListener* l)
    {
        if (!l) return;
        if (find(listeners.begin(), listeners.end(), l) != listeners.end()) return;
        listeners.push_back(l);
    }

    void RemoveListener(IHealListener* l)
    {
        listeners.erase(remove(listeners.begin(), listeners.end(), l), listeners.end());
    }

    void Broadcast(int amount)
    {
        for (int i = 0; i < listeners.size(); i++)
        {
            IHealListener* l = listeners[i];
            l->OnHeal(amount);
        }
    }
};

// ---- DINLEYICILER ----

class UI : public IHealthListener, public IDeathListener, public IHealListener
{
public:
    void OnHealthChanged(int hp) override { cout << "\n[UI]  can barisi guncellendi: " << hp << "\n"; }
    void OnDeath() override { cout << "[UI]  OYUN BITTI ekrani\n"; }
    void OnHeal(int amount) override { cout << "[UI]  +" << amount << " yesil yazi\n"; }
};

class SoundSystem : public IHealthListener, public IDeathListener
{
public:
    void OnHealthChanged(int hp) override { cout << "[Ses] ah! (" << hp << ")\n"; }
    void OnDeath() override { cout << "[Ses] olum muzigi\n"; }
};

class AchievementSystem : public IDeathListener
{
public:
    void OnDeath() override { cout << "[Bas] 'Ilk olum' basarımı acıldı!\n"; }
};

class Player
{
public:
    HealthEvent OnHealthChanged;
    DeathEvent OnDeath;
    HealEvent OnHeal;

    int health = 100;

    void TakeDamage(int d)
    {
        health -= d;
        OnHealthChanged.Broadcast(health);
        if (health <= 0)
        {
            OnDeath.Broadcast();
        }
    }

    void Heal(int amount)
    {
        health += amount;
        OnHeal.Broadcast(amount);
        OnHealthChanged.Broadcast(health);
    }
};


int main()
{
    Player p;
    UI ui;
    SoundSystem sound;
    AchievementSystem achievements;

    p.OnHealthChanged.AddListener(&ui);
    p.OnHealthChanged.AddListener(&sound);
    p.OnDeath.AddListener(&ui);
    p.OnDeath.AddListener(&sound);
    p.OnDeath.AddListener(&achievements);
    p.OnHeal.AddListener(&ui);

    cout << "----- 30 hasar -----\n";
    p.TakeDamage(30);

    cout << "\n----- 15 iyilesme -----\n";
    p.Heal(15);

    cout << "\n--- 85 Hasar (Olumcul) ---\n";
    p.TakeDamage(85);

    return 0;
}
