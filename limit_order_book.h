#ifndef LIMIT_ORDER_BOOK_H
#define LIMIT_ORDER_BOOK_H

#include <map>
#include <unordered_map>
#include <optional>

enum class Side : bool { Buy, Sell };

struct Limit;

struct Order {
    Order *next_order = nullptr;
    Order *prev_order = nullptr;
    Limit *parent_limit = nullptr;
    int id_number = 0;
    int shares = 0;
    int entry_time = 0;
    int event_time = 0;
    Side buy_or_sell = Side::Buy;

    Order() = default;

    Order(const int id, const Side buy, const int shares_, const int entry_t)
        : id_number(id), shares(shares_),
          entry_time(entry_t), event_time(entry_t), buy_or_sell(buy) {}
};

struct Limit {
    Order sentinel;
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

    void reduce(Order &order, const int shares) noexcept {
        order.shares -= shares;
        total_volume -= shares;
    }
};

class Book {
    std::map<int, Limit> buy_limits;
    std::map<int, Limit> sell_limits;
    std::unordered_map<int, Order> orders_map;

public:
    Book() = default;
    ~Book() = default;

    Book(const Book &) = delete;
    Book &operator=(const Book &) = delete;
    Book(Book &&) = delete;
    Book &operator=(Book &&) = delete;

    [[nodiscard]] std::optional<int> get_best_bid() const noexcept {
        if (buy_limits.empty()) {
            return std::nullopt;
        }
        return buy_limits.rbegin()->first;
    }

    [[nodiscard]] std::optional<int> get_best_ask() const noexcept {
        if (sell_limits.empty()) {
            return std::nullopt;
        }
        return sell_limits.begin()->first;
    }

    void add_order(int id, Side side, int shares, int limit_price, int entry_time);
    void execute_order(int id, int shares);
    void execute_order_at(int id, int shares, int exec_price);
    void cancel_order(int id, int shares);
    void delete_order(int id);
    void replace_order(int old_id, int new_id, int shares, int limit_price, int entry_time);
};

#endif // LIMIT_ORDER_BOOK_H