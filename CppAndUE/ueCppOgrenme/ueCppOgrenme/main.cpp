//
//  main.cpp
//  ueCppOgrenme
//
//  Created by Recep AKKUŞ on 14.07.2026.
//

#include <iostream>
using namespace std;


class Animal {
public:
    virtual void MakeSound() const;   // const VAR
};

class Dog : Animal
{
public:
    virtual void MakeSound() override
    
    
    
}

int main(int argc, const char * argv[]) {
    // insert code here...
    cout << "Hello" << endl;
    return 0;
    
    
}

