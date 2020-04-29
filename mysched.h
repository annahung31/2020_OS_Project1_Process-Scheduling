#ifndef MYSCHED_H_
#define MYSCHED_H_

#include <errno.h>
#include <sched.h> // sched_setscheduler()
#include "list.h"

// wait for one unit of time
#define wait_unit(unit) ({ for(size_t i = 0; i < unit; ++i)wait_one_unit; })
// be careful, it is i++ not ++i!
#define wait_one_unit ({ volatile unsigned long i; for(i = 0; i < 1000000UL; i++); })
// convenient way to convert string into policy number
#define policy(S) (POLICY[(int)S%7%4])
// If not in debug mode, uncomment this line.
#define DEBUG

struct ready_queue{
	struct list_head list;
	long start, exe;
	pid_t pid;
};


// sorting function for recording indices
// 1 for ascending, 0 for descending
void swap(int * a, int * b);
void sort(int value[], int index[], int N, int ascend);

void resort(int value[], int index[], int N, int ascend, const int T[], const int T_inverse[]);

void inverse_permutation(const int a[], int b[], int N);

struct ready_queue * find_shortest(struct ready_queue *ready);
void check_terminate(struct ready_queue *ready, struct sched_param *param, unsigned long local_clock, int *flag);
struct ready_queue * check_preempt(struct ready_queue *ready, struct ready_queue *tmp, unsigned long local_clock, int *preempt);
void check_remain(struct ready_queue *ready, struct sched_param *param, unsigned long * local_clock);

#endif
