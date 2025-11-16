#ifndef KERNEL_SIM
#define KERNEL_SIM

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h> 
#include <pthread.h>

#define TIME_FRAME_USEC 500000
#define PC_MAX 100
#define SEND_D1_CHANCE_1000 100 // Out of 1000
#define SEND_D2_CHANCE_1000 5 // Out of 1000
#define REQUEST_INPUT_CHANCE_100 15

#define CLOCK 0 // Code for clock tick sent by inter controller

#define FINISH -1 // Code for app finished sent by app.

#define D1 1 //Code for waiting on input 1 sent by app.
#define D2 2 //Code for waiting on input 2 sent by app.

#define WT 1 //Code for waiting on read sent by app.
#define RD 2 //Code for waiting on write sent by app.
#define AD 3 //Code for waiting on exec sent by app.
#define RM 4 //Code for waiting on exec sent by app.
#define LS 5 //Code for waiting on exec sent by app.

// Enumeration for process states
#define FINISHED -1
#define EXECUTING 0
#define READY 1

#define WAITING_D1_WT (D1 + WT * 10) // 11
#define WAITING_D1_RD (D1 + RD * 10) // 12
#define WAITING_D1_AD (D1 + AD * 10) // 13
#define WAITING_D1_RM (D1 + RM * 10) // 14
#define WAITING_D1_LS (D1 + LS * 10) // 15

#define WAITING_D2_WT (D2 + WT * 10) // 21
#define WAITING_D2_RD (D2 + RD * 10) // 22
#define WAITING_D2_AD (D2 + AD * 10) // 23
#define WAITING_D2_RM (D2 + RM * 10) // 24
#define WAITING_D2_LS (D2 + LS * 10) // 25
//

typedef struct appContext
{
    int PC;
} AppContext;

typedef struct appData
{
    AppContext context;
    pthread_mutex_t appMutex;
    int appSendingData;
} AppData;

const char* FINISHED_STR = "Finished";
const char* EXECUTING_STR = "Executing";
const char* READY_STR = "Ready";

const char* WAITING_D1_WT_STR = "Waiting D1 for WT";
const char* WAITING_D1_RD_STR = "Waiting D1 for RD";
const char* WAITING_D1_AD_STR = "Waiting D1 for AD";
const char* WAITING_D1_RM_STR = "Waiting D1 for RM";
const char* WAITING_D1_LS_STR = "Waiting D1 for LS";

const char* WAITING_D2_WT_STR = "Waiting D2 for WT";
const char* WAITING_D2_RD_STR = "Waiting D2 for RD";
const char* WAITING_D2_AD_STR = "Waiting D2 for AD";
const char* WAITING_D2_RM_STR = "Waiting D2 for RM";
const char* WAITING_D2_LS_STR = "Waiting D2 for LS";

const char* INVALID_STATE_STR = "Invalid state";

const char* AppStateToStr(int state)
{
    if (state == FINISHED)
        return FINISHED_STR;
    else if (state == EXECUTING)
        return EXECUTING_STR;
    else if (state == READY)
        return READY_STR;
    else if (state == WAITING_D1_WT)
        return WAITING_D1_WT_STR;
    else if (state == WAITING_D1_RD)
        return WAITING_D1_RD_STR;
    else if (state == WAITING_D1_AD)
        return WAITING_D1_AD_STR;
    else if (state == WAITING_D1_RM)
        return WAITING_D1_RM_STR;
    else if (state == WAITING_D1_LS)
        return WAITING_D1_LS_STR;
        
    else if (state == WAITING_D2_WT)
        return WAITING_D2_WT_STR;
    else if (state == WAITING_D2_RD)
        return WAITING_D2_RD_STR;
    else if (state == WAITING_D2_AD)
        return WAITING_D2_AD_STR;
    else if (state == WAITING_D2_RM)
        return WAITING_D2_RM_STR;
    else if (state == WAITING_D2_LS)
        return WAITING_D2_LS_STR;
    
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
