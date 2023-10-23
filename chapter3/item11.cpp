#include <iostream>
#include <thread>
#include <mutex>
#include  <climits>
#include <map>
#include <shared_mutex>

/*
 * todo
 * */
std::mutex mtx;
int shared_data = 0;

/*
 * todo unique_lock 基本用法
 * */
void use_unique()
{
    //lock可自动解锁,也可手动解锁
    std::unique_lock<std::mutex> lock(mtx);
    std::cout <<"lock success" << std::endl;
    shared_data++;
    lock.unlock();
}

/*
 * todo 可判断是否占有锁
 * */
void own_lock()
{
    std::unique_lock<std::mutex> lock(mtx);
    shared_data++;
    if (lock.owns_lock())
    {
        std::cout << "owns lock" << std::endl;
    }
    else
    {
        std::cout << "doesn't own lock" << std::endl;
    }
    lock.unlock();
    if (lock.owns_lock())
    {
        std::cout << "owns lock" << std::endl;
    }
    else
    {
        std::cout << "doesn't own lock" << std::endl;
    }
}

/*
 * todo 可以延迟加锁  golang 关键字 defer
 * */
void defer_lock()
{
    //延迟加锁
    std::unique_lock<std::mutex> lock(mtx,std::defer_lock);
    //可以加锁
    lock.lock();
    //可以自动析构解锁,也可以手动解锁
    lock.unlock();
}


/*
 * todo 同时使用owns和defer
 * */

void use_own_defer()
{
    std::unique_lock<std::mutex> lock(mtx);
    //判断是否拥有锁
    if (lock.owns_lock())
    {
        std::cout << "Main thread has the lock." << std::endl;
    }
    else
    {
        std::cout << "Main thread does not have the lock." << std::endl;
    }
    std::thread t([](){
        std::unique_lock<std::mutex> lock(mtx,std::defer_lock);
        // 判断是否拥有锁
        if (lock.owns_lock())
        {
            std::cout << "Thread has the lock." << std::endl;
        }
        else
        {
            std::cout << "Thread does not have the lock." << std::endl;
        }
        // 加锁
        lock.lock();
        // 判断是否拥有锁
        if (lock.owns_lock())
        {
            std::cout << "Thread has the lock." << std::endl;
        }
        else
        {
            std::cout << "Thread does not have the lock." << std::endl;
        }
        // 解锁
        lock.unlock();
    });
    t.join();
}

/*
 * todo 同样支持领养操作
 * */

void use_own_adopt()
{
    /*
     * todo 如果注释掉下面的 mtx.lock() 会报错.
     *      奇怪: 视频运行报错了,但是我这里没报错.
     * */
    //mtx.lock();
    std::unique_lock<std::mutex> lock(mtx,std::adopt_lock);
    if (lock.owns_lock()) {
        std::cout << "owns lock" << std::endl;
    }
    else {
        std::cout << "does not have the lock" << std::endl;
    }
    lock.unlock();
}

/*
 * todo 之前的交换代码可以可以用如下方式等价实现
 * */
int a = 10;
int b = 99;
std::mutex mtx1;
std::mutex mtx2;

/*
 * todo 一旦mutex被unique_lock管理,加锁和释放的操作就交给unique_lock,不能调用mutex加锁和解锁,因为锁的使用权已经交给unique_lock了
 * */
void safe_swap()
{
    std::lock(mtx1,mtx2);
    std::unique_lock<std::mutex> lock1(mtx1,std::adopt_lock);
    std::unique_lock<std::mutex> lock2(mtx2,std::adopt_lock);
    std::swap(a,b);
    /*
     * todo 错误用法 视频报错,但是我运行没问题. ???
     * */
    //mtx1.unlock();
    //mtx2.unlock();
    /*
     * todo 正确用法
     * */
    lock1.unlock();
    lock2.unlock();
}

void safe_swap2()
{
    std::unique_lock<std::mutex> lock1(mtx1,std::adopt_lock);
    std::unique_lock<std::mutex> lock2(mtx2,std::adopt_lock);
    //todo 需用lock1,lock2加锁
    //std::lock(lock1,lock2);
    //todo 错误用法
    std::lock(mtx1,mtx2);
    std::swap(a,b);
}


/*
 * todo 转移互斥量所有权  互斥量本身不支持move操作,但是unique_lock支持
 *      unique_lock支持移动构造,移动赋值,不支持拷贝构造,拷贝赋值! 好处:
 *      Cpp将一个局部变量返回给外部,外部调用者会执行该局部变量的拷贝构造,如果没有拷贝构造和拷贝赋值,那么编译器就会找到移动构造和移动赋值
 * */

std::unique_lock<std::mutex> get_lock()
{
    std::unique_lock<std::mutex> lock(mtx);
    shared_data++;
    return lock;
}

void use_return()
{
    std::unique_lock<std::mutex> lock(get_lock());
    shared_data++;
}


//锁粒度表示加锁的精细程度。
//一个锁的粒度要足够大,以保证可以锁住要访问的共享数据
// 一个锁的粒度要足够小,以保证非共享的数据不被锁住影响性能
void precision_lock()
{
    std::unique_lock<std::mutex> lock(mtx);
    shared_data++;
    lock.unlock();
    //不涉及共享数据的耗时操作不要放在锁内执行
    std::this_thread::sleep_for(std::chrono::seconds(1));
    lock.lock();
    shared_data++;
}

/*
 * todo
 *  C++ 17 标准shared mutex
 *  C++14 提供了 shared time mutex
 *  C++11 无上述互斥，想使用可以利用boost库
 * */


/*class DNService {
public:
    DNService() {}
    //读操作采用共享锁
    std::string QueryDNS(std::string dnsname) {
        std::shared_lock<std::shared_mutex> shared_locks(_shared_mtx);
        auto iter = _dns_info.find(dnsname);
        if (iter != _dns_info.end()) {
            return iter->second;
        }
        return "";
    }
    //写操作采用独占锁
    void AddDNSInfo(std::string dnsname, std::string dnsentry) {
        std::lock_guard<std::shared_mutex>  guard_locks(_shared_mtx);
        _dns_info.insert(std::make_pair(dnsname, dnsentry));
    }
private:
    std::map<std::string, std::string> _dns_info;
    mutable std::shared_mutex  _shared_mtx;
};*/

class RecursiveDemo
{
private:
    std::map<std::string,int> _students_info;
    std::recursive_mutex _recursive_mtx;
public:
    RecursiveDemo(){}

    bool QueryStudent(std::string name)
    {
        std::lock_guard<std::recursive_mutex> recursive_lock(_recursive_mtx);
        auto iter_find = _students_info.find(name);
        if (iter_find == _students_info.end()) {
            return false;
        }
        return true;
    }

    void AddScore(std::string name,int score)
    {
        std::lock_guard<std::recursive_mutex> recursive_lock(_recursive_mtx);
        if (!QueryStudent(name))
        {
            _students_info.insert(std::make_pair(name,score));
            return;
        }
        _students_info[name] = _students_info[name] + score;
    }

    /*
     * todo
     *  不推荐采用递归锁,使用递归锁说明设计思路并不理想,需优化设计
     *  推荐拆分逻辑,将共有逻辑拆分为统一接口
     * */

    void AddScoreAtomic(std::string name,int score)
    {
        std::lock_guard<std::recursive_mutex> recursive_lock(_recursive_mtx);
        auto iter_find = _students_info.find(name);
        if (iter_find == _students_info.end())
        {
            _students_info.insert(std::make_pair(name,score));
            return;
        }
        _students_info[name] = _students_info[name] + score;
        return;
    }
};

int main() {
    //use_unique();
    //own_lock();
    //defer_lock();
    //use_own_defer();
    //use_own_adopt();
    //safe_swap();
    //safe_swap2();
    //use_return();
    //precision_lock();
    return 0;
}