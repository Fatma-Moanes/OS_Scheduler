#include "Headers/headers.h"
#include "Headers/ProcessStruct.h"
#include "Headers/ProcessHeap.h"
#include "Headers/MessageBuffer.h"
#include "Headers/EventsQueue.h"
#include "Headers/ProcessQueue.h"

event_queue gEventQueue;
heap_t *gProcessHeap;
queue gProcessQueue;
Process *gpCurrentProcess = NULL;
int gFirstTimeAddFlag=0;
//Process *gTestP = NULL;

void ProcessArrivalHandler(int);

void InitIPC();

int ReceiveProcess();

void ExecuteProcess();

void CleanResources();

void AddEvent(enum EventType);

int gMsgQueueId = 0;




int main(int argc, char *argv[]) {
    printf("HPF: *** Scheduler here\n");
    initClk(); ///printf("*** Clk Initiated in Sch successfully\n");
    InitIPC();
    //initialize processes heap
    gProcessHeap = (heap_t *) calloc(1, sizeof(heap_t));
    //initialize EventQueu
    gEventQueue= NewEventQueue();
    signal(SIGUSR1, ProcessArrivalHandler);
    signal(SIGINT, CleanResources);
    //ReceiveProcess();
    ///printf("*** Signals initiated, before pause() \n");
    Process *rcv_poped;
    //HeapPush(gProcessHeap,)
    pause();

    gFirstTimeAddFlag=0;
    int mTempRunTime=0;
    char mTempSRT[20];
    //char *argv2[];

    enum EventType type = START;
    while (!HeapEmpty(gProcessHeap)) {
        gpCurrentProcess= HeapPop(gProcessHeap);
        /*printf("^^ the current process to be excuted: %d \n", gpCurrentProcess->mId);
        printf("&&& START TIME OF THE CURRENT PROCESS: %d \n", getClk());*/

        ExecuteProcess();
        /*if(getClk() == gpCurrentProcess->mRuntime + gpCurrentProcess->mArrivalTime  ){
            type = FINISH;
        }*/
        AddEvent(START);
//        int pid = fork();
//        if(pid == 0){
//            // in child
//            mTempRunTime =rcv_poped->mRuntime;
//            //itoa(rcv_poped->mRuntime, mTempRunTime, 10);
//            argv2[] = {"process.out", mTempSRT, NULL};//mTempRunTime
//            execv("process.out", argv2);
//        }
//        //Event temp_ev;
//        Event *pEvent = malloc(sizeof(Process));
//        while(!pEvent){
//            printf("There is a problem in allocating for the event, retrying now\n ");
//            pEvent = malloc(sizeof(Process));
//        }
//        pEvent->mCurrentRemTime = 0;
//        pEvent->mCurrentWaitTime=0 ;
//        //pEvent->mpProcess = pProcess;
//        pEvent->mTimeStep = getClk();
//        EventQueueEnqueue(gEventQueue, pEvent);
//        //EventQueueEnqueue(gEventQueue, pEvent);

        //pause();
        ///printf("Loop runs one time here BEfore  wait \n");


        printf("TEST1:  %d\n", getClk());
        wait(NULL);
        printf("TEST2:  %d\n", getClk());
        gpCurrentProcess->mRemainTime = 0;
        AddEvent(FINISH);

        ///printf("Loop runs one time here AFTER  wait \n");

        //printf("READ ONLY THE LAST OF This ==> The clock after the run = %d \n", getClk());
        printf("CURRENT PROC WAITING TIME ==> = %d \n", gpCurrentProcess->mWaitTime);
       /* if(type == START){
            printf("READ ONLY THE LAST OF This ==> START\n");
        }else printf("READ ONLY THE LAST OF This ==> FINISH\n");
*/

       printf("ONE WHILE LOOP FINISHED \n");
    }
    //printf("The clock after the run = %d \n", getClk());
}

void ProcessArrivalHandler(int signum) {
    while(ReceiveProcess()==0);
}

void InitIPC() {
    key_t key = ftok(gFtokFile, gFtokCode);
    gMsgQueueId = msgget(key, 0);
    if (gMsgQueueId == -1) {
        perror("HPF: *** Scheduler IPC init failed");
        raise(SIGINT);
    }
    printf("HPF: *** Scheduler IPC ready!\n");
}

int ReceiveProcess() {
    Message msg;
    if (msgrcv(gMsgQueueId, (void *)&msg, sizeof(msg.mProcess),0, IPC_NOWAIT) == -1){
        perror("HPF: *** Error in receive");
        return -1;
    }
    //msg = msgrcv(gMsgQueueId, (void *)&msg, sizeof(msg.mProcess),0, IPC_NOWAIT);
    printf(" till here, msg is received well in scheduler \n");
    Process *pProcess = malloc(sizeof(Process));
    while(!pProcess){
        printf("There is a problem in allocating memory for the new messages, retrying Now\n");
        pProcess = malloc(sizeof(Process));
    }
    *pProcess= msg.mProcess;
    if(!pProcess){
        printf("Error in allocation of the message\n");
    }

    HeapPush(gProcessHeap, pProcess->mPriority, pProcess);
    printf("Process is now pushed in the Heap \n");
    return 0;

}


void CleanResources(int signum) {
    printf("HPF: *** Cleaning scheduler resources\n");
    Process *pProcess = NULL;
    while (ProcDequeue(gProcessQueue, &pProcess)) //while processes queue is not empty
        free(pProcess); //free memory allocated by this process

    Event *pEvent = NULL;
    while (EventQueueDequeue(gEventQueue, &pEvent)) //while event queue is not empty
        free(pEvent); //free memory allocated by the event
    printf("HPF: *** Scheduler clean!\n");
    exit(EXIT_SUCCESS);
};


void AddEvent(enum EventType type) {
    Event *pEvent = malloc(sizeof(Event));
    while (!pEvent) {
        perror("HPF: *** Malloc failed");
        printf("HPF: *** Trying again");
        pEvent = malloc(sizeof(Event));
    }
    pEvent->mTimeStep = getClk();
    printf("Event has been created and timestep assigned \n");
    if (type == FINISH) {
        pEvent->mTaTime = getClk() - gpCurrentProcess->mArrivalTime;
        pEvent->mWTaTime = (double) pEvent->mTaTime / gpCurrentProcess->mRuntime;
    }
    pEvent->mpProcess = gpCurrentProcess;
    pEvent->mType = type;
    //printf("Current event type is ...\n");
    pEvent->mCurrentRemTime = gpCurrentProcess->mRemainTime;
    //printf("Event is about to be created");
    EventQueueEnqueue(gEventQueue, pEvent);
    //printf("Event inserted successfully in the Queue \n");
    printf("EVENT HAS BEEN ADDED TO QUEUE \n");

    //PrintEvent(pEvent);
}


void ExecuteProcess() {

    gpCurrentProcess->mPid = fork(); //fork a new child and store its pid in the process struct
    while (gpCurrentProcess->mPid == -1) { //if forking fails
        perror("HPF: *** Error forking process");
        printf("HPF: *** Trying again...\n");
        gpCurrentProcess->mPid = fork();
    }
    if (!gpCurrentProcess->mPid) {
        printf("&&& START TIME OF THE CURRENT PROCESS (BEGIN OF CHILD): %d \n", getClk());
        char buffer[10]; //buffer to convert runtime from int to string
        sprintf(buffer, "%d", gpCurrentProcess->mRuntime);
        char *argv[] = {"process.out", buffer, NULL};
        execv("process.out", argv);
        perror("HPF: *** Process execution failed");

        printf("REACHED the END of the CHILD at excute, AKA: a PROCESS RUN \n");
        printf("THE CLK NOW AT CHILD END = %d\n", getClk());

        ///wait(NULL);

        exit(EXIT_FAILURE);
    }
    printf("Now in the Parent of the ExcuteProcess \n");
    gpCurrentProcess->mWaitTime = getClk() - gpCurrentProcess->mArrivalTime;
    /*if(gFirstTimeAddFlag==0){
        gpCurrentProcess->mWaitTime = gpCurrentProcess->mWaitTime + getClk();
        printf("THE ADDED TIME TO THE FIRST RUN TO ADJUST TIME = %d \n", getClk());
        gFirstTimeAddFlag = 1;
    }*/
    printf("^^ the current process that JUST excuted: %d \n", gpCurrentProcess->mId);




}
//    if (gpCurrentProcess->mRuntime == gpCurrentProcess->mRemainTime) { //if this process never ran before
//        gpCurrentProcess->mPid = fork(); //fork a new child and store its pid in the process struct
//        while (gpCurrentProcess->mPid == -1) { //if forking fails
//            perror("HPF: *** Error forking process");
//            printf("HPF: *** Trying again...\n");
//            gpCurrentProcess->mPid = fork();
//        }
//        if (!gpCurrentProcess->mPid) { //if child then execute the process
//            char buffer[10]; //buffer to convert runtime from int to string
//            sprintf(buffer, "%d", gpCurrentProcess->mRuntime);
//            char *argv[] = {"process.out", buffer, NULL};
//            execv("process.out", argv);
//            perror("HPF: *** Process execution failed");
//            exit(EXIT_FAILURE);
//        }
//        //initial wait time for any process is the time at which it started - arrival time
//        gpCurrentProcess->mWaitTime = getClk() - gpCurrentProcess->mArrivalTime;
//        AddEvent(START); //create an event
//    } else { //this process was stopped and now we need to resume it
//        if (kill(gpCurrentProcess->mPid, SIGCONT) == -1) { //continue process
//            perror("HPF: *** Error resuming process");
//            return;
//        }
//        gpCurrentProcess->mWaitTime += getClk() - gpCurrentProcess->mLastStop; //add the additional waiting time
//        AddEvent(CONT);
//


