//#include <stdio.h>
#include <argp.h>
#include <sys/ioctl.h> // ioctl
#include <fcntl.h> // open
#include <unistd.h> // close

#include "uipc.h"

static char doc[] =
  "UIPC - user-level helper function";

/* A description of the arguments we accept. */
static char args_doc[] = "ENTER|TRIGGER";

/* The options we understand. */
static struct argp_option options[] = {
  {"enter", 'e', 0, 0, "enter mwait state" },
  {"trigger", 't', 0, 0, "write to monitored memory to trigger mwait" },
  { 0 }
};

/* Used by main to communicate with parse_opt. */
struct arguments
{
  char *args[1];                /* arg1 & arg2 */
  int choice;
};

/* Parse a single option. */
static error_t
parse_opt (int key, char *arg, struct argp_state *state)
{
  /* Get the input argument from argp_parse, which we
     know is a pointer to our arguments structure. */
  struct arguments *arguments = state->input;

  switch (key)
    {
    case 'e':
      arguments->choice = UIPC_ENTER_MONITOR_MWAIT;
      break;
    case 't':
      arguments->choice = UIPC_TRIGGER_MONITOR;
      break;
    case ARGP_KEY_ARG:
      if (state->arg_num >= 1)
        /* Too many arguments. */
        argp_usage (state);

      arguments->args[state->arg_num] = arg;

      break;

    case ARGP_KEY_END:
      if (state->arg_num < 1)
        /* Not enough arguments. */
        argp_usage (state);
      break;

    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

int main(int argc, char *argv[])
{
    //if (argc != 2) {
    //    printf("Usage:\n./program ENTER|TRIGGER\n");
    //    return 0;
    //}

    /* parse arguments using argp library */
    struct arguments arguments;
    struct argp argp = { options, parse_opt, args_doc, doc };
    argp_parse (&argp, argc, argv, 0, 0, &arguments);

    int fd = open("/dev/uipc-mwait", O_RDWR);
    if (fd < 0) {
        perror("open /dev/uipc-mwait failed.\n");
        return 1;
    }
    close(fd);
    return 0;
}
