
#define _WEBSOCKETPP_CPP11_TYPE_TRAITS_ 1
#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/client.hpp>
#include "deribit_client.h"
#include "utils.h"
#include <websocketpp/common/thread.hpp>
#include <websocketpp/common/memory.hpp>
#include <iostream>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <openssl/sha.h>
#include <openssl/hmac.h>
#include <boost/asio.hpp>
#include <boost/asio/ssl/context.hpp>

DeribitClient::DeribitClient(const std::string& client_id, const std::string& client_secret)
    : client_id_(client_id), client_secret_(client_secret) {

    client_ = std::make_unique<WebsocketClient>();

    client_->clear_access_channels(websocketpp::log::alevel::all);
    client_->set_access_channels(websocketpp::log::alevel::connect |
        websocketpp::log::alevel::disconnect |
        websocketpp::log::alevel::app);

    client_->init_asio();

    client_->set_open_handler(std::bind(&DeribitClient::on_open, this, std::placeholders::_1));
    client_->set_message_handler(std::bind(&DeribitClient::on_message, this, std::placeholders::_1, std::placeholders::_2));
    client_->set_close_handler(std::bind(&DeribitClient::on_close, this, std::placeholders::_1));
    client_->set_fail_handler(std::bind(&DeribitClient::on_fail, this, std::placeholders::_1));

    client_->set_tls_init_handler([](websocketpp::connection_hdl hdl) {
        namespace asio = websocketpp::lib::asio;

        auto ctx = std::make_shared<asio::ssl::context>(asio::ssl::context::tlsv12_client);

        try {
            // Modern TLS settings
            ctx->set_options(
                asio::ssl::context::default_workarounds |
                asio::ssl::context::no_sslv2 |
                asio::ssl::context::no_sslv3 |
                asio::ssl::context::single_dh_use
            );

            // Load system CA certificates
            ctx->set_default_verify_paths();

            // Don't verify the certificate - this is for testing only
            // In production, you should properly verify the cert
            ctx->set_verify_mode(asio::ssl::verify_none);
        }
        catch (const std::exception& e) {
            std::cerr << "TLS init error: " << e.what() << std::endl;
        }

        return ctx;
        });
}

DeribitClient::~DeribitClient() {
    disconnect();
}

bool DeribitClient::connect() {
    try {
        websocketpp::lib::error_code ec;
        // Try using the production URL instead
        auto con = client_->get_connection("wss://test.deribit.com/ws/api/v2", ec);

        // If you still want to use test, use this
        // auto con = client_->get_connection("wss://test.deribit.com/ws/api/v2", ec);

        if (ec) {
            std::cerr << "Connection error: " << ec.message() << std::endl;
            return false;
        }

        connection_ = con->get_handle();
        client_->connect(con);

        // Start the ASIO io_service run loop in a separate thread
        websocketpp::lib::thread asio_thread(&WebsocketClient::run, client_.get());
        asio_thread.detach();
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Exception in connect: " << e.what() << std::endl;
        return false;
    }
}

void DeribitClient::disconnect() {
    if (is_connected()) {
        websocketpp::lib::error_code ec;
        client_->close(connection_, websocketpp::close::status::going_away, "", ec);
        if (ec) {
            std::cerr << "Error closing connection: " << ec.message() << std::endl;
        }
    }
}

bool DeribitClient::is_connected() const {
    return client_->get_con_from_hdl(connection_)->get_state() == websocketpp::session::state::open;
}

void DeribitClient::authenticate() {
    json params = {
        {"grant_type", "client_credentials"},
        {"client_id", client_id_},
        {"client_secret", client_secret_}
    };

    // Debug output to verify parameters are being sent correctly
    std::cout << "Authentication parameters: " << params.dump() << std::endl;

    send_request("public/auth", params, [this](const json& response) {
        std::cout << "Auth response: " << response.dump() << std::endl;
        if (response.contains("result") && response["result"].contains("access_token")) {
            access_token_ = response["result"]["access_token"].get<std::string>();
            std::cout << "Authentication successful" << std::endl;
        } else if (response.contains("error")) {
            std::cerr << "Authentication failed: " << response.dump() << std::endl;
            // Try re-authenticating with different format
            if (response["error"]["message"].get<std::string>().find("Invalid params") != std::string::npos) {
                std::cout << "Retrying authentication with alternative format..." << std::endl;
                
                // Alternative authentication format
                json alt_params = {
                    {"grant_type", "client_credentials"},
                    {"client_id", client_id_},
                    {"client_secret", client_secret_}
                };
                
                // Make sure all fields are properly included
                send_request("public/auth", alt_params, [this](const json& alt_response) {
                    if (alt_response.contains("result") && alt_response["result"].contains("access_token")) {
                        access_token_ = alt_response["result"]["access_token"].get<std::string>();
                        std::cout << "Authentication successful with alternative format" << std::endl;
                    } else {
                        std::cerr << "Authentication failed after retry: " << alt_response.dump() << std::endl;
                    }
                });
            }
        }
    });
}

void DeribitClient::subscribe(const std::string& channel) {
    json params = {
        {"channels", {channel}}
    };

    send_request("public/subscribe", params);
}

void DeribitClient::unsubscribe(const std::string& channel) {
    json params = {
        {"channels", {channel}}
    };

    send_request("public/unsubscribe", params);
}

void DeribitClient::place_order(const std::string& instrument, double amount, double price,
    const std::string& type, const std::string& side) {
    json params = {
        {"instrument_name", instrument},
        {"amount", amount},
        {"type", type},
        {"side", side}
    };

    if (price > 0) {
        params["price"] = price;
    }

    send_request("private/" + side, params, [](const json& response) {
        std::cout << "Order response: " << response.dump() << std::endl;
        });
}

void DeribitClient::cancel_order(const std::string& order_id) {
    json params = {
        {"order_id", order_id}
    };

    send_request("private/cancel", params, [](const json& response) {
        std::cout << "Cancel response: " << response.dump() << std::endl;
        });
}

void DeribitClient::modify_order(const std::string& order_id, double amount, double price) {
    json params = {
        {"order_id", order_id},
        {"amount", amount},
        {"price", price}
    };

    send_request("private/edit", params, [](const json& response) {
        std::cout << "Modify response: " << response.dump() << std::endl;
        });
}

void DeribitClient::get_orderbook(const std::string& instrument, int depth) {
    json params = {
        {"instrument_name", instrument},
        {"depth", depth}
    };

    send_request("public/get_order_book", params, [](const json& response) {
        std::cout << "Orderbook: " << response.dump(2) << std::endl;
        });
}

void DeribitClient::get_positions(const std::string& currency, const std::string& kind) {
    json params = {};

    if (!currency.empty()) {
        params["currency"] = currency;
    }

    if (!kind.empty()) {
        params["kind"] = kind;
    }

    std::string requestDescription = "Positions";
    if (!currency.empty()) {
        requestDescription = currency + " " + requestDescription;
    }
    if (!kind.empty()) {
        requestDescription = kind + " " + requestDescription;
    }

    send_request("private/get_positions", params, [requestDescription](const json& response) {
        std::cout << requestDescription << ": " << response.dump(2) << std::endl;
        });
}

void DeribitClient::register_message_handler(const std::string& channel, MessageHandler handler) {
    message_handlers_[channel] = handler;
}

void DeribitClient::on_open(websocketpp::connection_hdl hdl) {
    std::cout << "Connected to Deribit" << std::endl;
    authenticate();
}

void DeribitClient::on_message(websocketpp::connection_hdl hdl, WebsocketClient::message_ptr msg) {
    try {
        json response = json::parse(msg->get_payload());

        if (response.contains("id")) {
            int id = response["id"].get<int>();
            auto it = response_handlers_.find(id);
            if (it != response_handlers_.end()) {
                it->second(response);
                response_handlers_.erase(it);
            }
        }
        else if (response.contains("params")) {
            json params = response["params"];
            if (params.contains("channel")) {
                std::string channel = params["channel"].get<std::string>();
                auto it = message_handlers_.find(channel);
                if (it != message_handlers_.end()) {
                    it->second(response);
                }
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error processing message: " << e.what() << std::endl;
    }
}

void DeribitClient::on_close(websocketpp::connection_hdl hdl) {
    std::cout << "Disconnected from Deribit" << std::endl;
}

void DeribitClient::on_fail(websocketpp::connection_hdl hdl) {
    std::cerr << "Connection failed" << std::endl;
}

void DeribitClient::send_request(const std::string& method, const json& params,
    std::function<void(const json&)> handler) {
    if (!is_connected()) {
        std::cerr << "Not connected to Deribit" << std::endl;
        return;
    }

    json request = {
        {"jsonrpc", "2.0"},
        {"id", ++request_id_},
        {"method", method},
        {"params", params}
    };

    if (handler) {
        response_handlers_[request_id_] = handler;
    }

    try {
        client_->send(connection_, request.dump(), websocketpp::frame::opcode::text);
    }
    catch (const std::exception& e) {
        std::cerr << "Error sending request: " << e.what() << std::endl;
    }
}

std::string DeribitClient::generate_nonce() const {
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();

    std::ostringstream oss;
    oss << millis;
    return oss.str();
}