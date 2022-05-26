template<typename T, unsigned CAPACITY>
class StaticQueue{
    unsigned _size = 0, head = 0;
    T arr[CAPACITY];

    public:
    T& top(){ return arr[head]; }
    void push(T const& value){arr[(head+_size)%CAPACITY] = value; _size += 1; }
    void pop(){ head = (head+1)%CAPACITY; _size -= 1; }
    bool not_empty() const { return _size != 0; }
    unsigned size() const { return _size; }
    void clear(){ _size = 0; }
};