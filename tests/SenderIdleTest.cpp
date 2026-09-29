#include "components/OSCSender.h"
#include "components/VMCSender.h"

#include <chrono>
#include <thread>

int main() {
    VMCSender vmc("127.0.0.1", 39541);
    OSCSender osc("127.0.0.1", 9001);
    // Both workers must stay alive without any gloves or queued packets.
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}
