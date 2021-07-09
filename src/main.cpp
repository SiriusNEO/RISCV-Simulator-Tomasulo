#include "cpu_core.hpp"

int main() {
#ifdef DEBUG
    std::cout << "Hello, Tomasulo!" << std::endl;
    //freopen("testcases/superloop.data", "r", stdin);
    freopen("sample.data", "r", stdin);
    freopen("myout.txt", "w", stdout);
#endif

    RISC_V::CPU Yakumo_Yukari;
    Yakumo_Yukari.run();
    return 0;
}