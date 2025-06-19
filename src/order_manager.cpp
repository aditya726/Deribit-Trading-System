#include "order_manager.h"

OrderManager::OrderManager() {}

void OrderManager::add_order(const Order& order) {
    std::lock_guard<std::mutex> lock(orders_mutex_);
    orders_[order.id] = order;
}

void OrderManager::update_order(const std::string& id, const json& update) {
    std::lock_guard<std::mutex> lock(orders_mutex_);

    auto it = orders_.find(id);
    if (it != orders_.end()) {
        if (update.contains("amount")) it->second.amount = update["amount"];
        if (update.contains("price")) it->second.price = update["price"];
        if (update.contains("filled")) it->second.filled = update["filled"];
        if (update.contains("status")) it->second.status = update["status"];
    }
}

void OrderManager::remove_order(const std::string& id) {
    std::lock_guard<std::mutex> lock(orders_mutex_);
    orders_.erase(id);
}

Order OrderManager::get_order(const std::string& id) const {
    std::lock_guard<std::mutex> lock(orders_mutex_);
    auto it = orders_.find(id);
    if (it != orders_.end()) {
        return it->second;
    }
    return {};
}

std::vector<Order> OrderManager::get_orders() const {
    std::lock_guard<std::mutex> lock(orders_mutex_);
    std::vector<Order> result;
    for (const auto& pair : orders_) {
        result.push_back(pair.second);
    }
    return result;
}

std::vector<Order> OrderManager::get_orders_for_instrument(const std::string& instrument) const {
    std::lock_guard<std::mutex> lock(orders_mutex_);
    std::vector<Order> result;
    for (const auto& pair : orders_) {
        if (pair.second.instrument == instrument) {
            result.push_back(pair.second);
        }
    }
    return result;
}