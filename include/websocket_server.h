#pragma once
#include <websocketpp/config/asio.hpp>
#include <websocketpp/server.hpp>
#include <set>
#include <mutex>
#include <functional>
#include <string>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
// Using TLS-enabled configuration
using WebsocketServer = websocketpp::server<websocketpp::config::asio_tls>;

class WebSocketServer {
public:
    using MessageHandler = std::function<void(const std::string&, websocketpp::connection_hdl)>;

    WebSocketServer(int port, const std::string& cert_file, const std::string& key_file);
    ~WebSocketServer();

    void start();
    void stop();
    void broadcast(const std::string& message);
    void send(websocketpp::connection_hdl hdl, const std::string& message);
    void register_message_handler(MessageHandler handler);

private:
    int port_;
    WebsocketServer server_;
    std::set<websocketpp::connection_hdl, std::owner_less<websocketpp::connection_hdl>> connections_;
    std::mutex connections_mutex_;
    MessageHandler message_handler_;

    void on_open(websocketpp::connection_hdl hdl);
    void on_close(websocketpp::connection_hdl hdl);
    void on_message(websocketpp::connection_hdl hdl, WebsocketServer::message_ptr msg);

    // TLS context setup
    std::string cert_file_;
    std::string key_file_;
    // Using connection_hdl in the signature for TLS init handler
    std::shared_ptr<websocketpp::lib::asio::ssl::context> on_tls_init(websocketpp::connection_hdl hdl);
};