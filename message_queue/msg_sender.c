/* The sender program is very similar to msg1.c. In the main set up, delete the
 msg_to_receive declaration and replace it with buffer[MAX_TEXT], remove the message
 queue delete and make the following changes to the running loop.
 We now have a call to msgsnd to send the entered text to the queue. */
 
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <unistd.h>
 
#include <sys/msg.h>
 
#define MAX_TEXT (2)

/* Using 64-bit values saves one instruction clearing the high half of low */
#define DECLARE_ARGS(val, low, high)    unsigned long low, high
#define EAX_EDX_VAL(val, low, high)     ((low) | (high) << 32)
#define EAX_EDX_RET(val, low, high)     "=a" (low), "=d" (high)

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

static int sender_cnt;
 
int main()
{
    int running = 1;
    struct my_msg_st some_data;
    int msgid;
    char buffer[MAX_TEXT];
 
    msgid = msgget((key_t)1234, 0666 | IPC_CREAT);
 
    if (msgid == -1) {
        fprintf(stderr, "msgget failed with error: %d\n", errno);
        exit(EXIT_FAILURE);
    }

    strcpy(some_data.some_text, "t");
    while(running) {
        //printf("Enter some text: ");
        //fgets(buffer, MAX_TEXT, stdin);
        //some_data.my_msg_type = 1;
        //strcpy(some_data.some_text, buffer);
        some_data.my_msg_type = 1;
 
        uint64_t send_tsc = rdtsc();
        if (msgsnd(msgid, (void *)&some_data, MAX_TEXT, 0) == -1) {
            fprintf(stderr, "msgsnd failed\n");
            exit(EXIT_FAILURE);
        }
        printf("[%d] sent in 0x%lx\n", sender_cnt++, send_tsc);        
        if (strncmp(buffer, "end", 3) == 0 || sender_cnt > 1000) {
            running = 0;
        }
    }
 
    exit(EXIT_SUCCESS);
}
