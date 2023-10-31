#include <iostream>
#include <thread>
#include <mutex>
#include <map>
#include <shared_mutex>
#include <memory>
#include <condition_variable>
#include <queue>

int num = 1;
std::mutex mtx_num;
std::condition_variable cvA;
std::condition_variable cvB;

void PoorImplemention()
{
    std::thread t1([](){
        for(;;)
        {
            {
                std::lock_guard<std::mutex> lock(mtx_num);
                if (num == 1)
                {
                    std::cout << "thread A print 1" << std::endl;
                    num++;
                    continue;
                }
            }
            std::this_thread::sleep_for(std::chrono::seconds(500));
        }
    });


    std::thread t2([](){
        for(;;)
        {
            {
                std::lock_guard<std::mutex> lock(mtx_num);
                if (num == 2)
                {
                    std::cout << "thread B print 2" << std::endl;
                    num--;
                    continue;
                }
            }
            std::this_thread::sleep_for(std::chrono::seconds(500));
        }
    });

    t1.join();
    t2.join();
}


void ResonableImplemention()
{
    std::thread t1([](){
        for (;;)
        {
            std::unique_lock<std::mutex> lock(mtx_num);
            /*
             * todo false ---> 挂起 ---> 释放锁
             * */
            cvA.wait(lock,[](){
                //todo 谓词
                return num == 1;
            });
            /*
             * todo 上面的语句等价于 用while而没有用if 防止虚假唤醒.
             * */
            while(num != 1)
            {
                cvA.wait(lock);
            }
            num++;
            std::cout << "thread A print 1..." << std::endl;
            cvB.notify_one();
        }
    });

    std::thread t2([](){
        for(;;)
        {
            std::unique_lock<std::mutex> lock(mtx_num);
            cvB.wait(lock,[](){
                return num == 2;
            });
            num--;
            std::cout << "thread B print 2" << std::endl;
            cvA.notify_one();
        }
    });
    t1.join();
    t2.join();
}

template<typename T>
class threadsafe_queue
{
private:
    mutable std::mutex mut;
    std::queue<T> data_queue;
    std::condition_variable data_cond;
public:
    threadsafe_queue(){}
    /*
     * todo 拷贝构造
     *  设置成 const引用的两个好处:
     *  1.接受常量类型的变量
     *  2.不支持移动构造的时候,执行移动构造会调用拷贝构造.
     * */
    threadsafe_queue(threadsafe_queue const& other)
    {
        std::lock_guard<std::mutex> lk(other.mut);
        data_queue = other.data_queue;
    }
    void push(T new_value)
    {
        std::lock_guard<std::mutex> lk(mut);
        data_queue.push(new_value);
        data_cond.notify_one();
    }
    void wait_and_pop(T& value)
    {
        std::unique_lock<std::mutex> lk(mut);
        data_cond.wait(lk,[this]{return !data_queue.empty();});
        value = data_queue.front();
        data_queue.pop();
    }
    std::shared_ptr<T> wait_and_pop()
    {
        std::unique_lock<std::mutex> lk(mut);
        data_cond.wait(lk,[this]{return !data_queue.empty();});
        /*
         * todo 不要返回一个局部变量的地址或者局部变量的引用.
         *  智能指针不存在这个问题.
         * */
        std::shared_ptr<T> res(std::make_shared<T>(data_queue.front()));
        data_queue.pop();
        return res;
    }
    bool try_pop(T& value)
    {
        std::lock_guard<std::mutex> lk(mut);
        if (data_queue.empty())
            return false;
        value = data_queue.front();
        data_queue.pop();
        return true;
    }
    std::shared_ptr<T> try_pop()
    {
        std::lock_guard<std::mutex> lk(mut);
        if (data_queue.empty())
            return std::shared_ptr<T>();
        std::shared_ptr<T> res(std::make_shared<T>(data_queue.front()));
        data_queue.pop();
        return res;
    }
    bool empty() const
    {
        std::lock_guard<std::mutex> lk(mut);
        return data_queue.empty();
    }
};

void test_safe_que()
{
    threadsafe_queue<int> safe_que;
    std::mutex mtx_print;
    std::thread producer([&](){
        for (int i = 0;;i++)
        {
            safe_que.push(i);
            {
                std::lock_guard<std::mutex> printlk(mtx_print);
                std::cout << "producer push data is " << i << std::endl;
            }
            std::this_thread::sleep_for(std::chrono::microseconds (200));
        }
    });

    std::thread consumer1([&](){
        for (;;)
        {
            auto data = safe_que.wait_and_pop();
            {
                std::lock_guard<std::mutex> printlk(mtx_print);
                std::cout << "consumer1 wait and pop data is " << *data << std::endl;
            }
            std::this_thread::sleep_for(std::chrono::microseconds(500));
        }
    });

    std::thread consumer2([&](){
        for(;;)
        {
            auto data = safe_que.try_pop();
            if (data != nullptr)
            {
                std::lock_guard<std::mutex> printlk(mtx_print);
                std::cout << "consumer2 try_pop data is " << *data << std::endl;
            }
            std::this_thread::sleep_for(std::chrono::microseconds(500));
        }
    });

    producer.join();
    consumer1.join();
    consumer2.join();
}

int main() {
    //PoorImplemention();
    //ResonableImplemention();
    test_safe_que();
    return 0;
}