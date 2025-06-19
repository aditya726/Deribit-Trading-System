#pragma once

#include <string>
#include <map>
#include <vector>
#include <mutex>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

struct OrderBookEntry {
    double price;
    double amount;
};

struct OrderBook {
    std::vector<OrderBookEntry> bids;
    std::vector<OrderBookEntry> asks;
    int64_t timestamp;
};

class MarketData {
public:
    MarketData();

    void update_orderbook(const std::string& instrument, const OrderBook& orderbook);
    OrderBook get_orderbook(const std::string& instrument) const;

    void subscribe_instrument(const std::string& instrument);
    void unsubscribe_instrument(const std::string& instrument);
    std::vector<std::string> get_subscribed_instruments() const;

private:
    mutable std::mutex orderbooks_mutex_;
    std::map<std::string, OrderBook> orderbooks_;

    mutable std::mutex subscriptions_mutex_;
    std::vector<std::string> subscribed_instruments_;
};