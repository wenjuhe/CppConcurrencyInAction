#include <iostream>
#include <thread>
#include <mutex>
#include  <climits>
/*
 * todo 层级锁 可以用来检测代码是否有造成死锁的可能!
 * */
class hierarchical_mutex
{
private:
    std::mutex _internal_mutex;
    //当前层级
    unsigned long const _hierarchy_value;
    //上一层级值
    unsigned long _previous_hierarchy_value;
    //本线程记录的层级值
    //todo static变量需要初始化.
    static thread_local unsigned long _this_thread_hierarchy_value;

    void check_for_hierarchy_violation()
    {
        if(_this_thread_hierarchy_value <= _hierarchy_value)
        {
            throw std::logic_error("mutex hierarchy violated");
        }
    }

    void update_hierarchy_value()
    {
        _previous_hierarchy_value = _this_thread_hierarchy_value;
        _this_thread_hierarchy_value = _hierarchy_value;
    }
public:
    explicit hierarchical_mutex(unsigned long value) : _hierarchy_value(value),_previous_hierarchy_value(0){}
    /*
     * todo 拷贝构造,拷贝赋值删除之后,可以支持移动吗?
     *  不支持! 因为没有定义移动构造,默认走拷贝构造,但是拷贝构造已经被删除了,
     *  所以该类不能被移动,也不能被复制.
     * */
    hierarchical_mutex(const hierarchical_mutex&) = delete;
    hierarchical_mutex& operator=(const hierarchical_mutex&) = delete;
    void lock()
    {
        check_for_hierarchy_violation();
        _internal_mutex.lock();
        update_hierarchy_value();
    }

    void unlock()
    {
        if (_this_thread_hierarchy_value != _hierarchy_value)
        {
            throw std::logic_error("mutex hierarchy violdated");
        }
        _this_thread_hierarchy_value = _previous_hierarchy_value;
        _internal_mutex.unlock();
    }

    bool try_lock()
    {
        check_for_hierarchy_violation();
        if (!_internal_mutex.try_lock())
        {
            return false;
        }
        update_hierarchy_value();
        return true;
    }
};

thread_local unsigned long hierarchical_mutex::_this_thread_hierarchy_value(ULONG_MAX);

void test_hierarchy_lock()
{
    hierarchical_mutex hmtx1(1000);
    hierarchical_mutex hmtx2(500);

    std::thread t1([&hmtx1,&hmtx2](){
        hmtx1.lock();
        hmtx2.lock();
        hmtx2.unlock();
        hmtx1.unlock();
    });

    std::thread t2([&hmtx1,&hmtx2](){
        hmtx2.lock();
        hmtx1.lock();
        hmtx1.unlock();
        hmtx2.unlock();
    });

    t1.join();
    t2.join();
}

int main() {
    test_hierarchy_lock();
    return 0;
}