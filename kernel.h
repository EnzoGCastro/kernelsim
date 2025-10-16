#ifndef KERNEL_SIM
#define KERNEL_SIM

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>

#define CLOCK 0 // Code for clock tick sent by inter controller

#define FINISH -1 // Code for app finished sent by app.

#define D1 1 //Code for waiting on input 1 sent by app.
#define D2 2 //Code for waiting on input 2 sent by app.

#define R 1 //Code for waiting on read sent by app.
#define W 2 //Code for waiting on write sent by app.
#define X 3 //Code for waiting on exec sent by app.

// Enumeration for process states
#define FINISHED -1
#define EXECUTING 0
#define READY 1
#define WAITING_D1_W (D1 + W * 10) // 11
#define WAITING_D1_R (D1 + R * 10) // 12
#define WAITING_D1_X (D1 + X * 10) // 13
#define WAITING_D2_W (D2 + W * 10) // 21
#define WAITING_D2_R (D2 + R * 10) // 22
#define WAITING_D2_X (D2 + X * 10) // 23
//

typedef struct appContext
{
    int PC;
} AppContext;

const char* FINISHED_STR = "Finished";
const char* EXECUTING_STR = "Executing";
const char* READY_STR = "Ready";
const char* WAITING_D1_W_STR = "Waiting D1 for W";
const char* WAITING_D1_R_STR = "Waiting D1 for R";
const char* WAITING_D1_X_STR = "Waiting D1 for X";
const char* WAITING_D2_W_STR = "Waiting D2 for W";
const char* WAITING_D2_R_STR = "Waiting D2 for R";
const char* WAITING_D2_X_STR = "Waiting D2 for X";
const char* INVALID_STATE_STR = "Invalid state";

const char* AppStateToStr(int state)
{
    if (state == FINISHED)
        return FINISHED_STR;
    else if (state == EXECUTING)
        return EXECUTING_STR;
    else if (state == READY)
        return READY_STR;
    else if (state == WAITING_D1_W)
        return WAITING_D1_W_STR;
    else if (state == WAITING_D1_R)
        return WAITING_D1_R_STR;
    else if (state == WAITING_D1_X)
        return WAITING_D1_X_STR;
    else if (state == WAITING_D2_W)
        return WAITING_D2_W_STR;
    else if (state == WAITING_D2_R)
        return WAITING_D2_R_STR;
    else if (state == WAITING_D2_X)
        return WAITING_D2_X_STR;
    return INVALID_STATE_STR;
}

void itoa(int a, char* str, int strLen)
{
    int i = strLen;
    while (a > 0)
    {
        i--;
        str[i] = (a % 10) + '0';
        a /= 10;
    }
}

#endif
