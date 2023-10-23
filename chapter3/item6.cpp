#include <iostream>
#include <mutex>
#include <thread>
#include <stack>


struct empty_stack :std::exception
{
    const char* what() const throw();
};


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


    void pop(T& value)
    {
        std::lock_guard<std::mutex> lock(m);
        if (data.empty())
        {
            throw empty_stack();
        }
        value = data.top();
        data.pop();
    }


    bool empty() const
    {
        std::lock_guard<std::mutex> lock(m);
        return data.empty();
    }
};


int main() {
    std::cout << "Hello, World!" << std::endl;
    return 0;
}
