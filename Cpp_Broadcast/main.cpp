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
        listeners.push_back(l);
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
