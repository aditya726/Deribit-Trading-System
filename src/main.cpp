#include "deribit_client.h"
#include "websocket_server.h"
#include "order_manager.h"
#include "market_data.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <nlohmann/json.hpp>
#include <atomic>
#include <mutex>
#include <condition_variable>

using json = nlohmann::json;

// Global variables for controlling the application flow
std::atomic<bool> running(true);
std::mutex orderbook_mutex;
std::condition_variable orderbook_cv;
bool orderbook_received = false;
OrderBook latest_orderbook;
std::string latest_instrument;

// Forward declarations
void print_menu();
void handle_menu_choice(int choice, DeribitClient& deribit_client, OrderManager& order_manager, MarketData& market_data);
void print_orderbook(const OrderBook& orderbook, const std::string& instrument);
void handle_place_order(DeribitClient& deribit_client, OrderManager& order_manager);
void handle_cancel_order(DeribitClient& deribit_client);
void handle_modify_order(DeribitClient& deribit_client);
void handle_get_positions(DeribitClient& deribit_client);
void handle_get_orderbook(DeribitClient& deribit_client, MarketData& market_data);
void handle_subscribe_instrument(DeribitClient& deribit_client, MarketData& market_data);
void handle_unsubscribe_instrument(DeribitClient& deribit_client, MarketData& market_data);

int main() {
    // Configuration
    const std::string client_id = "client_id";
    const std::string client_secret = "cleint_secret_key";
    const int websocket_port = 9002;

    // Path to SSL certificates
    const std::string cert_file = "PATH";
    const std::string key_file = "PATh";

    // Initialize components
    DeribitClient deribit_client(client_id, client_secret);
    WebSocketServer websocket_server(websocket_port, cert_file, key_file);
    OrderManager order_manager;
    MarketData market_data;

    // Connect to Deribit
    std::cout << "Connecting to Deribit..." << std::endl;
    if (!deribit_client.connect()) {
        std::cerr << "Failed to connect to Deribit" << std::endl;
        return 1;
    }

    // Wait for authentication
    std::cout << "Authenticating..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // Start WebSocket server in a separate thread
    std::thread server_thread([&websocket_server]() {
        websocket_server.start();
        });
    server_thread.detach();

    // Register message handlers for orderbook updates
    auto register_orderbook_handler = [&](const std::string& instrument) {
        std::string channel = "book." + instrument + ".raw";
        deribit_client.register_message_handler(channel, [&](const json& msg) {
            if (msg.contains("params") && msg["params"].contains("data")) {
                json data = msg["params"]["data"];
                OrderBook orderbook;
                orderbook.timestamp = data["timestamp"].get<int64_t>();

                for (const auto& bid : data["bids"]) {
                    orderbook.bids.push_back({ bid[0].get<double>(), bid[1].get<double>() });
                }
                for (const auto& ask : data["asks"]) {
                    orderbook.asks.push_back({ ask[0].get<double>(), ask[1].get<double>() });
                }

                market_data.update_orderbook(instrument, orderbook);

                // Store latest orderbook for display
                {
                    std::lock_guard<std::mutex> lock(orderbook_mutex);
                    latest_orderbook = orderbook;
                    latest_instrument = instrument;
                    orderbook_received = true;
                }
                orderbook_cv.notify_one();

                // Broadcast to WebSocket clients
                websocket_server.broadcast(msg.dump());
            }
            });
        };

    // Register handlers for common instruments
    register_orderbook_handler("BTC-PERPETUAL");
    register_orderbook_handler("ETH-PERPETUAL");

    // Main menu loop
    int choice = 0;
    do {
        print_menu();
        std::cout << "Enter your choice: ";
        std::cin >> choice;

        // Clear input buffer
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        handle_menu_choice(choice, deribit_client, order_manager, market_data);

    } while (choice != 0 && running);

    // Clean up
    deribit_client.disconnect();
    websocket_server.stop();

    std::cout << "Application terminated." << std::endl;
    return 0;
}

void print_menu() {
    std::cout << "\n========== DERIBIT TRADING SYSTEM ==========\n";
    std::cout << "1. Place Order\n";
    std::cout << "2. Cancel Order\n";
    std::cout << "3. Modify Order\n";
    std::cout << "4. Get Positions\n";
    std::cout << "5. Get Orderbook\n";
    std::cout << "6. Subscribe to Instrument\n";
    std::cout << "7. Unsubscribe from Instrument\n";
    std::cout << "8. List All Orders\n";
    std::cout << "9. List Subscribed Instruments\n";
    std::cout << "0. Exit\n";
    std::cout << "============================================\n";
}

void handle_menu_choice(int choice, DeribitClient& deribit_client,
    OrderManager& order_manager, MarketData& market_data) {
    switch (choice) {
    case 0:
        running = false;
        break;
    case 1:
        handle_place_order(deribit_client,order_manager);
        break;
    case 2:
        handle_cancel_order(deribit_client);
        break;
    case 3:
        handle_modify_order(deribit_client);
        break;
    case 4:
        handle_get_positions(deribit_client);
        break;
    case 5:
        handle_get_orderbook(deribit_client, market_data);
        break;
    case 6:
        handle_subscribe_instrument(deribit_client, market_data);
        break;
    case 7:
        handle_unsubscribe_instrument(deribit_client, market_data);
        break;
    case 8: {
        std::vector<Order> orders = order_manager.get_orders();
        std::cout << "\nAll Orders (" << orders.size() << "):\n";
        for (const auto& order : orders) {
            std::cout << "ID: " << order.id
                << ", Instrument: " << order.instrument
                << ", Side: " << order.side
                << ", Type: " << order.type
                << ", Price: " << order.price
                << ", Amount: " << order.amount
                << ", Status: " << order.status << std::endl;
        }
        if (orders.empty()) {
            std::cout << "No orders found.\n";
        }
        break;
    }
    case 9: {
        std::vector<std::string> instruments = market_data.get_subscribed_instruments();
        std::cout << "\nSubscribed Instruments (" << instruments.size() << "):\n";
        for (const auto& instrument : instruments) {
            std::cout << "- " << instrument << std::endl;
        }
        if (instruments.empty()) {
            std::cout << "No subscriptions found.\n";
        }
        break;
    }
    default:
        std::cout << "Invalid choice. Please try again." << std::endl;
    }
}

void handle_place_order(DeribitClient& deribit_client, OrderManager& order_manager) {
    std::string instrument, type, side;
    double amount, price;

    std::cout << "Enter instrument (e.g., BTC-PERPETUAL): ";
    std::getline(std::cin, instrument);

    std::cout << "Enter type (limit/market): ";
    std::getline(std::cin, type);

    std::cout << "Enter side (buy/sell): ";
    std::getline(std::cin, side);

    std::cout << "Enter amount: ";
    std::cin >> amount;

    price = 0;
    if (type == "limit") {
        std::cout << "Enter price: ";
        std::cin >> price;
    }

    std::cout << "Placing order..." << std::endl;
    deribit_client.place_order(instrument, amount, price, type, side);

    // Create a local order record and add it to the order manager
    Order new_order;
    new_order.id = "completed" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count()); // Temporary ID
    new_order.instrument = instrument;
    new_order.side = side;
    new_order.type = type;
    new_order.amount = amount;
    new_order.price = price;
    new_order.filled = 0;
    new_order.status = "successfull";
    new_order.timestamp = std::chrono::system_clock::now().time_since_epoch().count();

    // Add the order to the order manager
    order_manager.add_order(new_order);
    std::cout << "Order added to local tracking with temporary ID: " << new_order.id << std::endl;
}

void handle_cancel_order(DeribitClient& deribit_client) {
    std::string order_id;

    std::cout << "Enter order ID to cancel: ";
    std::getline(std::cin, order_id);

    std::cout << "Cancelling order " << order_id << "..." << std::endl;
    deribit_client.cancel_order(order_id);
}

void handle_modify_order(DeribitClient& deribit_client) {
    std::string order_id;
    double amount, price;

    std::cout << "Enter order ID to modify: ";
    std::getline(std::cin, order_id);

    std::cout << "Enter new amount: ";
    std::cin >> amount;

    std::cout << "Enter new price: ";
    std::cin >> price;

    std::cout << "Modifying order " << order_id << "..." << std::endl;
    deribit_client.modify_order(order_id, amount, price);
}

void handle_get_positions(DeribitClient& deribit_client) {
    std::string currency, kind;
    int choice;

    std::cout << "\nGet Positions Options:\n";
    std::cout << "1. All Positions\n";
    std::cout << "2. BTC Positions\n";
    std::cout << "3. ETH Positions\n";
    std::cout << "4. Futures Positions\n";
    std::cout << "5. Options Positions\n";
    std::cout << "6. Custom Query\n";
    std::cout << "Enter your choice: ";
    std::cin >> choice;
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    switch (choice) {
    case 1:
        std::cout << "Getting all positions..." << std::endl;
        deribit_client.get_positions();
        break;
    case 2:
        std::cout << "Getting BTC positions..." << std::endl;
        deribit_client.get_positions("BTC", "");
        break;
    case 3:
        std::cout << "Getting ETH positions..." << std::endl;
        deribit_client.get_positions("ETH", "");
        break;
    case 4:
        std::cout << "Getting futures positions..." << std::endl;
        deribit_client.get_positions("", "future");
        break;
    case 5:
        std::cout << "Getting options positions..." << std::endl;
        deribit_client.get_positions("", "option");
        break;
    case 6:
        std::cout << "Enter currency (BTC/ETH/leave empty for all): ";
        std::getline(std::cin, currency);

        std::cout << "Enter instrument type (future/option/leave empty for all): ";
        std::getline(std::cin, kind);

        std::cout << "Getting positions for specified criteria..." << std::endl;
        deribit_client.get_positions(currency, kind);
        break;
    default:
        std::cout << "Invalid choice. Getting all positions instead." << std::endl;
        deribit_client.get_positions();
    }
}

void print_orderbook(const OrderBook& orderbook, const std::string& instrument) {
    std::cout << "\n===== ORDERBOOK: " << instrument << " =====\n";
    std::cout << "Timestamp: " << orderbook.timestamp << "\n\n";

    std::cout << std::setw(20) << "BIDS" << std::setw(20) << "ASKS" << "\n";
    std::cout << std::setw(10) << "Price" << std::setw(10) << "Size"
        << std::setw(10) << "Price" << std::setw(10) << "Size" << "\n";
    std::cout << "----------------------------------------\n";

    size_t max_entries = std::max(orderbook.bids.size(), orderbook.asks.size());
    max_entries = std::min(max_entries, size_t(10)); // Show at most 10 levels

    for (size_t i = 0; i < max_entries; i++) {
        // Print bid
        if (i < orderbook.bids.size()) {
            std::cout << std::setw(10) << orderbook.bids[i].price
                << std::setw(10) << orderbook.bids[i].amount;
        }
        else {
            std::cout << std::setw(20) << " ";
        }

        // Print ask
        if (i < orderbook.asks.size()) {
            std::cout << std::setw(10) << orderbook.asks[i].price
                << std::setw(10) << orderbook.asks[i].amount;
        }

        std::cout << "\n";
    }
    std::cout << "========================================\n";
}

void handle_get_orderbook(DeribitClient& deribit_client, MarketData& market_data) {
    std::string instrument;
    int depth;

    std::cout << "Enter instrument (e.g., BTC-PERPETUAL): ";
    std::getline(std::cin, instrument);

    std::cout << "Enter depth (default: 10): ";
    std::string depth_str;
    std::getline(std::cin, depth_str);
    depth = depth_str.empty() ? 10 : std::stoi(depth_str);

    std::cout << "Getting orderbook for " << instrument << "..." << std::endl;

    // Reset orderbook flag
    {
        std::lock_guard<std::mutex> lock(orderbook_mutex);
        orderbook_received = false;
    }

    // Request orderbook
    deribit_client.get_orderbook(instrument, depth);

    // Wait for the response or timeout
    {
        std::unique_lock<std::mutex> lock(orderbook_mutex);
        if (orderbook_cv.wait_for(lock, std::chrono::seconds(5),
            [&] { return orderbook_received; })) {
            print_orderbook(latest_orderbook, instrument);
        }
        else {
            std::cout << "Timeout waiting for orderbook data." << std::endl;

            // Try to get from stored data
            OrderBook ob = market_data.get_orderbook(instrument);
            if (!ob.bids.empty() || !ob.asks.empty()) {
                std::cout << "Using cached orderbook data:" << std::endl;
                print_orderbook(ob, instrument);
            }
        }
    }
}

void handle_subscribe_instrument(DeribitClient& deribit_client, MarketData& market_data) {
    std::string instrument;

    std::cout << "Enter instrument to subscribe (e.g., BTC-PERPETUAL): ";
    std::getline(std::cin, instrument);

    std::string channel = "book." + instrument + ".raw";
    std::cout << "Subscribing to " << channel << "..." << std::endl;

    market_data.subscribe_instrument(instrument);
    deribit_client.subscribe(channel);
}

void handle_unsubscribe_instrument(DeribitClient& deribit_client, MarketData& market_data) {
    std::string instrument;

    std::cout << "Enter instrument to unsubscribe (e.g., BTC-PERPETUAL): ";
    std::getline(std::cin, instrument);

    std::string channel = "book." + instrument + ".raw";
    std::cout << "Unsubscribing from " << channel << "..." << std::endl;

    market_data.unsubscribe_instrument(instrument);
    deribit_client.unsubscribe(channel);
}