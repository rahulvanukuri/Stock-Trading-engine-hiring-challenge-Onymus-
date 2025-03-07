#include <iostream>
#include <vector>
#include <thread>
#include <atomic>
#include <algorithm>
#include <cstdio>

using namespace std;

struct Order {
    enum Type { BUY, SELL } type;
    string ticker;
    int quantity;
    int price;
    atomic<Order*> next;

    Order(Type type, string ticker, int quantity, int price)
        : type(type), ticker(ticker), quantity(quantity), price(price), next(nullptr) {}
};

class OrderBook {
private:
    vector<Order*> buyOrders;
    vector<Order*> sellOrders;
    atomic<bool> lock;

    void sortOrders() {
        sort(buyOrders.begin(), buyOrders.end(), [](Order* a, Order* b) { return a->price > b->price; });
        sort(sellOrders.begin(), sellOrders.end(), [](Order* a, Order* b) { return a->price < b->price; });
    }

public:
    OrderBook() : lock(false) {}

    void addOrder(Order::Type type, const string& ticker, int quantity, int price) {
        Order* newOrder = new Order(type, ticker, quantity, price);
        while (lock.exchange(true, memory_order_acquire));

        if (type == Order::BUY)
            buyOrders.push_back(newOrder);
        else
            sellOrders.push_back(newOrder);
        
        sortOrders();
        lock.store(false, memory_order_release);
    }

    void matchOrders() {
        while (lock.exchange(true, memory_order_acquire));
        
        size_t i = 0, j = 0;
        while (i < buyOrders.size() && j < sellOrders.size()) {
            if (buyOrders[i]->price >= sellOrders[j]->price) {
                int matchedQuantity = min(buyOrders[i]->quantity, sellOrders[j]->quantity);
                printf("Matched %d shares of %s at price %d\n", matchedQuantity, buyOrders[i]->ticker.c_str(), sellOrders[j]->price);
                
                buyOrders[i]->quantity -= matchedQuantity;
                sellOrders[j]->quantity -= matchedQuantity;
                
                if (buyOrders[i]->quantity == 0) i++;
                if (sellOrders[j]->quantity == 0) j++;
            } else {
                break;
            }
        }
        
        lock.store(false, memory_order_release);
    }
};

void simulateTrading(OrderBook& ob) {
    vector<thread> traders;
    for (int i = 0; i < 10; i++) {
        traders.emplace_back([&ob, i]() {
            for (int j = 0; j < 10; j++) {
                ob.addOrder((i % 2 == 0) ? Order::BUY : Order::SELL, "AAPL", 10 + j, 100 + (i * 2));
            }
        });
    }
    for (auto& t : traders) t.join();
    ob.matchOrders();
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    OrderBook ob;
    simulateTrading(ob);
    return 0;
}
