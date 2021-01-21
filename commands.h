#ifndef COMMANDS_H
#define COMMANDS_H

#include <linux/kernel.h>

#include "structures.h"
#include "global_vars.h"
#include "string_functions.h"
#include "list_interactions.h"


int make_command(char* buff, int length);

#endif
