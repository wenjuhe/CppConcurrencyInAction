#include <iostream>
#include <vector>
#include <thread>


void thread_work(std::string str) {
    std::cout << "str is " << str << std::endl;
}

class background_task{
public:
    void operator()(){
        std::cout << "background_task called" << std::endl;
    }
};


/*
 * todo C++ 结构体默认所以成员都是公有的.
 * */
struct func{
    int& _i;
    /*
     * todo 引用类型的成员变量 通过初始化列表来初始化.
     * */
    func(int& i) : _i(i){}
    //todo 实现仿函数
    void operator()()
    {
        for(int i = 0; i < 3; i++) {
            _i = i;
            std::cout << "_i is " << _i << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
};

void oops() {
    int some_local_state = 0;
    func my_func(some_local_state);
    std::thread my_thread(my_func);
    //todo 隐患: 访问局部变量,局部变量可能会随着}结束而回收或随着主线程退出而回收
    my_thread.detach();
}

void use_join () {
    int some_local_state = 0;
    func myfunc(some_local_state);
    std::thread functhread(myfunc);
    functhread.join();
}

void catch_exception() {
    int some_local_state = 0;
    func myfunc(some_local_state);
    std::thread functhread{myfunc};
    try{
        //todo 本线程做一些事情
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }catch (std::exception& e) {
        functhread.join();
        throw;
    }
    functhread.join();
}

/*
 * todo RAII
 * */
class thread_guard{
private:
    std::thread& _t;
public:
    explicit thread_guard(std::thread& t) : _t(t){}
    ~thread_guard(){
        //todo join只能调用一次
        if (_t.joinable()) {
            _t.join();
        }
    }

    thread_guard(thread_guard const&) = delete;
    thread_guard& operator=(thread_guard const&) = delete;
};

void auto_guard(){
    int some_local_state = 0;
    func my_func(some_local_state);
    std::thread t(my_func);
    thread_guard g(t);
    //todo 本线程做一些事情
    std::cout << "auto guard finished" << std::endl;
}

void print_str(int i,std::string const& s) {
    std::cout << "i is " << i << "str is " << s << std::endl;
}

void danger_oops(int some_param) {
    char buffer[1024];
    sprintf(buffer,"%i",some_param);
    /*
     * 在线程内部将char const* 转化为std::string
     * 指针常量 char const*  指针本身不能变
     * 常量指针 const char*  指向的内容不能变
     * */
    std::thread t(print_str,3,buffer); //todo 局部变量的指针传递给线程的回调函数
    t.detach();
    std::cout << "danger oops finished " << std::endl;
}

void safe_oops(int some_param) {
    char buffer[1024];
    sprintf(buffer,"%i",some_param);
    std::thread t(print_str,3,std::string(buffer));
    t.detach();
}

void change_param(int& param) {
    param++;
}

void ref_oops(int some_param) {
    std::cout << "before change,param is" << some_param << std::endl;
    //需要使用引用显示转换
    //std::thread t2(change_param,some_param);
    std::thread t2(change_param,std::ref(some_param));
    t2.join();
    std::cout << "after change,param is" << some_param << std::endl;
}

class X{
public:
    void do_lengthy_work() {
        std::cout << "do_lengthy_work" << std::endl;
    }
};

//todo 绑定类成员函数
void bind_class_opps() {
    X my_x;
    std::thread t(&X::do_lengthy_work,&my_x);
    t.join();
}

void deal_unique(std::unique_ptr<int> p) {
    std::cout << "unique ptr data is" << *p << std::endl;
    (*p)++;
    std::cout << "after unique ptr data is " << *p << std::endl;
}

void move_oops() {
    auto p = std::make_unique<int>(100);
    std::thread t(deal_unique,std::move(p));
    t.join();
    //todo 不能再使用p了,p已经被move废弃
    //std::cout << "after unique ptr data is " << *p << std::endl;
}


void some_function() {
    while(true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

void some_other_function() {
    while(true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

void dangerous_use() {
    //1. t1 绑定some_function
    std::thread t1(some_function);

    //2. 转移t1管理的线程给t2,转移后t1无效,不绑定线程.
    std::thread t2 = std::move(t1);

    //3. t1可继续绑定其他线程,执行some_other_function
    /*
     * todo std::thread返回的是局部变量,并且是一个右值. 调用std::thread的移动赋值.
     * */
    t1 = std::thread(some_other_function);

    //4. 创建一个线程变量t3
    std::thread t3;

    //5. 转移t2管理的线程给t3
    t3 = std::move(t2);

    //6. 转移t3管理的线程给t1
    t1 = std::move(t3);
    std::this_thread::sleep_for(std::chrono::seconds(2000));
}

std::thread f(){
    return std::thread(some_function);
}



int main() {
    std::string hellostr = "hello world";
    /*
     * todo 1. 通过()初始化并启动一个线程
     * */
    std::thread t(thread_work,hellostr);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    /*
     * todo 2. 主线程等待子线程退出,如果注释掉下面的语句,字符串仍然会打印,但是会报错
     * */
    t.join(); //主线程等待子线程执行完毕!


    /*
     * todo 3 t2被当作函数对象的定义,其类型为返回std::thread,参数为background_task
     * */
    //std::thread t2(background_task());
    //t2.join();

    //todo 可多加一层()
    std::thread t2((background_task()));
    t2.join();

    //todo 可使用{}方式初始化
    std::thread t3{background_task()};
    t3.join();

    /*
     * todo 4 lambda表达式
     * */
    std::thread t4([](std::string str){
        std::cout << "str is " << str << std::endl;
    },hellostr);

    t4.join();

    /*
     * todo 5 detach注意事项
     * */
    oops();
    std::this_thread::sleep_for(std::chrono::seconds(1));

    /*
     * todo 6 join 用法
     * */
    use_join();


    /*
     * todo 7 捕获异常
     * */
    std::cout << "catch_exception" << std::endl;
    catch_exception();

    /*
     * todo 8 自动守卫
     * */
    std::cout << "auto_guard" << std::endl;
    auto_guard();

    //todo 可能存在崩溃
    danger_oops(100);
    std::this_thread::sleep_for(std::chrono::seconds(1));


    return 0;
}

