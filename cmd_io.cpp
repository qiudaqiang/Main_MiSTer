
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include "cmd_io.h"
#include "user_io.h"
#include "fpga_io.h"
#include "video.h"

void cmd_load_rbf(char*cmd)
{
  fpga_load_rbf(cmd);
}

int cmdutil_read_enable_param(char*cmd)
{
  uint32_t set = 0;
  if (1 != sscanf(cmd, "%d", &set))
  {
    printf("invalid argument\n");
    return -1;
  }
  return !!set;
}

void cmd_mask_scan_add(char*cmd)
{
  if (cmd[0]=='\0') return;
  ForceFileScanAdd(cmd);
}

void cmd_mask_scan_rename(char*cmd)
{
  if (cmd[0]=='\0') return;
  ForceFileScanRenameLast(cmd);
}

void cmd_mask_scan_clear(char*cmd)
{
  ForceFileScanClear();
}

int get_key_code(char a, uint32_t * key)
{
  if (97 <= a&&a <= 122) a -= 32;

  // TODO : per core map ?
  switch (a)
  {
    break;default:
      printf("invalid argument\n");
      return -1;

    break;case 'S': *key = 0x58;
    break;case 'U': *key = 0x67;
    break;case 'D': *key = 0x6c;
    break;case 'L': *key = 0x69;
    break;case 'R': *key = 0x6a;
    break;case 'N': *key = 0x1c;
    break;case 'P': *key = 0x01;
    break;case 'H': *key = 0x66;
    break;case 'E': *key = 0x6b;
  }

  return 0;
}

void cmd_useract(char*cmd)
{
  if (*cmd != '\0')
  {
    char a = *cmd;
    cmd++;
    if (*cmd != '\0')
    {
      uint32_t key = 0;
      get_key_code(a, &key);
      int press;
      if (1 == sscanf(cmd, "%d", &press))
      {
        user_io_kbd(key, press);
        return;
      }
    }
  }
  printf("invalid argument\n");
}

static void nsleep(long ns){
  struct timespec w = {0};
  w.tv_sec = 0;
  w.tv_nsec = ns;
  while (nanosleep(&w, &w));
}

// To be run in a sub process
static int send_user_io_sequence(void* data){
  char* scr = (char*) data;
  while(*scr != '\0'){
    if (*scr != ' '){
      FILE* f;
      f = fopen(CMD_FIFO, "w");
      if (f)
      {
        fprintf(f, "useract %c 1\n", *scr);
        fclose(f);
      }
      nsleep(100000000);
      f = fopen(CMD_FIFO, "w");
      if (f)
      {
        fprintf(f, "useract %c 0\n", *scr);
        fclose(f);
      }
      nsleep(100000000);
    }
    scr++;
  }
  return 0;
}

static int forkdo(int (*callback)(void*), void* data){
  pid_t pid = fork();
  if (pid != 0) return 0; // main process
  exit(callback(data));    // child process
}

void cmd_emulact(char*cmd)
{
  forkdo(send_user_io_sequence, cmd);
}

void cmd_select_a_rom(char*cmd)
{
  cmd_emulact("SNEN");
  // TODO : per-core sequence
}

struct cmdentry
{
  const char * name;
  void (*cmd)(char*);
};
int cmdlist_compare(const void * a, const void * b)
{
  return strcmp(((struct cmdentry*)a)->name, ((struct cmdentry*)b)->name);
}
void handle_MiSTer_cmd(char*cmd)
{
  static struct cmdentry cmdlist[] =
  {
  // The "name field" can not contain ' ' or '\0'.
  // The array must be lexicographically sorted wrt "name" field (e.g.
  //   :sort vim command, but mind '!' and escaped chars at end of similar names).
    {"emulact",      cmd_emulact},
    {"fb_cmd",       video_cmd},
    {"load_core",    cmd_load_rbf},
    {"scan_clear",   cmd_mask_scan_clear},
    {"scan_mask_add",cmd_mask_scan_add},
    {"scan_rename",  cmd_mask_scan_rename},
    {"select_a_rom", cmd_select_a_rom},
    {"useract",      cmd_useract},
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
	struct cmdentry * command = (struct cmdentry*) bsearch(&target , cmdlist,
    sizeof(cmdlist)/sizeof(cmdlist[0]), sizeof(cmdlist[0]), cmdlist_compare);
  if (!command)
  {
    printf("invalid MiSTer command: %s\n", cmd);
    return;
  }
  printf("MiSTer command: %s\n", cmd);
  while (cmd[namelen] == ' ')
    namelen += 1;
  command->cmd(cmd+namelen);
}

