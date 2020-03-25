#include "Headers/headers.h"
#include "Headers/ProcessStruct.h"
#include "Headers/ProcessHeap.h"
#include "Headers/MessageBuffer.h"

void ProcessArrivalHandler(int);

void InitIPC();

int ReceiveProcess();

void CleanResources();

int gMsgQueueId = 0;

heap_t *gProcessHeap;

int main(int argc, char *argv[]) {
    printf("SRTN: *** Scheduler here\n");
    initClk();
    InitIPC();
    //initialize processes heap
    gProcessHeap = (heap_t *) calloc(1, sizeof(heap_t));
    signal(SIGUSR1, ProcessArrivalHandler);
    signal(SIGINT, CleanResources);

    while (1) {
        pause();
    }
}

void ProcessArrivalHandler(int signum) {

}

void InitIPC() {

}

int ReceiveProcess() {

}

void CleanResources() {

};
