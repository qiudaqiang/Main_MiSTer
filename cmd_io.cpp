
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <signal.h>

#include "cmd_io.h"
#include "user_io.h"
#include "fpga_io.h"
#include "video.h"
#include "input.h"
#include "menu.h"
#include "support.h"

//#include "debug_log.h"

#define SBSEARCH(T, SA, C)	(bsearch(T, SA, sizeof(SA)/sizeof(SA[0]), sizeof(SA[0]), (C)))

static pid_t slave_pid = 0;
static const char* slave_working_directory = "/media/fat/fbmenu";
static const char* slave_command_line[] = {"/media/fat/fbmenu/lua","/media/fat/fbmenu/fbmenu.lua",NULL}; // TODO : read from ini

void cmd_video_cmd(const char* cmd)
{
  video_cmd((char *)cmd);
}

void cmd_load_rbf(const char*cmd)
{
  int len = strlen(cmd);
  if (len > 4 && !strcasecmp(cmd + len - 4, ".mra")) arcade_load(cmd);
  else fpga_load_rbf((char*)cmd);
}

int first_string_compare(const void * a, const void * b)
{
  return strcmp(*(const char**)a, *(const char**)b);
}

static int slave_init(){
  pid_t pid = fork();
  if (pid < 0) return -1; //error
  if (pid > 0) {
    // in parent: child correctly created
    slave_pid = pid;
    return pid;
  }
  // in child: execute command
  chdir(slave_working_directory);
  execv(slave_command_line[0],(char**)slave_command_line);
  // reached only on execv error
  fprintf(stderr, "%s\n", strerror(errno));
  exit(errno);
  return 0;
}

static int slave_check(){
  if (slave_pid == 0) return 0;
  int unknown = kill(slave_pid, 0);
  if (unknown) slave_pid = 0;
  return !unknown;
}

static void slave_resume(){
  MenuHide();
  input_switch(0);
  video_chvt(1);
  video_fb_enable(1);
  slave_check(); // updates slave_pid
  if (slave_pid == 0) slave_init();
  else kill(slave_pid, SIGCONT);
}

static void slave_suspend(){
  video_fb_enable(0);
  input_switch(1);
  if (slave_check()) // updates slave_pid
  {
    kill(slave_pid, SIGSTOP);
  }
}

static int slave_running = 0;

int is_slave_enable(){
  return slave_running;
}

void slave_enable(int status){
  switch (status) {
  break;case 0:
    slave_running = 0;
    slave_suspend();
  break;default:
    slave_running = 1;
    slave_resume();
  }
}

static void cmd_slave_enable(const char* cmd){
  switch (*cmd) {
  break;case '!': slave_enable(!is_slave_enable());
  break;case '0': slave_enable(0);
  break;case '1': slave_enable(1);
  }
}

struct cmdentry
{
  const char * name;
  void (*cmd)(const char*);
};
static void handle_single_cmd(char*cmd)
{
  static struct cmdentry cmdlist[] =
  {
  // The "name" field can not contain ' ' or '\0'.
  // The array must be lexicographically sorted wrt "name" field (e.g.
  //   :sort vim command, but mind '!' and escaped chars at end of similar names).
    {"fb_cmd",       cmd_video_cmd},
    {"load_core",    cmd_load_rbf},
    {"slave_enable", cmd_slave_enable},
  };

  int namelen;
  for (namelen = 0; ; namelen += 1)
    if (cmd[namelen] == ' '||cmd[namelen] == '\0')
      break;
  if (namelen <= 0) return;
  char name[namelen+1];
  strncpy(name, cmd, namelen);
  name[namelen] = '\0';
  struct cmdentry target = {name, 0};
  struct cmdentry * command = (struct cmdentry*)
    SBSEARCH(&target, cmdlist, first_string_compare);
  if (!command)
  {
    printf("invalid MiSTer command: %s", cmd);
    return;
  }
  printf("MiSTer command: %s", cmd);
  while (cmd[namelen] == ' ')
    namelen += 1;
  command->cmd(cmd+namelen);
}

void handle_MiSTer_cmd(int cmdfd)
{
  static char CMD[1024];
  char* cmd = CMD;
  int len = read(cmdfd, CMD, sizeof(CMD) - 1);
  if (len)
  {
    if (cmd[len - 1] == '\n') cmd[len - 1] = 0;
    cmd[len] = 0;

    // Split lines in multiple commands
    while (cmd && *cmd != '\0') {
      char * next = 0;
      for (int c = 0; cmd[c] != '\0'; c += 1) if (cmd[c] == '\n') {
        cmd[c] = '\0';
        next = cmd + c + 1;
        break;
      }

      handle_single_cmd(cmd);

      if (!next) break;
      cmd = next;
    }
  }
}

