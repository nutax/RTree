#include <iostream>
#include "StaticPriorityQueue.hpp"
#include "StaticQueue.hpp"

StaticPriorityQueue<MAXHEAP, int, 100> q;
StaticQueue<int, 100> pq;

int main(){
    q.push(0);
    q.push(1);
    q.push(4);
    q.push(2);
    q.push(3);
    

    for(;q.not_empty(); q.pop())
    std::cout << q.top() << std::endl;

    std::cout << std::endl;

    q.push(7);
    q.push(4);
    q.push(2);
    q.push(2);
    q.push(8);

    for(;q.not_empty(); q.pop())
    std::cout << q.top() << std::endl;
    return 0;
}