#ifndef __HISTORY_H__
#define __HISTORY_H__

#include "signal.hpp"
#include <deque>
#include <readline/readline.h>
#define SIZE 1000

using namespace std;

typedef struct _history_state{
    int index;
    int size;
    deque<string> history;
} history_state;

history_state cmd_history;

void read_history();
void write_history();
void add_history(char* s);
int backward_history(int count, int key);
int forward_history(int count, int key);
void initialize_readline();

#endif