
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include "cmd_io.h"
#include "user_io.h"
#include "fpga_io.h"
#include "video.h"

#define SBSEARCH(T, SA, C)	(bsearch(T, SA, sizeof(SA)/sizeof(SA[0]), sizeof(SA[0]), (C)))

void cmd_video_cmd(const char* cmd)
{
  video_cmd((char *)cmd);
}

void cmd_load_rbf(const char*cmd)
{
  fpga_load_rbf((char*)cmd);
}

int cmdutil_read_enable_param(const char*cmd)
{
  uint32_t set = 0;
  if (1 != sscanf(cmd, "%d", &set))
  {
    printf("invalid argument\n");
    return -1;
  }
  return !!set;
}

void cmd_mask_scan_add(const char*cmd)
{
  if (cmd[0]=='\0') return;
  ForceFileScanAdd((char*)cmd);
}

void cmd_mask_scan_rename(const char*cmd)
{
  if (cmd[0]=='\0') return;
  ForceFileScanRenameLast((char*)cmd);
}

void cmd_mask_scan_clear(const char*cmd)
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

    break;case 'U': *key = 0x67; // up
    break;case 'D': *key = 0x6c; // down
    break;case 'L': *key = 0x69; // left
    break;case 'R': *key = 0x6a; // right
    break;case 'O': *key = 0x1c; // open
    break;case 'E': *key = 0x01; // esc
    break;case 'H': *key = 0x66; // home
    break;case 'F': *key = 0x6b; // end (finish)
    break;case 'M': *key = 0x58; // osd menu
  }

  return 0;
}

void cmd_useract(const char*cmd)
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
      nsleep(50000000);
      f = fopen(CMD_FIFO, "w");
      if (f)
      {
        fprintf(f, "useract %c 0\n", *scr);
        fclose(f);
      }
      nsleep(50000000);
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

void cmd_emulact(const char*cmd)
{
  forkdo(send_user_io_sequence, (void*)cmd);
}

int first_string_compare(const void * a, const void * b)
{
  return strcmp(*(const char**)a, *(const char**)b);
}

struct keyvalue {
  const char* key;
  const char* value;
};
void cmd_select_a_rom(const char*cmd)
{
  if (!cmd || cmd[0] == '\0') cmd = user_io_get_core_name();

  struct keyvalue default_override[] = {
  // The "key" field can not contain '\0'.
  // The array must be lexicographically sorted wrt "name" field (e.g.
  //   :sort vim command, but mind '!' and escaped chars at end of similar names).

    { "ACUARIUS.CAQ",    "EEMDOFO" },
    { "AO486.C",         "EEMDOFO" },
    { "AO486.D",         "EEMDDOFO" },
    { "ARCHIE.1",        "EEMDOFO" },
    { "ATARI800.Cart",   "EEMDDOFO" },
    { "ATARI800.D2",     "EEMDOFO" },
    { "Amstrad.B",       "EEMDOFO" },
    { "C16.Cart",        "EEMDOFO" },
    { "C16.Disk",        "EEMDDOFO" },
    { "C16.PRG",         "EEMDDOFO" },
    { "C16.Play",        "EEMDDDOFO" },
    { "C16.Tape",        "EEMDDOFO" },
    { "C64.Cart",        "EEMDDOFO" },
    { "C64.Play",        "EEMDDDDOFO" },
    { "C64.Tape",        "EEMDDDOFO" },
    { "CoCo 3.1",        "EEMDOFO" },
    { "CoCo 3.2",        "EEMDDOFO" },
    { "CoCo 3.3",        "EEMDDDOFO" },
    { "Coleco.SG",       "EEMDOFO" },
    { "MACPLUS.2",       "EEMDOFO" },
    { "MACPLUS.VHD",     "EEMDDOFO" },
    { "MegaCD.BIOS",     "EEMDOFO" },
    { "NES.FDSBIOS",     "EEMDOFO" },
    { "NK0011M.A",       "EEMDOFO" },
    { "NK0011M.B",       "EEMDDOFO" },
    { "NK0011M.H",       "EEMDDDOFO" },
    { "SAMCOUPE.2",      "EEMDOFO" },
    { "SPMX.DDI",        "EEMDOFO" },
    { "Spectrum.Tape",   "EEMDOFO" },
    { "TGFX16.SGX",      "EEMDOFO" },
    { "TI-00_4A.D",      "EEMDOFO" },
    { "TI-00_4A.G",      "EEMDDOFO" },
    { "VECTOR06.A",      "EEMDOFO" },
    { "VECTOR06.B",      "EEMDDOFO" },
    { "VIC20.CT",        "EEMDDOFO" },
    { "VIC20.Cart",      "EEMDOFO" },
    { "VIC20.Disk",      "EEMDDDOFO" },
    { "VIC20.Play",      "EEMDDDDDOFO" },
    { "VIC20.Tape",      "EEMDDDDOFO" },
    { "ZSpectrum.Tape",  "EEMDOFO" },

    //{ "Altair8800", 0}, // unsupported
    //{ "MultiComp", 0 }, // unsupported
    //{ "X68000", 0 }, // unsupported
  };

  struct keyvalue target = {cmd, 0};
	struct keyvalue* code = (struct keyvalue*)
    SBSEARCH(&target, default_override, first_string_compare);

  if (code) cmd_emulact((char*)(code->value));
  else cmd_emulact((char*)"EEMOFO"); // default
}

struct cmdentry
{
  const char * name;
  void (*cmd)(const char*);
};
void handle_MiSTer_cmd(char*cmd)
{
  static struct cmdentry cmdlist[] =
  {
  // The "name" field can not contain ' ' or '\0'.
  // The array must be lexicographically sorted wrt "name" field (e.g.
  //   :sort vim command, but mind '!' and escaped chars at end of similar names).
    {"emulact",      cmd_emulact},
    {"fb_cmd",       cmd_video_cmd},
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
	struct cmdentry * command = (struct cmdentry*)
    SBSEARCH(&target, cmdlist, first_string_compare);
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

