#include "../include/auth.h"
#include <sqlite3.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>  // Добавлено для exit()
#include <stdbool.h> // Добавлено для bool

extern pthread_mutex_t db_mutex;

void init_database() {
    sqlite3* db;
    char* err_msg = 0;
    int rc = sqlite3_open("../db/auth.db", &db);
    
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Не удалось открыть базу данных: %s\n", sqlite3_errmsg(db));
        exit(1);
    }
    
    const char* sql = "CREATE TABLE IF NOT EXISTS users("
                      "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                      "username TEXT NOT NULL UNIQUE,"
                      "password TEXT NOT NULL);";
    
    pthread_mutex_lock(&db_mutex);
    rc = sqlite3_exec(db, sql, 0, 0, &err_msg);
    pthread_mutex_unlock(&db_mutex);
    
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
    }
    
    sqlite3_close(db);
}

bool authenticate_user(const char* username, const char* password) {
    sqlite3* db;
    sqlite3_stmt* stmt;
    int rc = sqlite3_open("../db/auth.db", &db);
    
    const char* sql = "SELECT 1 FROM users WHERE username = ? AND password = ?;";
    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    
    sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, password, -1, SQLITE_STATIC);
    
    rc = sqlite3_step(stmt);
    bool result = (rc == SQLITE_ROW);
    
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    
    return result;
}

bool register_user(const char* username, const char* password) {
    sqlite3* db;
    sqlite3_stmt* stmt;
    int rc = sqlite3_open("../db/auth.db", &db);
    
    const char* sql = "INSERT INTO users(username, password) VALUES(?, ?);";
    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    
    sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, password, -1, SQLITE_STATIC);
    
    pthread_mutex_lock(&db_mutex);
    rc = sqlite3_step(stmt);
    pthread_mutex_unlock(&db_mutex);
    
    bool result = (rc == SQLITE_DONE);
    
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    
    return result;
}