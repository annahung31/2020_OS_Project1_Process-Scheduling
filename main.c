#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <unistd.h> // used fork to produce child process
#include <sched.h> 
#include <sys/time.h>
#include <linux/kernel.h>
#include <sys/syscall.h>
#include "list.h" 
#include "mysched.h"


// 1 for FIFO, 2 for RR
int POLICY[2] = {1, 2};

int main(){
	#ifdef DEBUG
	setbuf(stdout, NULL);
	#endif
	
	// read the process info from input file
	char S[4];
	int N;
	scanf("%s\n%d", S, &N);
	
	char P[N][32];
	int R[N], T[N];
	for(size_t i = 0; i < N; ++i){
		scanf("%s %d %d", P[i], &R[i], &T[i]);
	}


	
	// cpu set up
	cpu_set_t mask, mask1;

	CPU_ZERO(&mask);
	CPU_SET(0, &mask);
	
	CPU_ZERO(&mask1);
	CPU_SET(1, &mask1);

	
	// setup scheduler, param for FIFO,RR; param0 for others
	struct sched_param param, param0;
	param.sched_priority = sched_get_priority_max((S[0] == 'R') ? SCHED_RR : SCHED_FIFO);
	param0.sched_priority = 0;
	
	
	if(S[0] == 'F' || S[0] == 'R'){
		if(sched_setscheduler(0, policy(S[0]), &param)){
			printf("1 sched_setscheduler error: %s\n", strerror(errno));
			exit(1);
		}
	}
	
	//printf("scheduler:  %d \n",  param.sched_priority);	
  	// sort the ready and execution time
        int R_index[N], T_index[N], R_inverse[N], T_inverse[N];
        for(size_t i = 0; i < N; ++i)
                R_index[i] = T_index[i] = i;

	sort(R, R_index, N, 1);
	sort(T, T_index, N, 1);
	inverse_permutation(R_index, R_inverse, N);
	inverse_permutation(T_index, T_inverse, N);
	if(S[0] == 'S' || S[0] == 'P')
		resort(R, R_index, N, 1, T, T_inverse);
	
	// get ready
	pid_t pid;
	unsigned long local_clock = 0;
	unsigned long i = 0;
	struct timeval start;
	struct timespec start_n, end_n;
	struct ready_queue ready, *tmp, *tmp1;
	
	INIT_LIST_HEAD(&ready.list);
	
	for(; i < N; ++i){
		int empty, preempt = 0;
		int this_pid, flag = 0;
		
		// wait until next child to be forked
		while(local_clock != R[i]){
			if((S[0] == 'S' || S[0] == 'P') && !list_empty(&ready.list))
				check_terminate(&ready, &param, local_clock, &flag);
			
			wait_one_unit;
			++local_clock;
		}
		
		// check if ready queue is empty		
		empty = list_empty(&ready.list);
		// add to ready queue
		tmp = (struct ready_queue*)malloc(sizeof(struct ready_queue));
		tmp->start = (empty) ? R[i] : -1;
		tmp->exe = T[T_inverse[R_index[i]]];
		list_add_tail(&(tmp->list), &(ready.list));
		
		if(S[0] == 'S' || S[0] == 'P'){
			if(!list_empty(&ready.list)){
				check_terminate(&ready, &param, local_clock, &flag);
				if(S[0] == 'P'){
					tmp1 = check_preempt(&ready, tmp, local_clock, &preempt);
				}
			}
		}
		//printf("Pid: %d \n", &this_pid);	
		//pid = fork();
		//this_pid = getpid();
		syscall(335, 1, &start_n.tv_sec, &start_n.tv_nsec, &end_n.tv_sec, &end_n.tv_nsec, &this_pid);   // 1 for start time
		pid = fork();
		
		if(!pid){
			this_pid = getpid();
			printf("Process: %s %d \n", P[R_index[i]], this_pid);
			
			for(unsigned long _i = 0; _i < T[T_inverse[R_index[i]]]; ++_i){
				wait_one_unit;
			}
	
			syscall(335, 0, &start_n.tv_sec, &start_n.tv_nsec, &end_n.tv_sec, &end_n.tv_nsec, &this_pid); // 0 for end time
			exit(0);
		}
		else if(pid == -1){
			printf("Fork error!\n");
			exit(1);
		}
		else{
			tmp->pid = pid;
			
			
			if(sched_setaffinity(pid, sizeof(cpu_set_t), &mask)){
				printf("sched_setaffinity error: %s\n", strerror(errno));
				exit(1);
			}
			if(flag){
				if(sched_setscheduler(pid, SCHED_FIFO, &param)){
					printf("policy: %d, sched_setscheduler error: %s\n", SCHED_FIFO, strerror(errno));
					exit(1);
				}
			}
			else{
				if(S[0] == 'S' || S[0] == 'P'){
					if(empty){
						if(sched_setscheduler(pid, SCHED_FIFO, &param)){
							printf("policy: %d, sched_setscheduler error: %s\n", SCHED_FIFO, strerror(errno));
							exit(1);
						}
					}
					else{
						if(S[0] == 'P' && preempt){
							
							
							if(sched_setscheduler(pid, SCHED_FIFO, &param)){
								printf("policy: %d, sched_setscheduler error: %s\n", SCHED_FIFO, strerror(errno));
								exit(1);
							}
							if(sched_setscheduler(tmp1->pid, SCHED_IDLE, &param0)){
								printf("policy: %d, sched_setscheduler error: %s\n", SCHED_IDLE, strerror(errno));
								exit(1);
							}
		      			}
						else{
							if(list_entry(ready.list.next, struct ready_queue, list)->pid == pid){
								if(sched_setscheduler(pid, SCHED_IDLE, &param0)){
									printf("policy: %d, sched_setscheduler error: %s\n", SCHED_IDLE, strerror(errno));
									exit(1);
								}
							}
						}
					}
				}
			}
			
			if((S[0] == 'S' || S[0] == 'P') && !list_empty(&ready.list))
				check_terminate(&ready, &param, local_clock, &flag);
		}
	}
	
	//consume the remaining children which are still in idle after list empty
	while(!list_empty(&ready.list) && (S[0] == 'S' || S[0] == 'P')){
		check_remain(&ready, &param, &local_clock);
	}

	//wait for the last child process to terminate
	while(wait(NULL) > 0);
	
	exit(0);
}

