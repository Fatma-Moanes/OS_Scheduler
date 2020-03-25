//
// Created by shaffei on 3/24/20.
//
// Definition of the struct that represents a single process
#ifndef OS_STARTER_CODE_PROCESS_H
#define OS_STARTER_CODE_PROCESS_H

#include <stdio.h>

typedef struct Processes {
    unsigned int mId;
    unsigned int mArrivalTime;
    unsigned int mPriority; //here we assume that a negative priority is not allowed
    unsigned int mRuntime;
    unsigned int mRemainTime;
    unsigned int mWaitTime;
} Process;

void PrintProcess(Process *pProcess) {  //for debugging purposes
    printf("ID = %d, ", pProcess->mId);
    printf("Arrival = %d, ", pProcess->mArrivalTime);
    printf("Runtime = %d, ", pProcess->mRuntime);
    printf("Priority = %d, ", pProcess->mPriority);
    printf("Remaining time = %d, ", pProcess->mRemainTime);
    printf("Waiting time = %d\n", pProcess->mWaitTime);
}

#endif