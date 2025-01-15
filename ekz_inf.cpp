#include <iostream>
#include <memory>
#include <pqxx/pqxx>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>  // Для записи в файл
#include <vector>
#include <chrono>
#include <ctime>

// Шаблонный класс для работы с PostgreSQL
template<typename T>
class DatabaseConnection {
public:
    DatabaseConnection(const std::string& connStr) : conn(connStr) {
        if (conn.is_open()) {
            spdlog::info("Connection to database established.");
        } else {
            spdlog::error("Failed to connect to database.");
            throw std::runtime_error("Failed to connect to database.");
        }
    }

    // Выполнение SQL-запроса с параметрами
    std::vector<std::vector<std::string>> executeQuery(const std::string& query, const std::vector<std::string>& params = {}) {
        pqxx::nontransaction ntx(conn);
        pqxx::result res;

        try {
            pqxx::prepare::named(stmt, query);  // Использование именованных запросов
            for (size_t i = 0; i < params.size(); ++i) {
                stmt.parameter(i + 1) = params[i];
            }
            res = ntx.exec(stmt);
        } catch (const std::exception& e) {
            spdlog::error("Error executing query: {}", e.what());
            throw;
        }

        std::vector<std::vector<std::string>> result;
        for (const auto& row : res) {
            std::vector<std::string> rowData;
            for (const auto& field : row) {
                rowData.push_back(field.c_str());
            }
            result.push_back(rowData);
        }

        return result;
    }

    // Выполнение SQL-запроса без возвращаемых данных
    void executeNonQuery(const std::string& query, const std::vector<std::string>& params = {}) {
        pqxx::work txn(conn);

        try {
            pqxx::prepare::named(stmt, query);
            for (size_t i = 0; i < params.size(); ++i) {
                stmt.parameter(i + 1) = params[i];
            }
            txn.exec(stmt);
            txn.commit();
        } catch (const std::exception& e) {
            spdlog::error("Error executing non-query: {}", e.what());
            txn.abort();
            throw;
        }
    }

    // Работа с транзакциями
    void beginTransaction() {
        txn = std::make_unique<pqxx::work>(conn);
    }

    void commitTransaction() {
        if (txn) {
            txn->commit();
        }
    }

    void rollbackTransaction() {
        if (txn) {
            txn->abort();
        }
    }

    ~DatabaseConnection() {
        if (conn.is_open()) {
            conn.close();
        }
    }

private:
    pqxx::connection conn;
    pqxx::prepare::declaration stmt;
    std::unique_ptr<pqxx::work> txn;
};

// Базовый класс пользователя
class User {
public:
    virtual void viewOrderStatus(int orderId) = 0;
    virtual void createOrder() = 0;
    virtual void cancelOrder(int orderId) = 0;
    virtual void returnOrder(int orderId) = 0;
    virtual ~User() = default;
};

// Класс Администратора
class Admin : public User {
public:
    void viewOrderStatus(int orderId) override {
        try {
            std::cout << "Viewing status of order ID " << orderId << " as Admin." << std::endl;
            dbConn.executeQuery("SELECT status FROM orders WHERE order_id = $1", {std::to_string(orderId)});
        } catch (const std::exception& e) {
            spdlog::error("Error viewing order status: {}", e.what());
        }
    }

    void createOrder() override {
        try {
            std::cout << "Admin creates a new order." << std::endl;
            dbConn.executeNonQuery("INSERT INTO orders (status) VALUES ($1)", {"pending"});
        } catch (const std::exception& e) {
            spdlog::error("Error creating order: {}", e.what());
        }
    }

    void cancelOrder(int orderId) override {
        try {
            std::cout << "Admin cancels order ID " << orderId << std::endl;
            dbConn.executeNonQuery("UPDATE orders SET status = $1 WHERE order_id = $2", {"canceled", std::to_string(orderId)});
        } catch (const std::exception& e) {
            spdlog::error("Error canceling order: {}", e.what());
        }
    }

    void returnOrder(int orderId) override {
        try {
            std::cout << "Admin returns order ID " << orderId << std::endl;
            dbConn.executeNonQuery("UPDATE orders SET status = $1 WHERE order_id = $2", {"returned", std::to_string(orderId)});
        } catch (const std::exception& e) {
            spdlog::error("Error returning order: {}", e.what());
        }
    }

    void addProduct(const std::string& name, double price, int stock) {
        try {
            std::cout << "Admin adds a new product: " << name << std::endl;
            dbConn.executeNonQuery("INSERT INTO products (name, price, stock_quantity) VALUES ($1, $2, $3)", 
                                    {name, std::to_string(price), std::to_string(stock)});
        } catch (const std::exception& e) {
            spdlog::error("Error adding product: {}", e.what());
        }
    }

    void deleteProduct(int productId) {
        try {
            std::cout << "Admin deletes product with ID: " << productId << std::endl;
            dbConn.executeNonQuery("DELETE FROM products WHERE product_id = $1", {std::to_string(productId)});
        } catch (const std::exception& e) {
            spdlog::error("Error deleting product: {}", e.what());
        }
    }

private:
    DatabaseConnection<pqxx::connection> dbConn{"dbname=shopdb user=admin password=admin"};
};

// Класс Менеджера
class Manager : public User {
public:
    void viewOrderStatus(int orderId) override {
        try {
            std::cout << "Viewing status of order ID " << orderId << " as Manager." << std::endl;
            dbConn.executeQuery("SELECT status FROM orders WHERE order_id = $1", {std::to_string(orderId)});
        } catch (const std::exception& e) {
            spdlog::error("Error viewing order status: {}", e.what());
        }
    }

    void createOrder() override {
        try {
            std::cout << "Manager creates a new order." << std::endl;
            dbConn.executeNonQuery("INSERT INTO orders (status) VALUES ($1)", {"pending"});
        } catch (const std::exception& e) {
            spdlog::error("Error creating order: {}", e.what());
        }
    }

    void cancelOrder(int orderId) override {
        try {
            std::cout << "Manager cancels order ID " << orderId << std::endl;
            dbConn.executeNonQuery("UPDATE orders SET status = $1 WHERE order_id = $2", {"canceled", std::to_string(orderId)});
        } catch (const std::exception& e) {
            spdlog::error("Error canceling order: {}", e.what());
        }
    }

    void returnOrder(int orderId) override {
        try {
            std::cout << "Manager returns order ID " << orderId << std::endl;
            dbConn.executeNonQuery("UPDATE orders SET status = $1 WHERE order_id = $2", {"returned", std::to_string(orderId)});
        } catch (const std::exception& e) {
            spdlog::error("Error returning order: {}", e.what());
        }
    }

    void approveOrder(int orderId) {
        try {
            std::cout << "Manager approves order ID " << orderId << std::endl;
            dbConn.executeNonQuery("UPDATE orders SET status = $1 WHERE order_id = $2", {"approved", std::to_string(orderId)});
        } catch (const std::exception& e) {
            spdlog::error("Error approving order: {}", e.what());
        }
    }

private:
    DatabaseConnection<pqxx::connection> dbConn{"dbname=shopdb user=manager password=manager"};
};

// Класс Покупателя
class Customer : public User {
public:
    void viewOrderStatus(int orderId) override {
        try {
            std::cout << "Viewing status of order ID " << orderId << " as Customer." << std::endl;
            dbConn.executeQuery("SELECT status FROM orders WHERE order_id = $1", {std::to_string(orderId)});
        } catch (const std::exception& e) {
            spdlog::error("Error viewing order status: {}", e.what());
        }
    }

    void createOrder() override {
        try {
            std::cout << "Customer creates a new order." << std::endl;
            dbConn.executeNonQuery("INSERT INTO orders (status) VALUES ($1)", {"pending"});
        } catch (const std::exception& e) {
            spdlog::error("Error creating order: {}", e.what());
        }
    }

    void cancelOrder(int orderId) override {
        try {
            std::cout << "Customer cancels order ID " << orderId << std::endl;
            dbConn.executeNonQuery("UPDATE orders SET status = $1 WHERE order_id = $2", {"canceled", std::to_string(orderId)});
        } catch (const std::exception& e) {
            spdlog::error("Error canceling order: {}", e.what());
        }
    }

    void returnOrder(int orderId) override {
        try {
            std::cout << "Customer returns order ID " << orderId << std::endl;
            dbConn.executeNonQuery("UPDATE orders SET status = $1 WHERE order_id = $2", {"returned", std::to_string(orderId)});
        } catch (const std::exception& e) {
            spdlog::error("Error returning order: {}", e.what());
        }
    }

    void addToOrder(int orderId, int productId, int quantity) {
        try {
            std::cout << "Customer adds product ID " << productId << " to order ID " << orderId << std::endl;
            dbConn.executeNonQuery("INSERT INTO order_items (order_id, product_id, quantity) VALUES ($1, $2, $3)",
                                    {std::to_string(orderId), std::to_string(productId), std::to_string(quantity)});
        } catch (const std::exception& e) {
            spdlog::error("Error adding product to order: {}", e.what());
        }
    }

    void removeFromOrder(int orderId, int productId) {
        try {
            std::cout << "Customer removes product ID " << productId << " from order ID " << orderId << std::endl;
            dbConn.executeNonQuery("DELETE FROM order_items WHERE order_id = $1 AND product_id = $2",
                                    {std::to_string(orderId), std::to_string(productId)});
        } catch (const std::exception& e) {
            spdlog::error("Error removing product from order: {}", e.what());
        }
    }

private:
    DatabaseConnection<pqxx::connection> dbConn{"dbname=shopdb user=customer password=customer"};
};

// Меню программы
void showMainMenu() {
    std::cout << "1. Login as Admin\n";
    std::cout << "2. Login as Manager\n";
    std::cout << "3. Login as Customer\n";
    std::cout << "4. Exit\n";
}

// Главная функция
int main() {
    // Настройка логирования
    auto logger = spdlog::basic_logger_mt("basic_logger", "logs.txt");

    bool running = true;
    while (running) {
        showMainMenu();
        int choice;
        std::cin >> choice;

        switch (choice) {
            case 1:
                {
                    Admin admin;
                    admin.addProduct("Product1", 99.99, 100);
                    admin.deleteProduct(1);
                }
                break;
            case 2:
                {
                    Manager manager;
                    manager.approveOrder(1);
                }
                break;
            case 3:
                {
                    Customer customer;
                    customer.createOrder();
                    customer.addToOrder(1, 101, 2);
                }
                break;
            case 4:
                running = false;
                break;
            default:
                std::cout << "Invalid choice. Please try again.\n";
                break;
        }
    }

    return 0;
}
