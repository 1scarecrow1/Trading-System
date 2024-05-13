// File path: /src/order_book.cpp

#include <map>
#include <set>
#include <list>
#include <cmath>
#include <ctime>
#include <deque>
#include <queue>
#include <stack>
#include <string>
#include <bitset>
#include <cstdio>
#include <limits>
#include <vector>
#include <climits>
#include <cstring>
#include <cstdlib>
#include <fstream>
#include <numeric>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <unordered_map>

enum ORDERTYPE { IOC, GTD, GTC };
enum ACTION { CREATE, MODIFY, DELETE };

typedef unsigned int price_t;
typedef unsigned int quantity_t;
typedef unsigned int order_id_t;

struct Order{
    long timestamp;
    bool is_bid;
    order_id_t id;
    price_t price;
    quantity_t quantity;
    char venue[20];
    ORDERTYPE type;

    Order(
            long timestamp_,
            bool is_bid_,
            order_id_t id_,
            price_t price_,
            quantity_t quantity_,
            const char * venue_,
            ORDERTYPE type_
        )
        :
            timestamp(timestamp_),
            is_bid(is_bid_),
            id(id_),
            price(price_),
            quantity(quantity_),
            type(type_)
        {
            // Copy the venue using strcpy to ensure it's handled properly
            strcpy(this->venue, venue_);
        }
};

struct OrderAction{
    Order &order;
    ACTION action;
    OrderAction(Order& o, ACTION a):order(o),action(a){}
};

class OrderBook{
private:
    // Maps to maintain orders by id for efficient access
    std::unordered_map<order_id_t, Order> bids;
    std::unordered_map<order_id_t, Order> asks;

    // Function to find and display the best bid or ask
    void display_best(bool is_bid) {
        if (is_bid) {
            if (bids.empty()) {
                std::cout << "no bids" << std::endl;
                return;
            }
            auto max_bid = std::max_element(bids.begin(), bids.end(),
                            [](const auto& a, const auto& b) { return a.second.price < b.second.price; });
            display_best_bid(max_bid->second.price);
        } else {
            if (asks.empty()) {
                std::cout << "no asks" << std::endl;
                return;
            }
            auto min_ask = std::min_element(asks.begin(), asks.end(),
                            [](const auto& a, const auto& b) { return a.second.price > b.second.price; });
            display_best_ask(min_ask->second.price);
        }
    }

public:
    OrderBook(){};

    void add_order(Order &o);
    void modify_order(Order &o);
    void delete_order(order_id_t id);
    void process_order(OrderAction &o);
    void get_best_bid();
    void get_best_ask();
    void get_bid_levels();
    void get_ask_levels();
    void clear_book();
};

void OrderBook::add_order(Order &o) {
    if (o.type == ORDERTYPE::IOC) {
        display_error_IOC(o);
        return;
    }

    auto& map_ref = (o.is_bid) ? bids : asks;
    if (map_ref.count(o.id) > 0) {
        display_error_order_id_exist(o);
    } else {
        map_ref[o.id] = o;
        display_order_creation(o);
    }
}

void OrderBook::modify_order(Order &o) {
    auto& map_ref = (o.is_bid) ? bids : asks;
    auto it = map_ref.find(o.id);
    if (it == map_ref.end()) {
        display_error_exist_order_modification(o);
        return;
    }

    if (it->second.price != o.price) {
        display_error_amendment_price(o);
        return;
    }

    if (it->second.quantity <= o.quantity) {
        display_error_amendment_quantity(o);
        return;
    }

    it->second = o; // Update the order in the map
    display_amended_order(o);
}

void OrderBook::delete_order(order_id_t id) {
    // Find the map that potentially contains the order id
    // Since we don't have the order to know if it's a bid or ask, we have to check both maps
    bool found = false;
    if (bids.count(id) > 0) {
        bids.erase(id);
        found = true;
    } else if (asks.count(id) > 0) {
        asks.erase(id);
        found = true;
    }

    if (found) {
        Order dummy_order = {}; // Create a dummy order to use for displaying deletion message
        dummy_order.id = id;
        display_deleted_order(dummy_order);
    } else {
        Order dummy_order = {}; // Create a dummy order to use for displaying error message
        dummy_order.id = id;
        display_error_order_not_existing_deletion(dummy_order);
    }
}

void OrderBook::process_order(OrderAction &o) {
    switch (o.action) {
        case CREATE:
            add_order(o.order);
            break;
        case MODIFY:
            modify_order(o.order);
            break;
        case DELETE:
            delete_order(o.order.id);
            break;
    }
}

void OrderBook::get_best_bid() {
    display_best(true);
}

void OrderBook::get_best_ask() {
    display_best(false);
}

void OrderBook::clear_book() {
    int number_of_bids_deleted = bids.size();
    bids.clear();
    int number_of_asks_deleted = asks.size();
    asks.clear();
    std::cout << "Number of bids deleted:" << number_of_bids_deleted << std::endl;
    std::cout << "Number of asks deleted:" << number_of_asks_deleted << std::endl;
}


