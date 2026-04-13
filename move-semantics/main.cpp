#include <iostream>

class Buffer
{
    int* _array;
    size_t _size;
public:
    Buffer(size_t n) {
        _array = new int[n];
        _size = n;
        
        for (size_t i = 0; i < n; ++i) {
            _array[i] = 0;
        }
    }
    
    Buffer(Buffer&& other) noexcept : _array(other._array), _size(other._size) {
        other._array = nullptr;
        other._size = 0;
    }
    
    Buffer& operator=(Buffer&& other) noexcept {
        delete[] _array;
        
        _array = other._array;
        _size = other._size;
        
        other._array = nullptr;
        other._size = 0;
         
        return *this;
    }
    
    ~Buffer() { 
        delete[] _array;
        _size = 0;
    }
    
    int getElement(const int number) const {
        return _array[number];
    }
    
    size_t getSize() const {
        return _size;
    }
    
    void print() const {
        for (size_t i = 0; i < _size; ++i) {
            std::cout << _array[i] << std::endl;
        }
    }
    
    void setValue(int index, int value) {
        _array[index] = value;
    }
};


int main()
{
    Buffer local(5);
    local.setValue(0, 1);
    local.setValue(1, 2);
    local.setValue(2, 3);
    local.setValue(3, 4);
    local.setValue(4, 5);
    local.print();
    
    Buffer array(10);
    
    array = std::move(local); // After STD Move local empty, all data move to array var.
    
    array.print();

    return 0;
}