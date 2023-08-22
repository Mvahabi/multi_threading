#include <assert.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <regex.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <stdbool.h>
#include "asgn2_helper_funcs.h"
//#include "request.h"

#define BUFFER_SIZE      4096
#define MAX_REQUEST_SIZE 2048

int echo(int x);
