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
#endif //OS_STARTER_CODE_EVENTSTRUCT_H
