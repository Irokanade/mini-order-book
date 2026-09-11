#ifndef LIMIT_ORDER_BOOK_H
#define LIMIT_ORDER_BOOK_H

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <map>
#include <span>
#include <stdexcept>
#include <unordered_map>

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

// L2 market by price
struct L2MBP {
    uint32_t price;
    uint64_t volume;
    size_t order_count;
};

struct L3MBO {
    uint64_t id;
    uint64_t entry_time;
    uint32_t price;
    uint32_t shares;
};

class Book {
    std::map<uint32_t, Limit> buy_limits;
    std::map<uint32_t, Limit> sell_limits;
    std::unordered_map<uint64_t, Order> orders_map;

    [[nodiscard]] const Limit *find_limit(const uint32_t price, const Side side) const noexcept {
        const auto &limits_map = side == Side::Buy ? buy_limits : sell_limits;
        const auto it = limits_map.find(price);
        return it == limits_map.end() ? nullptr : &it->second;
    }

public:
    Book() = default;
    ~Book() = default;

    Book(const Book &) = delete;
    Book &operator=(const Book &) = delete;
    Book(Book &&) = delete;
    Book &operator=(Book &&) = delete;

    [[nodiscard]] bool bids_empty() const noexcept {
        return buy_limits.empty();
    }

    [[nodiscard]] bool asks_empty() const noexcept {
        return sell_limits.empty();
    }

    [[nodiscard]] L2MBP get_best_bid() const noexcept {
        const auto &[price, limit] = *buy_limits.rbegin();
        return {price, limit.total_volume, limit.size};
    }

    [[nodiscard]] L2MBP get_best_ask() const noexcept {
        const auto &[price, limit] = *sell_limits.begin();
        return {price, limit.total_volume, limit.size};
    }

    [[nodiscard]] size_t get_bid_depth(const size_t n, const std::span<L2MBP> out) const noexcept {
        size_t i = 0;
        for (auto it = buy_limits.rbegin(); it != buy_limits.rend() && i < n; ++it) {
            out[i++] = {it->first, it->second.total_volume, it->second.size};
        }

        return i;
    }

    [[nodiscard]] size_t get_ask_depth(const size_t n, const std::span<L2MBP> out) const noexcept {
        size_t i = 0;
        for (auto it = sell_limits.begin(); it != sell_limits.end() && i < n; ++it) {
            out[i++] = {it->first, it->second.total_volume, it->second.size};
        }

        return i;
    }

    [[nodiscard]] size_t get_orders_at(const uint32_t price, const Side side, const size_t n,
                       const std::span<L3MBO> out) const noexcept {
        const Limit *limit = find_limit(price, side);
        if (limit == nullptr) {
            return 0;
        }

        size_t i = 0;
        for (const Order *order = limit->front(); order != &limit->sentinel && i < n;
             order = order->next_order) {
            out[i++] = {order->id_number, order->entry_time, price, order->shares};
        }

        return i;
    }

    [[nodiscard]] uint64_t get_volume_at(const uint32_t price, const Side side) const noexcept {
        const Limit *limit = find_limit(price, side);
        return limit == nullptr ? 0 : limit->total_volume;
    }

    [[nodiscard]] size_t get_order_count_at(const uint32_t price, const Side side) const noexcept {
        const Limit *limit = find_limit(price, side);
        return limit == nullptr ? 0 : limit->size;
    }

    void add_order(const uint64_t id, const Side side, const uint32_t shares,
                   const uint32_t limit_price, const uint64_t entry_time) {
        auto [it, inserted] = orders_map.try_emplace(id, id, side, shares, entry_time);
        if (!inserted) {
            throw std::runtime_error("duplicate order id");
        }
        Order *order_ptr = &it->second;

        auto &limits_map = side == Side::Buy ? buy_limits : sell_limits;

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
        add_order(new_id, side, shares, limit_price, entry_time);
    }

    void execute_order(const uint64_t id, const uint32_t shares, const uint64_t event_time) {
        Order &order = orders_map.at(id);
        if (shares > order.shares) {
            throw std::runtime_error("execution exceeds remaining shares");
        }

        if (shares == order.shares) {
            delete_order(id);
        } else {
            order.parent_limit->reduce(order, shares);
            order.event_time = event_time;
        }
    }
};

#endif // LIMIT_ORDER_BOOK_H