
#ifndef CMDIO_H
#define CMDIO_H

#define CMD_FIFO "/dev/MiSTer_cmd"

void handle_MiSTer_cmd(char*cmd);

int slave_ui_toggle();

#endif
