//
// Created by shaffei on 3/24/20.
//

#ifndef OS_STARTER_CODE_EVENTSTRUCT_H
#define OS_STARTER_CODE_EVENTSTRUCT_H

#include "ProcessStruct.h"

enum EventType {
    START, STOP, CONT, FINISH
};

typedef struct EventStruct {
    enum EventType mType;
    Process *mpProcess;
    unsigned int mTimeStep;
    unsigned int mCurrentRemTime;
    unsigned int mCurrentWaitTime;
    unsigned int mTaTime;
    double mWTaTime;
} Event;

void PrintEvent(const Event *pEvent) { //print an event using the same output file format
    printf("At time %d ", pEvent->mTimeStep);
    printf("process %d ", pEvent->mpProcess->mId);
    switch (pEvent->mType) {
        case START:
            printf("started ");
            break;
        case STOP:
            printf("stopped ");
            break;
        case CONT:
            printf("resumed ");
            break;
        case FINISH:
            printf("finished ");
            break;
        default:
            printf("error ");
            break;
    }
    printf("arr %d total %d ", pEvent->mpProcess->mArrivalTime, pEvent->mpProcess->mRuntime);
    printf("remain %d wait %d", pEvent->mCurrentRemTime, pEvent->mCurrentWaitTime);
    if (pEvent->mType == FINISH)
        printf(" TA %d WTA %.2f", pEvent->mTaTime, pEvent->mWTaTime);
    printf("\n");
}

#endif //OS_STARTER_CODE_EVENTSTRUCT_H
