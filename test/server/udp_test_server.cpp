// Copyright (c) 2003-2025 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <array>
#include <boost/asio.hpp>
#include <functional>
#include <iostream>
#include <memory>
#include <vector>

using boost::asio::ip::udp;

class udp_server {
   public:
    udp_server(boost::asio::io_context& io_context)
        : socket_(io_context, udp::endpoint(udp::v4(), 13)) {
        start_receive();
    }

   private:
    void start_receive() {
        socket_.async_receive_from(boost::asio::buffer(recv_buffer_),
                                   remote_endpoint_,
                                   std::bind(&udp_server::handle_receive,
                                             this,
                                             boost::asio::placeholders::error,
                                             boost::asio::placeholders::bytes_transferred));
    }

    void handle_receive(const boost::system::error_code& error, std::size_t bytes_transferred) {
        if (!error) {
            auto message = std::make_shared<std::vector<uint8_t>>(
                recv_buffer_.begin(), recv_buffer_.begin() + bytes_transferred);

            socket_.async_send_to(boost::asio::buffer(*message),
                                  remote_endpoint_,
                                  std::bind(&udp_server::handle_send,
                                            this,
                                            message,
                                            boost::asio::placeholders::error,
                                            boost::asio::placeholders::bytes_transferred));

            start_receive();
        }
    }

    void handle_send(std::shared_ptr<std::vector<uint8_t>> /*message*/,
                     const boost::system::error_code& /*error*/,
                     std::size_t /*bytes_transferred*/) {}

    udp::socket socket_;
    udp::endpoint remote_endpoint_;
    static constexpr std::size_t kMaxDatagramSize = 65507;
    std::array<uint8_t, kMaxDatagramSize> recv_buffer_;
};

int main() {
    try {
        boost::asio::io_context io_context;
        udp_server server(io_context);
        io_context.run();
        std::cout << "server listening on 13\n";
    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
    }

    return 0;
}
