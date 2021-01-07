#include "lib/clock.h"
#include "lib/ipc.h"
#include "lib/data_structures.h"
#include "lib/process_management.h"
#include "lib/remaining_time.h"
/////////////////////////////global variables ////////////////
int msg_q_id, finished = 0, t_rem; // type should change with argv
bool flag_in_run = false, p_need_fill = false, set_Data = false, comp = true;

///////////////////functions handle signals/////////////////
void handler(int signum)
{
    finished = 1;
    signal(SIGUSR1, handler);
}

void endProcess(int signum)
{
    p_need_fill = true; // then in while change all values
    flag_in_run = false;
    signal(SIGCHLD, endProcess);
}
void stopProcess(int sinum)
{
    if (comp)
        t_rem = getReminingTime();
    else
    {
        flag_in_run = false;
        set_Data = true; //set data & stop process again
    }
    signal(SIGUSR2, stopProcess);
}
///////////////////////testing function //////////////////////
void doneFun(PriorityQueue *PQ, int v)
{
    PriorityQueue__push(PQ, &v, v * 30);
}
void creatPQ(PriorityQueue *PQ)
{
    int y = 9;
    for (int i = 0; i < 8; i++)
    {
        int u = y;
        doneFun(PQ, u);
        y--;
    }
    while (!PriorityQueue__isEmpty(PQ))
    {
        int *a = PriorityQueue__pop(PQ);
        printf("%d\n", *a);
    }
}
/////////////////////////using function/////////////
void checkComeShortest(PriorityQueue *PQ, int current_child_pid)
{
    PCB *process = PriorityQueue__peek(PQ);
    if (process->t_remaining < t_rem)
    {
        kill(current_child_pid, SIGUSR1);
        comp = false;
    }
}
void intialProcess(ProcessData *p, PCB *process)
{
    process->p_data = *p;
    process->state = IDLE;
    process->t_remaining = p->t_running;
    process->t_st = -1;
    process->actual_pid = -1;
    process->t_ta = -1;
    process->t_w = -1;
}

void insert_all_comming_in_PQueue(PriorityQueue *PQ, int type)
{
    int pid = 0;
    while (pid != -1)
    {
        ProcessData recievedProcess = recieveProcessMessage(msg_q_id, SCHEDULER_TYPE);
        pid = recievedProcess.pid;
        if (pid != -1)
        {
            PCB process;
            intialProcess(&recievedProcess, &process);

            if (type == 0)
            {
                long long p = recievedProcess.priority;
                PriorityQueue__push(PQ, &process, (p << 32) | recievedProcess.t_arrival);
            }
            else if (type == 1) ///sRTN
                PriorityQueue__push(PQ, &process, recievedProcess.t_running);
        }
    }
}
////////////////////////////////////////////

int main(int argc, char *argv[])
{
    PCB *current_process; // current process in run
    PriorityQueue *PQ = PriorityQueue__create(0), *i = PriorityQueue__create(0);
    initRemainingTimeCommunication(true);
    int type = atoi(argv[1]);
    initClk();
    signal(SIGCHLD, endProcess);
    signal(SIGUSR1, handler);
    signal(SIGUSR2, stopProcess);
    msg_q_id = getProcessMessageQueue(KEYSALT);
    printf("Recieved queue with id %d\n", msg_q_id);
    creatPQ(i);
    while (1)
    {
        if (p_need_fill)
        {
            flag_in_run = false;
            p_need_fill = false;
            // set all need values
        }
        else if (set_Data)
        {
            // another status
            current_process->state = STOPPED;
            current_process->t_remaining = getReminingTime();
            flag_in_run = false;
            PriorityQueue__push(PQ, current_process, current_process->t_remaining);
        }
        if (!flag_in_run && !PriorityQueue__isEmpty(PQ))
        {
            current_process = PriorityQueue__pop(PQ);
            current_process->t_st = getClk(); // set all others
            flag_in_run = true;
            comp = true;
            t_rem = current_process->t_remaining;
            if (current_process->actual_pid == -1 || type == 0)
                current_process->actual_pid = createChild("./process.out", current_process->t_remaining, 0); // anything else process nee
            else
                kill(current_process->actual_pid, SIGUSR2);
        }
        if (finished != 2 && (type != 0 || !flag_in_run))
        {
            if (finished == 1)
                finished == 2;
            insert_all_comming_in_PQueue(PQ, type);
            if (type == 1)
                checkComeShortest(PQ, current_process->actual_pid);
        }
    }

    destroyClk(false);
}
