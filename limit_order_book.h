#ifndef LIMIT_ORDER_BOOK_H
#define LIMIT_ORDER_BOOK_H

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <map>
#include <unordered_map>
#include <vector>

enum class Side : bool { Buy, Sell };

struct Limit;

struct Order {
    Order *next_order = nullptr;
    Order *prev_order = nullptr;
    Limit *parent_limit = nullptr;
    uint64_t id_number = 0;
    uint32_t shares = 0;
    uint64_t entry_time = 0;
    uint64_t event_time = 0;
    Side buy_or_sell = Side::Buy;

    Order() = default;

    Order(const uint64_t id, const Side buy, const uint32_t shares_, const uint64_t entry_t)
        : id_number(id), shares(shares_),
          entry_time(entry_t), event_time(entry_t), buy_or_sell(buy) {}
};

struct Limit {
    Order sentinel;
    uint32_t limit_price = 0;
    size_t size = 0;
    uint64_t total_volume = 0;

    explicit Limit(const uint32_t _limit_price) : limit_price(_limit_price) {
        sentinel.next_order = &sentinel;
        sentinel.prev_order = &sentinel;
    }

    ~Limit() = default;

    Limit(const Limit &) = delete;
    Limit &operator=(const Limit &) = delete;
    Limit(Limit &&) = delete;
    Limit &operator=(Limit &&) = delete;

    [[nodiscard]] bool empty() const noexcept {
        return sentinel.next_order == &sentinel;
    }

    [[nodiscard]] Order *front() const noexcept {
        return sentinel.next_order;
    }

    [[nodiscard]] Order *back() const noexcept {
        return sentinel.prev_order;
    }

    void append(Order *order) noexcept {
        order->next_order = &sentinel;
        order->prev_order = sentinel.prev_order;
        sentinel.prev_order->next_order = order;
        sentinel.prev_order = order;
        size++;
        total_volume += order->shares;
        order->parent_limit = this;
    }

    void remove(Order *order) noexcept {
        Order *prev_order = order->prev_order;
        Order *next_order = order->next_order;
        prev_order->next_order = next_order;
        next_order->prev_order = prev_order;
        size--;
        total_volume -= order->shares;
        order->parent_limit = nullptr;
    }

    void reduce(Order &order, const uint32_t shares) noexcept {
        order.shares -= shares;
        total_volume -= shares;
    }
};

struct DepthLevel {
    uint32_t price;
    uint64_t volume;
    size_t order_count;
};

class Book {
    std::map<uint32_t, Limit> buy_limits;
    std::map<uint32_t, Limit> sell_limits;
    std::unordered_map<uint64_t, Order> orders_map;

public:
    Book() = default;
    ~Book() = default;

    Book(const Book &) = delete;
    Book &operator=(const Book &) = delete;
    Book(Book &&) = delete;
    Book &operator=(Book &&) = delete;

    [[nodiscard]] bool buy_limits_empty() const noexcept {
        return buy_limits.empty();
    }

    [[nodiscard]] bool sell_limits_empty() const noexcept {
        return sell_limits.empty();
    }

    [[nodiscard]] uint32_t get_best_bid() const noexcept {
        return buy_limits.rbegin()->first;
    }

    [[nodiscard]] uint32_t get_best_ask() const noexcept {
        return sell_limits.begin()->first;
    }

    [[nodiscard]] uint64_t get_best_bid_size() const noexcept {
        return buy_limits.rbegin()->second.total_volume;
    }

    [[nodiscard]] uint64_t get_best_ask_size() const noexcept {
        return sell_limits.begin()->second.total_volume;
    }

    [[nodiscard]] size_t get_best_bid_order_count() const noexcept {
        return buy_limits.rbegin()->second.size;
    }

    [[nodiscard]] size_t get_best_ask_order_count() const noexcept {
        return sell_limits.begin()->second.size;
    }

    [[nodiscard]] std::vector<DepthLevel> get_bid_depth(const size_t n) const {
        std::vector<DepthLevel> depth;
        depth.reserve(n < buy_limits.size() ? n : buy_limits.size());

        for (auto it = buy_limits.rbegin(); it != buy_limits.rend() && depth.size() < n; ++it) {
            depth.push_back({it->first, it->second.total_volume, it->second.size});
        }

        return depth;
    }

    [[nodiscard]] std::vector<DepthLevel> get_ask_depth(const size_t n) const {
        std::vector<DepthLevel> depth;
        depth.reserve(n < sell_limits.size() ? n : sell_limits.size());

        for (auto it = sell_limits.begin(); it != sell_limits.end() && depth.size() < n; ++it) {
            depth.push_back({it->first, it->second.total_volume, it->second.size});
        }

        return depth;
    }

    template<Side side>
    void add_order(uint64_t id, uint32_t shares, uint32_t limit_price, uint64_t entry_time) {
        auto [it, inserted] = orders_map.try_emplace(id, id, side, shares, entry_time);
        Order *order_ptr = &it->second;

        auto &limits_map = [this]() -> std::map<uint32_t, Limit>& {
            if constexpr (side == Side::Buy) {
                return buy_limits;
            } else {
                return sell_limits;
            }
        }();

        auto [limit_it, limit_inserted] = limits_map.try_emplace(limit_price, limit_price);
        limit_it->second.append(order_ptr);
    }

    void delete_order(const uint64_t id) {
        Order &order = orders_map.at(id);
        Limit *limit = order.parent_limit;
        limit->remove(&order);

        if (limit->empty()) {
            auto &limits_map = order.buy_or_sell == Side::Buy ? buy_limits : sell_limits;
            limits_map.erase(limit->limit_price);
        }

        orders_map.erase(id);
    }

    void cancel_order(const uint64_t id, const uint32_t shares, const uint64_t event_time) {
        Order &order = orders_map.at(id);

        if (shares == order.shares) {
            delete_order(id);
        } else {
            order.parent_limit->reduce(order, shares);
            order.event_time = event_time;
        }
    }

    void replace_order(const uint64_t old_id, const uint64_t new_id, const uint32_t shares,
                        const uint32_t limit_price, const uint64_t entry_time) {
        const Side side = orders_map.at(old_id).buy_or_sell;
        delete_order(old_id);

        if (side == Side::Buy) {
            add_order<Side::Buy>(new_id, shares, limit_price, entry_time);
        } else {
            add_order<Side::Sell>(new_id, shares, limit_price, entry_time);
        }
    }

    void execute_order(const uint64_t id, const uint32_t shares, const uint64_t event_time) {
        Order &order = orders_map.at(id);

        if (shares == order.shares) {
            delete_order(id);
        } else {
            order.parent_limit->reduce(order, shares);
            order.event_time = event_time;
        }
    }
};

#endif // LIMIT_ORDER_BOOK_H