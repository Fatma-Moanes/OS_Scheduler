#include "Headers/headers.h"
#include "Headers/ProcessStruct.h"
#include "Headers/ProcessHeap.h"
#include "Headers/MessageBuffer.h"
#include "Headers/EventsQueue.h"
#include "Headers/ProcessQueue.h"

void ProcessArrivalHandler(int);

void InitIPC();

int ReceiveProcess();

void CleanResources();

int gMsgQueueId = 0;

heap_t *gProcessHeap;
event_queue gEventQueue;

int main(int argc, char *argv[]) {
    printf("RR: *** Scheduler here, and type is round robin\n");
    initClk(); //attach to shared memory of clock
    InitIPC(); //attach to the message queue of the process_generator
    gEventQueue = NewEventQueue(); //initialize events queue
    gProcessHeap = (heap_t *) calloc(1, sizeof(heap_t)); //initialize processes heap

    signal(SIGUSR1, ProcessArrivalHandler); //handle SIGUSR1 sent by process_generator when new process is available
    signal(SIGINT, CleanResources); //handle SIGINT to release resources

    while (1) {
        pause();
    }
}

void ProcessArrivalHandler(int signum) {
    //keep looping as long as a process was received
    while (!ReceiveProcess());
}

void InitIPC() {
    key_t key = ftok(gFtokFile, gFtokCode);
    gMsgQueueId = msgget(key, 0);
    if (gMsgQueueId == -1) {
        perror("RR: *** Scheduler IPC init failed");
        raise(SIGINT);
    }
    printf("RR: *** Scheduler IPC ready!\n");
}

int ReceiveProcess() {
    Message msg;
    //receive a message but do not wait, if not found return immediately
    if (msgrcv(gMsgQueueId, (void *) &msg, sizeof(msg.mProcess), 0, IPC_NOWAIT) == -1) {
        perror("RR: *** Error in receive");
        return -1;
    }

    //below is executed if a message was retrieved from the message queue
    printf("RR: *** Received by scheduler\n");
    Process *pProcess = malloc(sizeof(Process)); //allocate memory for the received process
    while (!pProcess) {
        perror("RR: *** Malloc failed");
        printf("RR: *** Trying again");
        pProcess = malloc(sizeof(Process));
    }
    *pProcess = msg.mProcess; //store the process received in the allocated space
    //push the process pointer into the process heap, and use the process priority as the value to sort the heap with
    HeapPush(gProcessHeap, pProcess->mPriority, pProcess);
    Event *pEvent = malloc(sizeof(Event)); //allocate memory for a new event
    while (!pEvent) {
        perror("RR: *** Malloc failed");
        printf("RR: *** Trying again");
        pEvent = malloc(sizeof(Process));
    }

    pEvent->mType = START;
    pEvent->mCurrentRemTime = 0;
    pEvent->mCurrentWaitTime = 0;
    pEvent->mpProcess = pProcess;
    pEvent->mTimeStep = getClk();

    EventQueueEnqueue(gEventQueue, pEvent); //enqueue this event in the event queue
    return 0;
}

void CleanResources() {
    while (!HeapEmpty(gProcessHeap)) { //while heap is not empty
        PrintProcess(HeapPeek(gProcessHeap));
        free(HeapPop(gProcessHeap)); //pop from heap and free memory allocated by the popped process
    }
    free(gProcessHeap);
    while (!EventQueueEmpty(gEventQueue)) { //while event queue is not empty
        Event *pTempEvent;
        EventQueueDequeue(gEventQueue, &pTempEvent); //dequeue from event queue
        free(pTempEvent); //free memory allocated by the event
    }
    exit(EXIT_SUCCESS);
};