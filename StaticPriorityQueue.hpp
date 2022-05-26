#include <stdint.h>
#include <array>

enum HEAP_TYPE{MINHEAP, MAXHEAP};

template<HEAP_TYPE PRIORITY, typename T, unsigned CAPACITY>
class StaticPriorityQueue{
    unsigned _size = 0;
    T arr[CAPACITY];

    public:
    void init(int n, T const& value) { for(int i = 0; i < n; i++) arr[i] = value; _size = n; }
    T const& top() const { return arr[0]; }
    void push(T const& value) {
        int i = _size;
        arr[i] = value;
        _size += 1;
        if constexpr ( PRIORITY == MINHEAP ){
            while (i != 0 && arr[i] < arr[i>>1]){
                auto aux = arr[i];
                arr[i] = arr[i>>1];
                arr[i>>1] = aux;
                i = i>>1;
            }
        }else{
            while (i != 0 && arr[i>>1] < arr[i]){
                auto aux = arr[i];
                arr[i] = arr[i>>1];
                arr[i>>1] = aux;
                i = i>>1;
            }
        }
    }

    void pop() {
        _size -= 1;
        arr[0] = arr[_size];
        
        int i = 0, most, l, r;
        while(true){
            most = i, l = i<<1, r = (i<<1) + 1;
            if constexpr ( PRIORITY == MINHEAP){
                if(l < _size && arr[l] < arr[i]) most = l;
                else most = i;
                if (r < _size && arr[r] < arr[most]) most = r;
                if(most == i) break;
                auto aux = arr[i];
                arr[i] = arr[most];
                arr[most] = aux;
                i = most;
            }else{
                if(l < _size && arr[i] < arr[l]) most = l;
                else most = i;
                if (r < _size && arr[most] < arr[r]) most = r;
                if(most == i) break;
                auto aux = arr[i];
                arr[i] = arr[most];
                arr[most] = aux;
                i = most;
            }
        }

    }

    bool not_empty() const {return _size != 0; }
    unsigned size() const { return _size; }
    void clear() { _size = 0; }

};