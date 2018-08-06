/* Here's the receiver program. */
 
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <unistd.h>
 
#include <sys/msg.h>

#include <sys/ipc.h> // IPC_RMID
#include <sys/shm.h> // IPC_RMID

#define MAX_TEXT (2)

/* Using 64-bit values saves one instruction clearing the high half of low */
#define DECLARE_ARGS(val, low, high)	unsigned long low, high
#define EAX_EDX_VAL(val, low, high)	((low) | (high) << 32)
#define EAX_EDX_RET(val, low, high)	"=a" (low), "=d" (high)

static __always_inline unsigned long long rdtsc(void)
{
	DECLARE_ARGS(val, low, high);

	asm volatile("rdtsc" : EAX_EDX_RET(val, low, high));

	return EAX_EDX_VAL(val, low, high);
}
 
struct my_msg_st {
    long int my_msg_type;
    char some_text[MAX_TEXT];
};

static int receiver_cnt;
 
int main()
{
    int running = 1;
    int msgid;
    struct my_msg_st some_data;
    long int msg_to_receive = 0;
 
/* First, we set up the message queue. */
 
    msgid = msgget((key_t)1234, 0666 | IPC_CREAT);
 
    if (msgid == -1) {
        fprintf(stderr, "msgget failed with error: %d\n", errno);
        exit(EXIT_FAILURE);
    }
 
/* Then the messages are retrieved from the queue, until an end message is encountered.
 Lastly, the message queue is deleted. */
 
    while(running) {
        if (msgrcv(msgid, (void *)&some_data, MAX_TEXT,
                   msg_to_receive, 0) == -1) {
            fprintf(stderr, "msgrcv failed with error: %d\n", errno);
            exit(EXIT_FAILURE);
        }
        uint64_t rcv_tsc = rdtsc();
        printf("[%d] 0x%lx You wrote: %s\n", receiver_cnt++, rcv_tsc, some_data.some_text);
        if (strncmp(some_data.some_text, "end", 3) == 0) {
            running = 0;
        }
    }
 
    if (msgctl(msgid, IPC_RMID, 0) == -1) {
        fprintf(stderr, "msgctl(IPC_RMID) failed\n");
        exit(EXIT_FAILURE);
    }
 
    exit(EXIT_SUCCESS);
}
