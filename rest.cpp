#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio.hpp>
#include <iostream>

namespace http = boost::beast::http;
namespace asio = boost::asio;
using tcp = asio::ip::tcp;

class RequestHandler {
public:
    void handleRequest(http::request<http::string_body>&& request, http::response<http::string_body>& response) {
        if (request.method() == http::verb::get && request.target() == "/api") {
            response.result(http::status::ok);
            response.set(http::field::server, "MyServer");
            response.set(http::field::content_type, "text/plain");
            response.body() = "Hello, World!";
            response.prepare_payload();
        } else {
            response.result(http::status::not_found);
            response.set(http::field::server, "MyServer");
            response.set(http::field::content_type, "text/plain");
            response.body() = "Not Found";
            response.prepare_payload();
        }
    }
};

class HttpServer {
public:
    explicit HttpServer(asio::io_context& ioContext, const tcp::endpoint& endpoint)
        : acceptor_(ioContext, endpoint)
    {
        startAccept();
    }

private:
    void startAccept() {
        acceptor_.async_accept(
            [this](std::error_code ec, tcp::socket socket) {
                if (!ec) {
                    std::make_shared<HttpSession>(std::move(socket), requestHandler_)->start();
                }
                startAccept();
            });
    }

    tcp::acceptor acceptor_;
    RequestHandler requestHandler_;
};

class HttpSession : public std::enable_shared_from_this<HttpSession> {
public:
    HttpSession(tcp::socket socket, RequestHandler& requestHandler)
        : socket_(std::move(socket))
        , requestHandler_(requestHandler)
    {
    }

    void start() {
        readRequest();
    }

private:
    void readRequest() {
        auto self = shared_from_this();
        http::async_read(socket_, buffer_, request_,
            [self](std::error_code ec, std::size_t bytes_transferred) {
                if (!ec) {
                    self->processRequest();
                }
            });
    }

    void processRequest() {
        response_.version(request_.version());
        response_.keep_alive(false);
        requestHandler_.handleRequest(std::move(request_), response_);
        writeResponse();
    }

    void writeResponse() {
        auto self = shared_from_this();
        http::async_write(socket_, response_,
            [self](std::error_code ec, std::size_t bytes_transferred) {
                self->socket_.shutdown(tcp::socket::shutdown_send, ec);
            });
    }

    tcp::socket socket_;
    http::request<http::string_body> request_;
    http::response<http::string_body> response_;
    asio::streambuf buffer_;
};

int main() {
    try {
        asio::io_context ioContext;
        const tcp::endpoint endpoint(tcp::v4(), 8080);
        HttpServer server(ioContext, endpoint);
        ioContext.run();
    } catch (const std::exception& ex) {
        std::cerr << "Exception: " << ex.what() << std::endl;
    }
    return 0;
}
