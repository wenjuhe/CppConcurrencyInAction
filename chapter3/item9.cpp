#include <iostream>
#include <thread>
#include <mutex>


/*
 * todo 假设这是一个很复杂的数据结构,假设我们不建议执行拷贝构造
 * */
class some_big_object
{
private:
    int _data;
public:
    some_big_object(int data) : _data(data){}
    //拷贝构造
    some_big_object(const some_big_object& b2) : _data(b2._data)
    {
        _data = b2._data;
    }
    //移动构造
    some_big_object(some_big_object&& b2) : _data(std::move(b2._data)){}

    //重载输出运算符
    friend std::ostream& operator << (std::ostream& os,const some_big_object& big_obj)
    {
        os << big_obj._data;
        return os;
    }

    //重载赋值运算符
    some_big_object& operator = (const some_big_object& b2)
    {
        if (this == &b2)
        {
            return *this;
        }
        _data = b2._data;
        return *this;
    }

    //交换数据
    friend void swap(some_big_object& b1,some_big_object& b2)
    {
        some_big_object temp = std::move(b1);
        b1 = std::move(b2);
        b2 = std::move(b1);
    }
};


class big_object_mgr
{
private:
    some_big_object _obj;
    std::mutex _mtx;
public:
    big_object_mgr(int data = 0) : _obj(data){}
    void printinfo()
    {
        std::cout << "current obj data is " << _obj << std::endl;
    }

    friend void danger_swap(big_object_mgr& objm1,big_object_mgr& objm2);
    friend void safe_swap(big_object_mgr& objm1,big_object_mgr& objm2);
    friend void danger_swap_scope(big_object_mgr& objm1,big_object_mgr& objm2);
};

void danger_swap(big_object_mgr& objm1,big_object_mgr& objm2)
{
    std::cout << "thread [ " << std::this_thread::get_id() << " ] begin" << std::endl;
    if (&objm1 == &objm2)
    {
        return;
    }
    std::lock_guard<std::mutex> guard1(objm1._mtx);
    //此处为了故意制造死锁,让线程小睡一会
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::lock_guard<std::mutex> guard2(objm2._mtx);
    std::swap(objm1._obj,objm2._obj);
    std::cout << "thread [ " <<std::this_thread::get_id() << "] end" << std::endl;
}

void test_dnager_swap()
{
    big_object_mgr objm1(5);
    big_object_mgr objm2(100);

    std::thread t1(danger_swap,std::ref(objm1),std::ref(objm2));
    std::thread t2(danger_swap,std::ref(objm2),std::ref(objm1));

    t1.join();
    t2.join();
    objm1.printinfo();
    objm2.printinfo();
}

void safe_swap(big_object_mgr& objm1,big_object_mgr& objm2)
{
    std::cout << "thread [ " << std::this_thread::get_id() << " ] begin" << std::endl;
    if (&objm1 == &objm2)
    {
        return;
    }

    std::lock(objm1._mtx,objm2._mtx);
    //todo 领养锁管理互斥量解锁.
    std::lock_guard<std::mutex> guard1(objm1._mtx,std::adopt_lock); //todo 管理锁的释放.
    //此处为了故意制造死锁,让线程小睡一会
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::lock_guard<std::mutex> guard2(objm2._mtx,std::adopt_lock);
    std::swap(objm1._obj,objm2._obj);
    std::cout << "thread [ " <<std::this_thread::get_id() << "] end" << std::endl;
}

void test_safe_swap()
{
    big_object_mgr objm1(5);
    big_object_mgr objm2(100);

    std::thread t1(safe_swap,std::ref(objm1),std::ref(objm2));
    std::thread t2(safe_swap,std::ref(objm2),std::ref(objm1));

    t1.join();
    t2.join();

    objm1.printinfo();
    objm2.printinfo();
}


//todo 上述代码可以简化为以下方式
void safe_swap_scope(big_object_mgr& objm1,big_object_mgr& objm2)
{
    std::cout << "thread [ " << std::this_thread::get_id() << " ] begin" << std::endl;
    if (&objm1 == &objm2)
    {
        return;
    }
}


int main()
{
    //test_dnager_swap();
    test_safe_swap();
    return 0;
}