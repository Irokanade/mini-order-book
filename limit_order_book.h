#ifndef LIMIT_ORDER_BOOK_H
#define LIMIT_ORDER_BOOK_H

#include <ranges>
#include <unordered_map>

enum class Side : bool { Buy, Sell };

struct Limit;

struct Order {
    Order *next_order = nullptr;
    Order *prev_order = nullptr;
    Limit *parent_limit = nullptr;
    int id_number = 0;
    int shares = 0;
    int limit = 0;
    int entry_time = 0;
    int event_time = 0;
    Side buy_or_sell = Side::Buy;

    Order() = default;

    Order(const int id, const Side buy, const int shares_, const int limit_price, const int entry_t)
        : id_number(id), shares(shares_), limit(limit_price),
          entry_time(entry_t), event_time(entry_t), buy_or_sell(buy) {}
};

struct Limit {
    Order sentinel;
    Limit *parent = nullptr;
    Limit *left_child = nullptr;
    Limit *right_child = nullptr;
    int limit_price = 0;
    int size = 0;
    int total_volume = 0;

    explicit Limit(const int _limit_price) : limit_price(_limit_price) {
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
};

struct Book {
    Limit *buy_tree = nullptr;
    Limit *sell_tree = nullptr;
    Limit *lowest_sell = nullptr;
    Limit *highest_buy = nullptr;

    std::unordered_map<int, Order *> orders_map;
    std::unordered_map<int, Limit *> buy_limits_map;
    std::unordered_map<int, Limit *> sell_limits_map;

    Book() = default;

    ~Book() {
        for (const auto &order: orders_map | std::views::values) {
            delete order;
        }

        for (const auto &limit: buy_limits_map | std::views::values) {
            delete limit;
        }

        for (const auto &limit: sell_limits_map | std::views::values) {
            delete limit;
        }
    }

    Book(const Book &) = delete;
    Book &operator=(const Book &) = delete;
    Book(Book &&) = delete;
    Book &operator=(Book &&) = delete;
};

#endif // LIMIT_ORDER_BOOK_H