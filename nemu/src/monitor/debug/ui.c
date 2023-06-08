#include "monitor/monitor.h"
#include "monitor/expr.h"
#include "monitor/watchpoint.h"
#include "nemu.h"

#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>

void cpu_exec(uint64_t);

/* We use the `readline' library to provide more flexibility to read from stdin. */
char* rl_gets() {
  static char *line_read = NULL;

  if (line_read) {
    free(line_read);
    line_read = NULL;
  }

  line_read = readline("(nemu) ");

  if (line_read && *line_read) {
    add_history(line_read);
  }

  return line_read;
}

static int cmd_c(char *args) {
  cpu_exec(-1);
  return 0;
}

static int cmd_q(char *args) {
  return -1;
}

static int cmd_help(char *args);
static int cmd_si(char *args);
static int cmd_info(char *args);
static int cmd_x(char* args);
static int cmd_p(char* args);
static int cmd_w(char* args);
static int cmd_d(char* args);

static int cmd_si(char *args);

static int cmd_info(char *args);

static int cmd_x(char *args);

static struct {
  char *name;
  char *description;
  int (*handler) (char *);
} cmd_table [] = {
  { "help", "Display informations about all supported commands", cmd_help },
  { "c", "Continue the execution of the program", cmd_c },
  { "q", "Exit NEMU", cmd_q },
  { "si", "Single step", cmd_si},
  { "info", "Print info", cmd_info},
  { "x", "Scan memory", cmd_x},
  { "p", "Calculate experssion", cmd_p},
  { "w", "Set watchpoint", cmd_w},
  { "d", "Delete watchpoint", cmd_d}
};

#define NR_CMD (sizeof(cmd_table) / sizeof(cmd_table[0]))

static int cmd_help(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL) {
    /* no argument given */
    for (i = 0; i < NR_CMD; i ++) {
      printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  }
  else {
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(arg, cmd_table[i].name) == 0) {
        printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf("Unknown command '%s'\n", arg);
  }
  return 0;
}

static int cmd_si(char *args) {
  char *arg = strtok(args, " ");
  int step = 0;
  if(arg == NULL) {
    // arg defalut value = 1
    cpu_exec(1);
    return 0;
  }
  sscanf(arg, "%d", &step);
  cpu_exec(step);
  return 0;
}

static int cmd_info(char *args) {
  char* arg = strtok(args, " ");
  if(strcmp(arg, "r") == 0) {
    // 32 bit reg
    for(int i = 0; i < 8; i++) {
      printf("%s : 0x%x\n", reg_name(i, 4), reg_l(i));
    }
    printf("eip : 0x%x\n", cpu.eip);
    // 16 bit reg
    for(int i = 0; i < 8; i++) {
      printf("%s : 0x%x\n", reg_name(i, 2), reg_l(i));
    }
    // 8 bit reg
    for(int i = 0; i < 8; i++) {
      printf("%s : 0x%x\n", reg_name(i, 1), reg_l(i));
    }
    printf("CR0=0x%x, CR3=0x%x\n", cpu.CR0, cpu.CR3);
  }
  else if(strcmp(arg, "w") == 0) {
    show_wp(); 
  }
  return 0;
}

static int cmd_x(char *args)
{
  char *N = strtok(NULL, " ");
  char *EXPR = strtok(NULL, " ");
  int len;
  vaddr_t addr;
  sscanf(N, "%d", &len);
  sscanf(EXPR, "%x", &addr);
  printf("0x%x:", addr);
  for (int i = 0; i < len; i++) {
    printf("%08x ", vaddr_read(addr, 4));
    addr += 4;
  }
  printf("\n");
  return 0;
}

static int cmd_p(char* args) {
  char* arg = strtok(args, " ");
  bool success = true;
  int value = expr(arg, &success);
  if(success) {
    printf("ans = 0x%08x\n", value);
  }
  else {
    printf("Illegal Expression!\n");
  }
  return 0;
}

static int cmd_w(char* args) {
  char* arg = strtok(NULL, "\n");
  WP* wp = new_wp();
  strcpy(wp->expr, arg);
  bool success = true;
  wp->value = expr(arg, &success);
  if(success){
    printf("watchpoint %d : %s\n", wp->NO, wp->expr);
  }
  else{
    printf("Illegal Expression!\n");
    free_wp(wp->NO);
  }
  return 0;
}

static int cmd_d(char* args) {
  char* arg = strtok(NULL, "\n");
  free_wp(atoi(arg));
  return 0;
}

void ui_mainloop(int is_batch_mode) {
  if (is_batch_mode) {
    cmd_c(NULL);
    return;
  }

  while (1) {
    char *str = rl_gets();
    char *str_end = str + strlen(str);

    /* extract the first token as the command */
    char *cmd = strtok(str, " ");
    if (cmd == NULL) { continue; }

    /* treat the remaining string as the arguments,
     * which may need further parsing
     */
    char *args = cmd + strlen(cmd) + 1;
    if (args >= str_end) {
      args = NULL;
    }

#ifdef HAS_IOE
    extern void sdl_clear_event_queue(void);
    sdl_clear_event_queue();
#endif

    int i;
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(cmd, cmd_table[i].name) == 0) {
        if (cmd_table[i].handler(args) < 0) { return; }
        break;
      }
    }

    if (i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
  }
}
