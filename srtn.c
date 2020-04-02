#include "Headers/headers.h"
#include "Headers/ProcessStruct.h"
#include "Headers/ProcessHeap.h"
#include "Headers/MessageBuffer.h"
#include "Headers/EventsQueue.h"
#include <math.h>

void ProcessArrivalHandler(int);

void InitIPC();

int ReceiveProcess();

void CleanResources();

void ExecuteProcess();

void ChildHandler(int signum);

void LogEvents(unsigned int, unsigned int);

void AddEvent(enum EventType);

int gMsgQueueId = 0;

Process *gpCurrentProcess = NULL;

heap_t *gProcessHeap;

bool gSwitchContext;

event_queue gEventQueue; //queue to store events occurring in the scheduler

int main(int argc, char *argv[]) {
    printf("SRTN: *** Scheduler here\n");
    initClk();
    InitIPC();
    //initialize processes heap
    gProcessHeap = (heap_t *) calloc(1, sizeof(heap_t));
    gEventQueue = NewEventQueue();

    signal(SIGUSR1, ProcessArrivalHandler);
    signal(SIGINT, CleanResources);
    signal(SIGCHLD, ChildHandler);

    pause();  //wait till a process comes

    unsigned int start_time = getClk();

    while ((gpCurrentProcess = HeapPop(gProcessHeap)) != NULL) {
        ExecuteProcess(); //starts the process with the least remaining time and handles context switching
        gSwitchContext = 0;

        while (!gSwitchContext)  //loops as long as the current process running has the least remaining time
        {
            sleep(1);
            gpCurrentProcess->mRemainTime--;
        }


    }
    unsigned int end_time = getClk();
    LogEvents(start_time,end_time);
}

void ProcessArrivalHandler(int signum) {
    //keep looping as long as a process was received in the current iteration
    while (!ReceiveProcess());

    if (HeapPeek(gProcessHeap)->mRuntime < gpCurrentProcess->mRemainTime) {

        kill(gpCurrentProcess->mPid, SIGTSTP);
        gpCurrentProcess->mLastStop = getClk();
        gSwitchContext = 1;
        AddEvent(STOP);
    }
}

void InitIPC() {
    key_t key = ftok(gFtokFile, gFtokCode); //same parameters used in process_generator so we attach to same queue
    gMsgQueueId = msgget(key, 0);
    if (gMsgQueueId == -1) {
        perror("SRTN: *** Scheduler IPC init failed");
        raise(SIGINT);
    }
    printf("SRTN: *** Scheduler IPC ready!\n");

}

int ReceiveProcess() {
    Message msg;
    //receive a message but do not wait, if not found return immediately
    while (msgrcv(gMsgQueueId, &msg, __size_t sizeof(msg.mType), 0, IPC_NOWAIT) == -1) {
        perror("SRTN: *** Error in receive");
        return -1;

    }

    //below is executed if a message was retrieved from the message queue
    printf("SRTN: *** Received by scheduler\n");
    Process *pProcess = malloc(sizeof(Process)); //allocate memory for the received process

    while (!pProcess) {
        perror("SRTN: *** Malloc failed");
        printf("SRTN: *** Trying again");
        pProcess = malloc(sizeof(Process));
    }
    *pProcess = msg.mProcess; //store the process received in the allocated space
    //push the process pointer into the process heap, and use the process runtime as the value to sort the heap with
    HeapPush(gProcessHeap, pProcess->mRemainTime, pProcess);

    /*
    Event *pEvent = malloc(sizeof(Event)); //allocate memory for a new event
    while (!pEvent) {
        perror("SRTN: *** Malloc failed");
        printf("SRTN: *** Trying again");
        pEvent = malloc(sizeof(Process));
    }
    pEvent->mType = START;
    pEvent->mCurrentRemTime = 0;
    pEvent->mCurrentWaitTime = 0;
    pEvent->mpProcess = pProcess;
    pEvent->mTimeStep = getClk();
    EventQueueEnqueue(gEventQueue, pEvent); //enqueue this event in the event queue
     */
    return 0;
}

void CleanResources() {
}

void ExecuteProcess() {

    if (gpCurrentProcess->mRemainTime == gpCurrentProcess->mRuntime)  //if the process hasn't executed before
    {
        gpCurrentProcess->mPid = fork();

        if (gpCurrentProcess->mPid == 0) {

            char buffer[10]; //buffer to convert runtime from int to string
            sprintf(buffer, "%d", gpCurrentProcess->mRuntime);
            char *argv[] = {"process.out", buffer, NULL};
            execv("process.out", argv);
            perror("SRTN: *** Process execution failed");
            exit(EXIT_FAILURE);

        }
        AddEvent(START);

        gpCurrentProcess->mWaitTime = getClk() - gpCurrentProcess->mArrivalTime;

    } else //If the process executed before and paused
    {

        //send a continue signal to the process
        kill(gpCurrentProcess->mPid, SIGCONT);
        gpCurrentProcess->mWaitTime += getClk() - gpCurrentProcess->mLastStop;  //update the waiting time of the process
        AddEvent(CONT);

    }
};

void ChildHandler(int signum) {
    int status;
    if (!waitpid(gpCurrentProcess->mPid, &status, WNOHANG)) //if current process did not terminate
        return;

    gSwitchContext = 1;
    gpCurrentProcess->mRuntime = 0;
    AddEvent(FINISH);
}


void LogEvents(unsigned int start_time, unsigned int end_time) {  //prints all events in the terminal
    unsigned int runtime_sum = 0, waiting_sum = 0, count = 0;
    double wta_sum = 0, wta_squared_sum = 0;
    Event *pEvent = NULL;
    while (EventQueueDequeue(gEventQueue, &pEvent)) { //while event queue is not empty
        PrintEvent(pEvent);
        if (pEvent->mType == FINISH) {
            runtime_sum += pEvent->mpProcess->mRuntime;
            waiting_sum += pEvent->mCurrentWaitTime;
            count++;
            wta_sum += pEvent->mWTaTime;
            wta_squared_sum += pEvent->mWTaTime * pEvent->mWTaTime;
            free(pEvent->mpProcess);
        }
        free(pEvent); //free memory allocated by the event
    }
    double cpu_utilization = runtime_sum * 100.0 / (end_time - start_time);
    double avg_wta = wta_sum / count;
    double avg_waiting = (double) waiting_sum / count;
    double std_wta = sqrt((wta_squared_sum - (2 * wta_sum * avg_wta) + (avg_wta * avg_wta * count)) / count);

    printf("\nCPU utilization = %.2f\n", cpu_utilization);
    printf("Avg WTA = %.2f\n", avg_wta);
    printf("Avg Waiting = %.2f\n", avg_waiting);
    printf("STD WTA = %.2f\n\n", std_wta);
}

void AddEvent(enum EventType type) {
    Event *pEvent = malloc(sizeof(Event));
    while (!pEvent) {
        perror("SRTN: *** Malloc failed");
        printf("SRTN: *** Trying again");
        pEvent = malloc(sizeof(Event));
    }
    pEvent->mTimeStep = getClk();
    if (type == FINISH) {
        pEvent->mTaTime = getClk() - gpCurrentProcess->mArrivalTime;
        pEvent->mWTaTime = (double) pEvent->mTaTime / gpCurrentProcess->mRuntime;
    }
    pEvent->mpProcess = gpCurrentProcess;
    pEvent->mCurrentWaitTime = gpCurrentProcess->mWaitTime;
    pEvent->mType = type;
    pEvent->mCurrentRemTime = gpCurrentProcess->mRemainTime;
    EventQueueEnqueue(gEventQueue, pEvent);
    //PrintEvent(pEvent);
}