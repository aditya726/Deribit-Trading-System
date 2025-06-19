#pragma once

#include <string>
#include <functional>
#include <map>
#include <memory>
#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/client.hpp>
#include <nlohmann/json.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl/context.hpp>


using json = nlohmann::json;
using WebsocketClient = websocketpp::client<websocketpp::config::asio_tls_client>;

class DeribitClient {
public:
    using MessageHandler = std::function<void(const json&)>;

    DeribitClient(const std::string& client_id, const std::string& client_secret);
    ~DeribitClient();

    bool connect();
    void disconnect();
    bool is_connected() const;

    void authenticate();
    void subscribe(const std::string& channel);
    void unsubscribe(const std::string& channel);

    void place_order(const std::string& instrument, double amount, double price,
        const std::string& type, const std::string& side);
    void cancel_order(const std::string& order_id);
    void modify_order(const std::string& order_id, double amount, double price);
    void get_orderbook(const std::string& instrument, int depth = 10);
    void get_positions(const std::string& currency = "", const std::string& kind = "");

    void register_message_handler(const std::string& channel, MessageHandler handler);

private:
    std::string client_id_;
    std::string client_secret_;
    std::string access_token_;

    std::unique_ptr<WebsocketClient> client_;
    websocketpp::connection_hdl connection_;

    std::map<std::string, MessageHandler> message_handlers_;
    std::map<int, std::function<void(const json&)>> response_handlers_;
    int request_id_ = 0;
    using TlsInitHandler = websocketpp::lib::function<std::shared_ptr<boost::asio::ssl::context>(websocketpp::connection_hdl)>;

    void on_open(websocketpp::connection_hdl hdl);
    void on_message(websocketpp::connection_hdl hdl, WebsocketClient::message_ptr msg);
    void on_close(websocketpp::connection_hdl hdl);
    void on_fail(websocketpp::connection_hdl hdl);

    void send_request(const std::string& method, const json& params,
        std::function<void(const json&)> handler = nullptr);

    std::string generate_nonce() const;
};