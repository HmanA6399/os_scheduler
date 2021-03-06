#include "signal.h"
#include "lib/clock.h"
#include "lib/ipc.h"
#include "lib/remaining_time.h"

/* Modify this file as needed*/
int remainingtime, old_clock, cur_clock;
bool stop = false;
void clearIPC(int signum)
{
    destroyRemainingTimeCommunication(false);
    destroyClk(false);
    exit(0);
    signal(SIGINT, clearIPC);
}

void setClock(int signum)
{
    //print remaning time
    //printf("current remainingtime in p %d  in schedular %d \n", remainingtime, getReminingTime());
    remainingtime = getReminingTime();
    stop = true;
    old_clock = getClk();
    cur_clock = getClk();
    //printf("do that old %d new %d \n", old_clock, cur_clock);
    signal(SIGCONT, setClock);
}
void had(int signum)
{
    printf("take signal now !");
}

int main(int agrc, char *argv[])
{
    initClk();
    // Initialize shared memory for remaining time
    // Handle SIGUSR1 sent by the scheduler on pre-emption
    signal(SIGCONT, setClock);
    signal(SIGUSR1, had);
    signal(SIGINT, clearIPC);
    initRemainingTimeCommunication(false /*i.e. not the creator*/);
    // Initially get the value  of remaining time
    remainingtime = atoi(argv[1]);
    printf("processID %d real %d  start at %d remanningTime %d \n", atoi(argv[2]), getpid(), getClk(), atoi(argv[1]));
    // Use clock changes to change the remaining time
    old_clock = getClk();
    cur_clock = getClk();

    while (remainingtime > 0)
    {
        // Load current clock
        cur_clock = getClk();
        stop = false;
        // Change remaining time only when clock has changed
        if ((cur_clock > old_clock))
        {
            if (!stop)
            {
                remainingtime -= 1;
            }
        }
        // Update clocks
        old_clock = cur_clock;
    }
    destroyRemainingTimeCommunication(false);
    destroyClk(false);
    return 0;
}
