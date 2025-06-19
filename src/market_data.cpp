#include "market_data.h"

MarketData::MarketData() {}

void MarketData::update_orderbook(const std::string& instrument, const OrderBook& orderbook) {
    std::lock_guard<std::mutex> lock(orderbooks_mutex_);
    orderbooks_[instrument] = orderbook;
}

OrderBook MarketData::get_orderbook(const std::string& instrument) const {
    std::lock_guard<std::mutex> lock(orderbooks_mutex_);
    auto it = orderbooks_.find(instrument);
    if (it != orderbooks_.end()) {
        return it->second;
    }
    return {};
}

void MarketData::subscribe_instrument(const std::string& instrument) {
    std::lock_guard<std::mutex> lock(subscriptions_mutex_);
    if (std::find(subscribed_instruments_.begin(), subscribed_instruments_.end(), instrument) == subscribed_instruments_.end()) {
        subscribed_instruments_.push_back(instrument);
    }
}

void MarketData::unsubscribe_instrument(const std::string& instrument) {
    std::lock_guard<std::mutex> lock(subscriptions_mutex_);
    subscribed_instruments_.erase(
        std::remove(subscribed_instruments_.begin(), subscribed_instruments_.end(), instrument),
        subscribed_instruments_.end()
    );
}

std::vector<std::string> MarketData::get_subscribed_instruments() const {
    std::lock_guard<std::mutex> lock(subscriptions_mutex_);
    return subscribed_instruments_;
}