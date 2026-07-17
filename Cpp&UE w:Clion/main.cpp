#include <iostream>
using namespace std;

class Animal
{
protected:
    string name;

public:
    Animal(string n) : name(n)
    {
    }

    virtual void MakeSound()
    {
        cout << "Ses Çıkarır" << endl;
    }
};

class Dog : public Animal
{
public:
    Dog(string n) : Animal(n)
    {
    }

    void MakeSound() override
    {
        cout << name << " " << "Hav Hav" << endl;
    }

    static void MakeSound2()
    {
        cout << "Hav Hav 2" << endl;
    }
};


int main()
{
    Animal* hayvan = new Dog("Karabaş");
    Dog d("Köpke");

    hayvan->MakeSound();

    d.MakeSound();

    Dog::MakeSound2();

    unique_ptr<Dog> hayvan2 = make_unique<Dog>("Golden");

    hayvan2->MakeSound();

    return 0;
}
