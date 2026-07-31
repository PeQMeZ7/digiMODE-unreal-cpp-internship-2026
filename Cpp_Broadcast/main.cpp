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


// ---- 1. YAYINCI (Publisher) ----

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

// ---- 2. YAYIN YAPAN SINIF ----



int main()
{
    return 0;
}
