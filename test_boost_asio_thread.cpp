#include <boost/asio.hpp>
#include <iostream>

std::mutex m;

void print_task(int n) {
    std::lock_guard<std::mutex> lc{ m };
    std::cout << "Task " << n << " is running on thr: " <<
        std::this_thread::get_id() << '\n';
}

int main() {
    boost::asio::thread_pool pool{ 4 }; // 创建一个包含 4 个线程的线程池

    for (int i = 0; i < 10; ++i) {
        boost::asio::post(pool, [i] { print_task(i); });
    }

    pool.join(); // 等待所有任务执行完成
}