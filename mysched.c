#include <stdio.h>
#include <semaphore.h>
#include <assert.h>
#include <stdlib.h>
#include <math.h>
#include "mysched.h"
#include "list.h"


// sorting function
// 1 for ascending, 0 for descending
void swap(int * a, int * b){
	int c = *a;
	*a = *b;
	*b = c;
}

void sort(int value[], int index[], int N, int ascend){
	if(ascend){
		for(unsigned i = 0; i < N; ++i){
			for(unsigned j = i + 1; j < N; ++j){
				if(value[i] > value[j]){
					swap(&value[i], &value[j]);
					swap(&index[i], &index[j]);
				}
			}
		}
	}
	else{
		for(unsigned i = 0; i < N; ++i){
			for(unsigned j = i + 1; j < N; ++j){
				if(value[i] < value[j]){
					swap(&value[i], &value[j]);
					swap(&index[i], &index[j]);
				}
			}
		}
	}
}

void inverse_permutation(const int a[], int b[], int N){
	int count = 0, cursor = 0, probe = 0;
	for(unsigned i = 0; i < N; ++i)
		b[i] = -1;
	
	while(count < N){
		if(b[a[probe]] == -1){
			b[a[probe]] = probe;
			probe = a[probe];
			++count;
		}
		else{
			for(; cursor < N; ++cursor)
				if(b[cursor] == -1){
					probe = cursor;
					break;
				}
		}
	}
}


void resort(int value[], int index[], int N, int ascend, const int T[], const int T_inverse[]){
        unsigned head = 0, end = 0;
        if(ascend){
                while(end + 1 < N){
                        if(value[head] == value[end + 1]){
                                ++end;
                        }
                        if(value[head] != value[end + 1] || end + 1 == N){
                                for(unsigned i = head; i <= end; ++i){
                                        for(unsigned j = head; j <= end; ++j){
                                                if(T[T_inverse[index[i]]] < T[T_inverse[index[j]]]){

                                                        
                                                        swap(&index[i], &index[j]);
                                                }
                                        }
                                }
                                head = (++end);
                        }
                }
        }
        else{
                while(end + 1 < N){
                        if(value[head] == value[end + 1]){
                                ++end;
                        }
                        if(value[head] != value[end + 1] || end + 1 == N){
                                for(unsigned i = head; i < end; ++i){
                                        for(unsigned j = head + 1; j <= end; ++j){
                                                if(T[T_inverse[index[i]]] > T[T_inverse[index[j]]]){
                                                        
                                                        swap(&index[i], &index[j]);
                                                }
                                        }
                                }
                                head = (++end);
                        }
                }
        }
}

struct ready_queue * find_shortest(struct ready_queue *ready){
	long long shortest = 100000000000; 
	struct list_head *shortest_pos, *pos;
	struct ready_queue *tmp;
	list_for_each(pos, &(ready->list)){
		tmp = list_entry(pos, struct ready_queue, list);
		if( tmp->exe < shortest ){
			shortest = tmp->exe;
			shortest_pos = pos;
		}
	}
	tmp = list_entry(shortest_pos, struct ready_queue, list);
	list_del(&(tmp->list));

	list_add(&(tmp->list), &(ready->list));
	
	return tmp;
}

void check_terminate(struct ready_queue *ready, struct sched_param *param, unsigned long local_clock, int *flag){
	struct ready_queue *tmp;
	
	// get the running child.
	tmp = list_entry(ready->list.next, struct ready_queue, list);

	//remove terminated child and add the shortest one to head
	
	if(tmp->start + tmp->exe == local_clock){
		
		list_del(&(tmp->list));
		if(!list_empty(&ready->list)){
			
			tmp = find_shortest(ready);
			
			tmp->start = local_clock;

			if(tmp->pid){
		 		if(sched_setscheduler(tmp->pid, SCHED_FIFO, param)){
					printf("3 sched_setscheduler error: %s\n", strerror(errno));
					exit(1);
				}
			}
			else{
				*flag = 1;
			}
		}
	}
}
struct ready_queue * check_preempt(struct ready_queue *ready, struct ready_queue *tmp, unsigned long local_clock, int * preempt){
	struct ready_queue *tmp1;
	
	tmp1 = list_entry(ready->list.next, struct ready_queue, list);

	if (tmp == tmp1){
		*preempt = 1;
	}
	else{
		if(tmp1->start + tmp1->exe - local_clock > tmp->exe){
			tmp1->exe = tmp1->start + tmp1->exe - local_clock;

			tmp1->start = -1;
			*preempt = 1;
			list_del(&(tmp->list));

			list_add(&(tmp->list), &(ready->list));
			
			tmp->start = local_clock;
		}
	}
	
	return tmp1;
}

void check_remain(struct ready_queue *ready, struct sched_param *param, unsigned long * local_clock){
	struct ready_queue *tmp;
	tmp = list_entry(ready->list.next, struct ready_queue, list);

	
	while(*local_clock < tmp->start + tmp->exe){
		wait_one_unit;
		++(*local_clock);
	}
	
	list_del(&(tmp->list));
	if(!list_empty(&ready->list)){	
		tmp = find_shortest(ready);
		tmp->start = *local_clock;


		if(sched_setscheduler(tmp->pid, SCHED_FIFO, param)){
			printf("3 sched_setscheduler error: %s\n", strerror(errno));
			exit(1);
		}
	}
}
