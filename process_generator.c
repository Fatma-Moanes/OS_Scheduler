#include "Headers/headers.h"
#include "Headers/ProcessQueue.h"
#include "Headers/ProcessHeap.h"
#include "Headers/MessageBuffer.h"
#include <string.h>

void ClearResources(int);

void ReadFile();

void InitIPC();

int GetUserChoice();

void ExecuteClock();

void ExecuteScheduler(int);

void SendProcess(Process *);

queue gProcessQueue;
int gMsgQueueId = 0;
pid_t gClockPid = 0;
pid_t gSchedulerPid = 0;

int main(int argc, char *argv[]) {
    //initialize the process queue
    gProcessQueue = NewProcQueue();
    //catch SIGINT
    signal(SIGINT, ClearResources);
    //initialize the IPC
    InitIPC();
    // TODO Initialization
    // 1. Read the input files.
    ReadFile();
    // 2. Ask the user for the chosen scheduling algorithm and its parameters, if there are any.
    int choice = GetUserChoice(); //1 = HPF, 2 = SRTN, 3 = RR
    while (choice < 1 || choice > 3) {
        printf("PG: *** Invalid choice\n");
        choice = GetUserChoice();
    }
    // 3. Initiate and create the scheduler and clock processes.
    ExecuteClock();
    ExecuteScheduler(choice);
    // 4. Use this function after creating the clock process to initialize clock
    initClk();
    // To get time use this
    // TODO Generation Main Loop
    while (!ProcQueueEmpty(gProcessQueue)) {
        //get current time
        int current_time = getClk();
        //temporary process pointer
        Process *pTempProcess;
        //peek the processes queue
        ProcPeek(gProcessQueue, &pTempProcess);
        //keep looping as long as the process on top has arrival time equal to the current time
        bool is_time = false; //flag to indicate whether at least one process matches current time or not
        while (pTempProcess->mArrivalTime == current_time) {
            is_time = true;
            SendProcess(pTempProcess); //send this process to the scheduler
            ProcDequeue(gProcessQueue, &pTempProcess); //dequeue this process from the processes queue
            free(pTempProcess); //free memory allocated by this process
            ProcPeek(gProcessQueue, &pTempProcess); //peek the next process in the queue
        }
        if (is_time) //if at least one process was sent to the scheduler
            kill(gSchedulerPid, SIGUSR1); //send SIGUSR1 to the scheduler
        usleep(900000); //sleep 0.9 sec
    }
    // invoke ClearResources() but use zero as parameter to indicate normal exit not interrupt
    ClearResources(0);
}

int GetUserChoice() {
    int decision;
    printf("PG: *** Choose scheduling algorithm:\n");
    printf("1. Highest Priority First\n2. Shortest Remaining Time Next\n3. Round Robin\n");
    scanf("%d", &decision);
    return decision;
}

void ClearResources(int signum) {
    //TODO Clears all resources in case of interruption
    //Clear IPC resources
    if (gMsgQueueId != 0) {
        printf("PG: *** Cleaning IPC resources...\n");
        if (msgctl(gMsgQueueId, IPC_RMID, NULL) == -1)
            perror("PG: *** Error");
        else
            printf("PG: *** IPC cleaned!\n");
    }

    //free queue memory
    printf("PG: *** Cleaning processes queue...\n");
    Process *pTemp;
    while (ProcDequeue(gProcessQueue, &pTemp)) {
        free(pTemp);
    }
    printf("PG: *** Process queue cleaned!\n");

    int status;
    //if this function is invoked due to an interrupt signal then immediately interrupt all processes
    if (signum == SIGINT) {
        //Interrupt forked processes
        printf("PG: *** Sending interrupt to scheduler\n");
        if (gSchedulerPid)
            kill(gSchedulerPid, SIGINT);
        printf("PG: *** Sending interrupt to clock\n");
        if (gClockPid) {
            destroyClk(false);
            kill(gClockPid, SIGINT);
        }
        //do not leave before both forked processes are done
        wait(&status);
        wait(&status);
    } else { //we need to wait until Scheduler exits by itself
        printf("PG: *** Waiting for scheduler to do its job...\n");
        waitpid(gSchedulerPid, &status, 0); //wait until scheduler exits
        printf("PG: *** Scheduler exit signal received\n");
        printf("PG: *** Sending interrupt to clock\n");
        if (gClockPid) {
            destroyClk(false);
            kill(gClockPid, SIGINT);
        }
        //do not leave before clock is done
        wait(&status);
    }
    printf("PG: *** Clean!\n");
    exit(EXIT_SUCCESS);
}

void ReadFile() {
    printf("PG: *** Attempting to open input file...\n");
    FILE *fp;
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    fp = fopen("processes.txt", "r");
    if (fp == NULL) {
        perror("PG: *** Error reading from input file");
        exit(EXIT_FAILURE);
    }
    printf("PG: *** Reading input file...\n");
    while ((read = getline(&line, &len, fp)) != -1) {
        if (line[0] == '#') {
            printf("PG: *** Hash detected, skipping line\n");
            continue;
        }

        Process *pProcess = malloc(sizeof(Process));
        while (!pProcess) {
            perror("PG: *** Malloc failed");
            printf("PG: *** Trying again");
            pProcess = malloc(sizeof(Process));
        }

        pProcess->mId = atoi(strtok(line, "\t"));
        pProcess->mArrivalTime = atoi(strtok(NULL, "\t"));
        pProcess->mRuntime = atoi(strtok(NULL, "\t"));
        pProcess->mPriority = atoi(strtok(NULL, "\t"));
        pProcess->mRemainTime = 0;
        pProcess->mWaitTime = 0;
        printf("PG: *** Process read with the following:\n");
        PrintProcess(pProcess);
        ProcEnqueue(gProcessQueue, pProcess);
        printf("PG: *** Above process added to queue!\n");
    }

    printf("PG: *** Releasing file resources...\n");
    fclose(fp);
    if (line)
        free(line);
    printf("PG: *** Input file done successfully!\n");
}

void InitIPC() {
    key_t key = ftok(gFtokFile, gFtokCode);
    gMsgQueueId = msgget(key, IPC_CREAT | 0666);
    if (gMsgQueueId == -1) {
        perror("PG: *** IPC init failed");
        raise(SIGINT);
    }
    printf("PG: *** IPC ready!\n");
}

void ExecuteClock() {
    gClockPid = fork();
    while (gClockPid == -1) {
        perror("PG: *** Error forking clock");
        printf("PG: *** Trying again...\n");
        gClockPid = fork();
    }
    if (gClockPid == 0) {
        printf("PG: *** Clock forking done!\n");
        printf("PG: *** Executing clock...\n");
        char *argv[] = {"clk.out", 0};
        execv("clk.out", argv);
        exit(EXIT_SUCCESS);
    }

}

void ExecuteScheduler(int type) {
    gSchedulerPid = fork();
    while (gSchedulerPid == -1) {
        perror("PG: *** Error forking scheduler");
        printf("PG: *** Trying again...\n");
        gSchedulerPid = fork();
    }
    if (gSchedulerPid == 0) {
        printf("PG: *** Scheduler forking done!\n");
        printf("PG: *** Executing scheduler...\n");
        char *argv[2];
        argv[1] = 0;
        switch (type) {
            case 1:
                argv[1] = "hpf.out";
                execv("hpf.out", argv);
                break;
            case 2:
                argv[1] = "srtn.out";
                execv("srtn.out", argv);
                break;
            case 3:
                argv[1] = "roundrobin.out";
                execv("roundrobin.out", argv);
                break;
            default:
                break;
        }
        exit(EXIT_SUCCESS);
    }
}

void SendProcess(Process *pProcess) {
    Message msg;
    msg.mType = 10; //arbitrary value for mType
    msg.mProcess = *pProcess;
    printf("PG: *** Sending process with id %d to scheduler...\n", msg.mProcess.mId);
    if (msgsnd(gMsgQueueId, (void *) &msg, sizeof(msg.mProcess), !IPC_NOWAIT) == -1) {
        perror("PG: *** Error while sending process");
    } else {
        printf("PG: *** Process sent!\n");
    }
}