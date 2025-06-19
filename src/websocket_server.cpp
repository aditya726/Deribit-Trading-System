#define _WEBSOCKETPP_CPP11_TYPE_TRAITS_ 1
#include "websocket_server.h"
#include <iostream>

WebSocketServer::WebSocketServer(int port, const std::string& cert_file, const std::string& key_file)
    : port_(port), cert_file_(cert_file), key_file_(key_file) {

    server_.init_asio();

    server_.set_open_handler(std::bind(&WebSocketServer::on_open, this, std::placeholders::_1));
    server_.set_close_handler(std::bind(&WebSocketServer::on_close, this, std::placeholders::_1));
    server_.set_message_handler(std::bind(&WebSocketServer::on_message, this,
        std::placeholders::_1, std::placeholders::_2));

    server_.clear_access_channels(websocketpp::log::alevel::all);
    server_.set_access_channels(websocketpp::log::alevel::connect |
        websocketpp::log::alevel::disconnect |
        websocketpp::log::alevel::app);

    // Set TLS handler correctly with connection_hdl parameter
    server_.set_tls_init_handler(std::bind(&WebSocketServer::on_tls_init, this, std::placeholders::_1));
}

WebSocketServer::~WebSocketServer() {
    stop();
}

// Fixed signature to include connection_hdl parameter
std::shared_ptr<websocketpp::lib::asio::ssl::context> WebSocketServer::on_tls_init(websocketpp::connection_hdl hdl) {
    // Use websocketpp's asio namespace
    namespace asio = websocketpp::lib::asio;

    auto ctx = std::make_shared<asio::ssl::context>(asio::ssl::context::tlsv12_server);

    try {
        // Set modern TLS options
        ctx->set_options(
            asio::ssl::context::default_workarounds |
            asio::ssl::context::no_sslv2 |
            asio::ssl::context::no_sslv3 |
            asio::ssl::context::no_tlsv1 |
            asio::ssl::context::no_tlsv1_1 |
            asio::ssl::context::single_dh_use
        );

        // Load certificate and private key
        ctx->use_certificate_file(cert_file_, asio::ssl::context::pem);
        ctx->use_private_key_file(key_file_, asio::ssl::context::pem);

        // For added security (optional)
        ctx->set_default_verify_paths();
    }
    catch (const std::exception& e) {
        std::cerr << "TLS init error: " << e.what() << std::endl;
    }

    return ctx;
}

void WebSocketServer::start() {
    try {
        server_.listen(port_);
        server_.start_accept();

        std::thread([this]() {
            try {
                server_.run();
            }
            catch (const std::exception& e) {
                std::cerr << "Server run error: " << e.what() << std::endl;
            }
            }).detach();

        std::cout << "Secure WebSocket server started on port " << port_ << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Error starting server: " << e.what() << std::endl;
    }
}

void WebSocketServer::stop() {
    server_.stop();
}

void WebSocketServer::broadcast(const std::string& message) {
    std::lock_guard<std::mutex> lock(connections_mutex_);
    for (auto hdl : connections_) {
        try {
            server_.send(hdl, message, websocketpp::frame::opcode::text);
        }
        catch (const std::exception& e) {
            std::cerr << "Error broadcasting message: " << e.what() << std::endl;
        }
    }
}

void WebSocketServer::send(websocketpp::connection_hdl hdl, const std::string& message) {
    try {
        server_.send(hdl, message, websocketpp::frame::opcode::text);
    }
    catch (const std::exception& e) {
        std::cerr << "Error sending message: " << e.what() << std::endl;
    }
}

void WebSocketServer::register_message_handler(MessageHandler handler) {
    message_handler_ = handler;
}

void WebSocketServer::on_open(websocketpp::connection_hdl hdl) {
    std::lock_guard<std::mutex> lock(connections_mutex_);
    connections_.insert(hdl);
    std::cout << "New secure connection established" << std::endl;
}

void WebSocketServer::on_close(websocketpp::connection_hdl hdl) {
    std::lock_guard<std::mutex> lock(connections_mutex_);
    connections_.erase(hdl);
    std::cout << "Connection closed" << std::endl;
}

void WebSocketServer::on_message(websocketpp::connection_hdl hdl, WebsocketServer::message_ptr msg) {
    if (message_handler_) {
        message_handler_(msg->get_payload(), hdl);
    }
}