#include "Headers/headers.h"
#include "Headers/ProcessStruct.h"
#include "Headers/ProcessHeap.h"
#include "Headers/MessageBuffer.h"
#include "Headers/EventsQueue.h"
#include "Headers/ProcessQueue.h"

void ProcessArrivalHandler(int);

void InitIPC();

int ReceiveProcess();

void CleanResources(int);

void ExecuteProcess();

void AlarmHandler(int);

void ChildHandler(int);

void AddEvent(enum EventType);

int HandleTie();

int gMsgQueueId = 0;
unsigned int gQuanta;
short gSwitchContextFlag = 0; //flag for main loop to switch context or not 1 = switch, 0 = do not switch
queue gProcessQueue;
event_queue gEventQueue;
heap_t *gProcessHeap;
Process *gpCurrentProcess = NULL; //pointer to th current running process

int main(int argc, char *argv[]) {
    gQuanta = atoi(argv[1]);
    printf("RR: *** Scheduler here, and type is round robin with quanta %d\n", gQuanta);
    initClk(); //attach to shared memory of clock
    InitIPC(); //attach to the message queue of the process_generator
    gEventQueue = NewEventQueue(); //init event queue
    gProcessQueue = NewProcQueue(); //init processes queue
    gProcessHeap = (heap_t *)calloc(1, sizeof (heap_t)); //init processes heap
    signal(SIGUSR1, ProcessArrivalHandler); //handle SIGUSR1 sent by process_generator when new process is available
    signal(SIGINT, CleanResources); //handle SIGINT to release resources
    signal(SIGALRM, AlarmHandler); //handle alarm signals received at the end of each quanta using the AlarmHandler
    signal(SIGCHLD, ChildHandler); //handle when a process finishes execution using ChildHandler

    pause(); //wait until the first process arrives when SIGUSR1 is received
    while (ProcDequeue(gProcessQueue, &gpCurrentProcess)) { //while processes queue is not empty
        if (HandleTie()) {
            while (!HeapEmpty(gProcessHeap)) {
                gpCurrentProcess = HeapPop(gProcessHeap);
                ExecuteProcess();
                gSwitchContextFlag = 0;
                while (!gSwitchContextFlag) //as long as this flag is set to zero keep pausing
                    pause();
            }
            continue;
        }
        gSwitchContextFlag = 0;
        ExecuteProcess();
        while (!gSwitchContextFlag) //as long as this flag is set to zero keep pausing
            pause();
    }
    raise(SIGINT); //when it's done raise SIGINT to clean and exit
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
    ProcEnqueue(gProcessQueue, pProcess);//push the process pointer into the process queue
    return 0;
}

void CleanResources(int signum) {
    printf("RR: *** Cleaning scheduler resources\n");
    /*Process *pProcess = NULL;
    while (ProcDequeue(gProcessQueue, &pProcess)) { //while processes queue is not empty
        PrintProcess(pProcess);
        free(pProcess); //free memory allocated by this process
    }*/

    Event *pEvent = NULL;
    while (EventQueueDequeue(gEventQueue, &pEvent)) { //while event queue is not empty
        PrintEvent(pEvent);
        free(pEvent); //free memory allocated by the event
    }
    printf("RR: *** Scheduler clean!\n");
    exit(EXIT_SUCCESS);
};

void ExecuteProcess() {
    if (gpCurrentProcess->mRuntime == gpCurrentProcess->mRemainTime) { //if this process never ran before
        gpCurrentProcess->mPid = fork(); //fork a new child and store its pid in the process struct
        while (gpCurrentProcess->mPid == -1) { //if forking fails
            perror("RR: *** Error forking process");
            printf("RR: *** Trying again...\n");
            gpCurrentProcess->mPid = fork();
        }
        if (!gpCurrentProcess->mPid) { //if child
            char buffer[10]; //buffer to convert runtime from int to string
            sprintf(buffer, "%d", gpCurrentProcess->mRuntime);
            char *argv[] = {"process.out", buffer, NULL};
            execv("process.out", argv);
            perror("RR: *** Process execution failed");
            exit(EXIT_FAILURE);
        }
        alarm(gQuanta); //register an alarm to fire when the quanta is over
        gpCurrentProcess->mWaitTime = getClk() - gpCurrentProcess->mArrivalTime;
        //create an event
        AddEvent(START);
    } else { //this process was stopped and now we need to resume it
        if (kill(gpCurrentProcess->mPid, SIGCONT) == -1) { //continue process
            perror("RR: *** Error resuming process");
            return;
        }
        alarm(gQuanta); //register an alarm to fire when the quanta is over
        gpCurrentProcess->mWaitTime += getClk() - gpCurrentProcess->mLastStop; //add the additional waiting time
        AddEvent(CONT);
    }
}

void AlarmHandler(int signum) {
    gpCurrentProcess->mRemainTime -= gQuanta; //remaining time should have been decreased by the value of the quanta
    if (!gpCurrentProcess->mRemainTime) //if this processed should finish at the end of the quanta now
        return;
    while (kill(gpCurrentProcess->mPid, SIGTSTP) == -1) { //stop current process
        perror("RR: *** Error stopping process");
        printf("RR: *** Trying again...\n");
    }
    gpCurrentProcess->mLastStop = getClk(); //store the time at which it is stopped
    ProcEnqueue(gProcessQueue, gpCurrentProcess); //re-enqueue the process to the queue
    AddEvent(STOP);
    gSwitchContextFlag = 1; //set flag to 1 so main loop knows it's time to switch context
}

void ChildHandler(int signum) {
    int status;
    if (!waitpid(gpCurrentProcess->mPid, &status, WNOHANG)) //if current process did not terminate
        return;

    alarm(0); //cancel pending alarm
    gpCurrentProcess->mRemainTime = 0;
    AddEvent(FINISH);
    gSwitchContextFlag = 1; //set flag to 1 so main loop knows it's time to switch context
}

void AddEvent(enum EventType type) {
    Event *pEvent = malloc(sizeof(Event));
    while (!pEvent) {
        perror("RR: *** Malloc failed");
        printf("RR: *** Trying again");
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
    PrintEvent(pEvent);
}

int HandleTie() {
    if (gpCurrentProcess->mRuntime > gpCurrentProcess->mRemainTime) //if this process ran before
        return 0;

    Process *pTemp;
    if(!ProcPeek(gProcessQueue, &pTemp))
        return 0;

    short is_tie = 0;
    while (pTemp->mArrivalTime == gpCurrentProcess->mArrivalTime) {
        is_tie = 1;
        ProcDequeue(gProcessQueue, &pTemp);
        HeapPush(gProcessHeap, pTemp->mPriority, pTemp);
        ProcPeek(gProcessQueue, &pTemp);
    }
    if (is_tie)
        HeapPush(gProcessHeap, gpCurrentProcess->mPriority, gpCurrentProcess);
    return is_tie;
}