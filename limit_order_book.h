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
};

struct Book {
    Limit *buy_tree;
    Limit *sell_tree;
    Limit *lowest_sell;
    Limit *highest_buy;
};

#endif // LIMIT_ORDER_BOOK_H