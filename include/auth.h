#ifndef AUTH_H
#define AUTH_H

#include <stdbool.h>
#include <pthread.h>

extern pthread_mutex_t db_mutex;

bool authenticate_user(const char* username, const char* password);
bool register_user(const char* username, const char* password);
void init_database();

#endif