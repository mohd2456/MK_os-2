#include <iostream>
#include <string>
#include <sqlite3.h>

class MKDatasetDB {
private:
    sqlite3* db = nullptr; // Keeps the database pointer alive for the class

public:
    // Destructor ensures the database is safely closed when the object is destroyed
    ~MKDatasetDB() {
        closeDB();
    }

    void openDB(const std::string& filename) {
        if (sqlite3_open(filename.c_str(), &db)) {
            std::cerr << "[DB] Can't open database: " << sqlite3_errmsg(db) << std::endl;
            db = nullptr;
        } else {
            std::cout << "[DB] Opened database successfully and holding connection active." << std::endl;
        }
    }

    // New Helper: Allows MK to run actual SQL statements (like creating tables or inserting data)
    void executeSQL(const std::string& sql) {
        if (!db) {
            std::cerr << "[DB] Error: Cannot execute SQL. No active database connection." << std::endl;
            return;
        }

        char* errMsg = nullptr;
        int rc = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errMsg);
        
        if (rc != SQLITE_OK) {
            std::cerr << "[DB] SQL Error: " << errMsg << std::endl;
            sqlite3_free(errMsg);
        } else {
            std::cout << "[DB] SQL Query executed successfully." << std::endl;
        }
    }

    void closeDB() {
        if (db) {
            sqlite3_close(db);
            db = nullptr;
            std::cout << "[DB] Database connection closed safely." << std::endl;
        }
    }
};