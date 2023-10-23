#include <iostream>
#include <mutex>
#include <thread>


std::mutex mtx;
int share_data = 100;

void use_lock() {
    while (true) {
        mtx.lock();
        share_data++;
        std::cout << "current thread is " << std::this_thread::get_id() << std::endl;
        std::cout << "shared data is " << share_data << std::endl;
        mtx.unlock();
        std::this_thread::sleep_for(std::chrono::seconds(10));
    }
}

void test_lock(){
    std::thread t1(use_lock);

    std::thread t2([](){
        while(true){
            mtx.lock();
            share_data--;
            std::cout << "current thread is " << std::this_thread::get_id() << std::endl;
            std::cout << "shared data is " << share_data << std::endl;
            mtx.unlock();
            std::this_thread::sleep_for(std::chrono::seconds(10));
        }
    });
    t1.join();
    t2.join();
}


int main() {
    std::cout << "Hello, World!" << std::endl;
    test_lock();
    return 0;
}
