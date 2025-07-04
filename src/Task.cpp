#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <iomanip>
#include <ctime>
#include <stdexcept>

using namespace std;

// Alias for shared_ptr<Product>
class Product;
using ProductPtr = shared_ptr<Product>;

// Interface for shippable items
class Shippable {
public:
    virtual string getName() const = 0;
    virtual double getWeight() const = 0;
    virtual ~Shippable() {}
};

// Base Product class
class Product {
protected:
    string name;
    double price;
    int quantity;
public:
    Product(const string& name, double price, int quantity)
        : name(name), price(price), quantity(quantity) {}

    virtual bool isExpired() const {return false;}
    virtual bool isShippable() const {return false;}

    string getName() const {return name;}
    double getPrice() const {return price;}
    int getQuantity() const {return quantity;}
    void reduceQuantity(int amount) { quantity -= amount;}

    virtual ~Product() {}
};

// Expirable product class
class ExpirableProduct : public Product {
    time_t expiryDate;
public:
    ExpirableProduct(const string& name, double price, int quantity, time_t expiryDate)
        : Product(name, price, quantity), expiryDate(expiryDate) {}

    bool isExpired() const override {return time(nullptr) > expiryDate;
    }
};

// Shippable product class
class ShippableProduct : public Product, public Shippable {
    double weight;
public:
    ShippableProduct(const string& name, double price, int quantity, double weight)
        : Product(name, price, quantity), weight(weight) {}

    bool isShippable() const override {return true;}
    string getName() const override {return name;}
    double getWeight() const override {return weight;}
};

// Expirable and shippable product
class ExpirableShippableProduct : public ExpirableProduct, public Shippable {
    double weight;
public:
    ExpirableShippableProduct(const string& name, double price, int quantity, time_t expiryDate, double weight)
        : ExpirableProduct(name, price, quantity, expiryDate), weight(weight) {}

    bool isShippable() const override {return true;}
    string getName() const override {return name;}
    double getWeight() const override {return weight;}
};

// Customer class
class Customer {
    string name;
    double balance;
public:
    Customer(const string& name, double balance) : name(name), balance(balance) {}
    double getBalance() const {return balance;}
    void pay(double amount) { balance -= amount;}
    void showBalance() const {
        cout << "Customer Balance: " << fixed << setprecision(2) << balance << '\n';
    }
};

// Cart item
struct CartItem {
    ProductPtr product;
    int quantity;
};

// Cart
class Cart {
    vector<CartItem> items;
public:
    void add(const ProductPtr& product, int quantity) {
        if (quantity > product->getQuantity()) {
            throw runtime_error("Not enough stock for: " + product->getName());
        }
        items.push_back({ product, quantity });
    }

    const vector<CartItem>& getItems() const {return items;}
    bool isEmpty() const {return items.empty();}
    void clear() { items.clear();}
};

// Shipping service
class ShippingService {
public:
    static void shipItems(const vector<pair<string, double>>& items) {
        cout << "** Shipment notice **\n";
        double totalWeight = 0;
        for (const auto& [desc, weight] : items) {
            cout << desc << '\n';
            totalWeight += weight;
        }
        cout << "Total package weight " << fixed << setprecision(1) << totalWeight << "kg\n\n";
    }
};

// Checkout
void checkout(Customer& customer, Cart& cart) {
    if (cart.isEmpty()) throw runtime_error("Cart is empty.");

    double subtotal = 0;
    double shippingCost = 0;
    vector<pair<string, double>> shippingItems;

    for (const auto& item : cart.getItems()) {
        auto& product = item.product;

        if (product->isExpired())
            throw runtime_error(product->getName() + " is expired.");

        if (item.quantity > product->getQuantity())
            throw runtime_error("Insufficient stock: " + product->getName());

        subtotal += product->getPrice() * item.quantity;

        if (product->isShippable()) {
            if (auto* s = dynamic_cast<Shippable*>(product.get())) {
                double weight = s->getWeight() * item.quantity;
                string desc = to_string(item.quantity) + "x " + s->getName() + "    " + to_string(int(weight * 1000)) + "g";
                shippingItems.emplace_back(desc, weight);
                shippingCost += weight * 10;
            }
        }
    }

    double total = subtotal + shippingCost;

    if (customer.getBalance() < total)
        throw runtime_error("Customer balance is insufficient.");

    if (!shippingItems.empty())
        ShippingService::shipItems(shippingItems);

    cout << "** Checkout receipt **\n";
    for (const auto& item : cart.getItems()) {
        cout << item.quantity << "x " << item.product->getName()
             << "    " << item.product->getPrice() * item.quantity << '\n';
        item.product->reduceQuantity(item.quantity);
    }

    cout << "----------------------\n";
    cout << "Subtotal         " << subtotal << '\n';
    cout << "Shipping         " << shippingCost << '\n';
    cout << "Amount           " << total << "\n\n";

    customer.pay(total);
    customer.showBalance();
    cart.clear();
}

int main() {
    time_t tomorrow = time(nullptr) + 86400;

    auto cheese = make_shared<ExpirableShippableProduct>("Cheese", 100, 10, tomorrow, 0.2);
    auto biscuits = make_shared<ExpirableShippableProduct>("Biscuits", 150, 5, tomorrow, 0.7);
    auto tv = make_shared<ShippableProduct>("TV", 5000, 3, 10.0);
    auto scratchCard = make_shared<Product>("Scratch Card", 50, 100);

    Customer customer("Ibrahim", 1000);
    Cart cart;

    try {
        cart.add(cheese, 1);
        cart.add(biscuits, 1);
        cart.add(scratchCard, 1);
        checkout(customer, cart);
    } catch (const exception& e) {
        cerr << "Checkout failed: " << e.what() << '\n';
    }
    return 0;
}
