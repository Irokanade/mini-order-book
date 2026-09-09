#ifndef LIMIT_ORDER_BOOK_H
#define LIMIT_ORDER_BOOK_H

#include <cstdint>
#include <map>
#include <unordered_map>
#include <optional>

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
    int size = 0;
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

    [[nodiscard]] std::optional<uint32_t> get_best_bid() const noexcept {
        if (buy_limits.empty()) {
            return std::nullopt;
        }
        return buy_limits.rbegin()->first;
    }

    [[nodiscard]] std::optional<uint32_t> get_best_ask() const noexcept {
        if (sell_limits.empty()) {
            return std::nullopt;
        }
        return sell_limits.begin()->first;
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

    void execute_order(uint64_t id, uint32_t shares, uint64_t event_time);
    void execute_order_at(uint64_t id, uint32_t shares, uint32_t exec_price, uint64_t event_time);
    void cancel_order(uint64_t id, uint32_t shares, uint64_t event_time);
    void delete_order(uint64_t id, uint64_t event_time);
    void replace_order(uint64_t old_id, uint64_t new_id, uint32_t shares, uint32_t limit_price, uint64_t entry_time);
};

#endif // LIMIT_ORDER_BOOK_H