#ifndef KERNEL_SIM
#define KERNEL_SIM

#include "sfss.h"

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <pthread.h>
#include <string.h>

#define TIME_FRAME_USEC 500000
#define PC_MAX 100
#define SEND_D1_CHANCE_1000 100 // Out of 1000
#define SEND_D2_CHANCE_1000 5 // Out of 1000
#define REQUEST_INPUT_CHANCE_100 15
#define MAX_SYSCALL_INFO_SIZE 1024
#define UDP_REQ_RESEND_DURATION_MSEC 500

#define CLOCK_TIME_MS (clock() / (CLOCKS_PER_SEC / 1000.0))

#define CLOCK 0 // Code for clock tick sent by inter controller

#define FINISH -1 // Code for app finished sent by app.

#define D1 1
#define D2 2

#define WT 1 //Code for waiting on read sent by app.
#define RD 2 //Code for waiting on write sent by app.
#define AD 3 //Code for waiting on exec sent by app.
#define RM 4 //Code for waiting on exec sent by app.
#define LS 5 //Code for waiting on exec sent by app.

// Enumeration for process states
#define FINISHED -1
#define EXECUTING 0
#define READY 1

#define WAITING_WT (WT + READY)
#define WAITING_RD (RD + READY)
#define WAITING_AD (AD + READY)
#define WAITING_RM (RM + READY)
#define WAITING_LS (LS + READY)
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

const char* WAITING_WT_STR = "Waiting D1 for WT";
const char* WAITING_RD_STR = "Waiting D1 for RD";
const char* WAITING_AD_STR = "Waiting D2 for AD";
const char* WAITING_RM_STR = "Waiting D2 for RM";
const char* WAITING_LS_STR = "Waiting D2 for LS";

const char* INVALID_STATE_STR = "Invalid state";

const char* AppStateToStr(int state)
{
    if (state == FINISHED)
        return FINISHED_STR;
    else if (state == EXECUTING)
        return EXECUTING_STR;
    else if (state == READY)
        return READY_STR;
    else if (state == WAITING_WT)
        return WAITING_WT_STR;
    else if (state == WAITING_RD)
        return WAITING_RD_STR;
    else if (state == WAITING_AD)
        return WAITING_AD_STR;
    else if (state == WAITING_RM)
        return WAITING_RM_STR;
    else if (state == WAITING_LS)
        return WAITING_LS_STR;
    
    return INVALID_STATE_STR;
}

void itoa(int a, char* str, int strLen)
{
    for (int i = strLen - 1; i >= 0; i--)
    {
        str[i] = (a % 10) + '0';
        a /= 10;
    }
}

#endif
