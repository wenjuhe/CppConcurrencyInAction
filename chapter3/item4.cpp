#include <iostream>
#include <mutex>
#include <thread>
#include <stack>


template<typename T>
class threadsafe_stack
{
private:
    std::stack<T> data;
    mutable std::mutex m;
public:
    threadsafe_stack(){}
    //拷贝构造
    threadsafe_stack(const threadsafe_stack& other)
    {
        /*
         * todo 锁住入参,因为拷贝的时候防止入参修改.
         * */
        std::lock_guard<std::mutex> lock(other.m);
        //todo 在构造函数的函数体(constructor body)内进行复制操作.
        data = other.data;
    }
    //todo 拷贝赋值
    threadsafe_stack& operator=(const threadsafe_stack&) = delete;

    void push(T new_value)
    {
        std::lock_guard<std::mutex> lock(m);
        data.push(std::move(new_value));
    }

    //todo 问题代码
    T pop()
    {
        std::lock_guard<std::mutex> lock(m);
        auto element = data.top();
        data.pop();
        return element;
    }

    bool empty() const
    {
        std::lock_guard<std::mutex> lock(m);
        return data.empty();
    }
};

void test_threadsafe_stack()
{
    threadsafe_stack<int> safe_stack;
    safe_stack.push(1);

    std::thread t1([&safe_stack](){
        if (!safe_stack.empty())
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            safe_stack.pop();
        }
    });

    std::thread t2([&safe_stack](){
        if (!safe_stack.empty()) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            safe_stack.pop();
        }
    });

    t1.join();
    t2.join();
}




int main() {
    std::cout << "Hello, World!" << std::endl;
    test_threadsafe_stack();
    return 0;
}
