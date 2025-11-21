#include <iostream>
#include <memory>
#include <boost/asio.hpp>

using namespace boost::asio;
using namespace boost::asio::ip;
using namespace std;

//会话类，处理单个客户端连接
class Session : public enable_shared_from_this<Session> {

public:
    Session(tcp::socket socket) : socket_(std::move(socket)) {}

    void start() {
        read();
    }
    
private:
    // 读取数据
    void read() {
        auto self(shared_from_this());
        socket_.async_read_some(buffer(buffer_),
            [this, self](boost::system::error_code ec, size_t length) {
            if (!ec) {
                std::cout << "Received: " << std::string(self->buffer_, length) << "\n";
                //回显数据
                write(length);
            }
            else {
                std::cerr << "Read error: " << ec.message() << std::endl;
            }
        });
    }

    // 写入数据
    void write(size_t length) {
        auto self(shared_from_this());
        async_write(socket_, buffer(buffer_, length),
            [this, self](boost::system::error_code ec, size_t /*length*/) {
            if (!ec) {
                // 继续读取数据
                read();
            }
            else {
                std::cerr << "Write error: " << ec.message() << std::endl;
            }
        });
    }

    tcp::socket socket_;
    char buffer_[1024];
};

//服务器类
class Server {

public:
    Server(io_context& io_context, unsigned short port) : acceptor_(io_context, tcp::endpoint(tcp::v4(), port)), socket_(io_context) {
        start_accept();
    }

private:
    // 接受新连接
    void start_accept() {
        acceptor_.async_accept(socket_,
            [this](boost::system::error_code ec) {
            if (!ec) {
                // 创建会话对象并启动
                auto session = make_shared<Session>(std::move(socket_));
                session->start();
            }
            else {
                std::cerr << "Accept error: " << ec.message() << std::endl;
            }

            // 继续接受新连接
            start_accept();
        });
    }

    tcp::acceptor acceptor_;
    tcp::socket socket_;
};

int main() {
    try {
        io_context io_context;
        Server server(io_context, 8888);
        std::cout << "Async Server started, listening on port 8888..." << std::endl;

        // 运行io_context
        io_context.run();
    }
    catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
    return 0;
}

