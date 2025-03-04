#include <spargel/base/test.h>

int main() {
    spargel::base::TestManager::getInstance()->runAll();
    return 0;
}
