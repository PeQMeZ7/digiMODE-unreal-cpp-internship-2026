#include <iostream>
#include <memory>

class Player
{
public:
    int Health = 100;
};

int main()
{
    auto player = std::make_unique<Player>();

    std::cout << player->Health << std::endl;
{{{{{{{{{{{{{{{
    {




    }}}}}}}}}}}}}}}}
    return 0;
}