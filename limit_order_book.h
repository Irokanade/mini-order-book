#ifndef LIMIT_ORDER_BOOK_H
#define LIMIT_ORDER_BOOK_H

#include <ranges>
#include <unordered_map>

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
    bool buy_or_sell = false;

    Order() = default;

    Order(const int id, const bool buy, const int shares_, const int limit_price, const int entry_t)
        : id_number(id), shares(shares_), limit(limit_price),
          entry_time(entry_t), event_time(entry_t), buy_or_sell(buy) {}
};

struct Limit {
    Limit *parent = nullptr;
    Limit *left_child = nullptr;
    Limit *right_child = nullptr;
    Order *head_order = nullptr;
    Order *tail_order = nullptr;
    int limit_price = 0;
    int size = 0;
    int total_volume = 0;

    explicit Limit(const int _limit_price) :
            head_order(new Order), tail_order(new Order),
            limit_price(_limit_price) {
        head_order->next_order = tail_order;
        tail_order->prev_order = head_order;
    }

    ~Limit() {
        // delete sentinels
        delete head_order;
        delete tail_order;
    }

    Limit(const Limit &) = delete;
    Limit &operator=(const Limit &) = delete;
    Limit(Limit &&) = delete;
    Limit &operator=(Limit &&) = delete;

    void append(Order *order) {
        order->next_order = tail_order;
        order->prev_order = tail_order->prev_order;

        tail_order->prev_order->next_order = order;
        tail_order->prev_order = order;

        size++;
        total_volume += order->shares;
        order->parent_limit = this;
    }

    void remove(Order *order) {
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