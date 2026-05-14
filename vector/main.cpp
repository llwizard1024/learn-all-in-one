#include "Vector.h"

#include <iostream>

int main() {

    Vector<int> arr;
    arr.push_back(10);
    arr.push_back(20);

    arr.pop_back();

    std::cout << arr.size() << std::endl;
    std::cout << arr.capacity() << std::endl;
    std::cout << arr[0] << std::endl;
    arr.push_back(45);
    std::cout << arr[1] << std::endl;
    arr.pop_back();
    arr.pop_back();
    arr.pop_back();

    if (arr.empty()) {
        std::cout << "Is empty!\n";
    }

    return 0;
}