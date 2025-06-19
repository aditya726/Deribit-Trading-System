#pragma once

#include <string>
#include <map>
#include <vector>
#include <mutex>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

struct Order {
    std::string id;
    std::string instrument;
    std::string side;
    std::string type;
    double amount;
    double price;
    double filled;
    std::string status;
    int64_t timestamp;
};

class OrderManager {
public:
    OrderManager();

    void add_order(const Order& order);
    void update_order(const std::string& id, const json& update);
    void remove_order(const std::string& id);

    Order get_order(const std::string& id) const;
    std::vector<Order> get_orders() const;
    std::vector<Order> get_orders_for_instrument(const std::string& instrument) const;

private:
    mutable std::mutex orders_mutex_;
    std::map<std::string, Order> orders_;
};