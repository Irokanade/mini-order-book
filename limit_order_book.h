#ifndef LIMIT_ORDER_BOOK_H
#define LIMIT_ORDER_BOOK_H

struct Limit;

struct Order {
    Order *next_order;
    Order *prev_order;
    Limit *parent_limit;
    int id_number;
    int shares;
    int limit;
    int entry_time;
    int event_time;
    bool buy_or_sell;
};

struct Limit {
    Limit *parent;
    Limit *left_child;
    Limit *right_child;
    Order *head_order;
    Order *tail_order;
    int limit_price;
    int size;
    int total_volume;

    explicit Limit(const int _limit_price) :
            parent(nullptr), left_child(nullptr), right_child(nullptr),
            head_order(new Order), tail_order(new Order),
            limit_price(_limit_price), size(0), total_volume(0) {
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
    Limit *buy_tree;
    Limit *sell_tree;
    Limit *lowest_sell;
    Limit *highest_buy;

    Book() : buy_tree(nullptr), sell_tree(nullptr),
            lowest_sell(nullptr), highest_buy(nullptr) {}
};

#endif // LIMIT_ORDER_BOOK_H