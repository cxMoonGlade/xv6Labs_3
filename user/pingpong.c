#include "kernel/types.h"
#include "user/user.h"

#define RD 0
#define WR 1

/**
 * # the character actually sent and received does not matter
 * 1. parent write and push a character to child 
 * 2. child read and print <pid>: received ping
 * 3. child write and push a character to parent
 * 4. parent read and print <pid>: received ping
*/
int main (int argc, char **argv){
  char buff = 'A';

  int fd_c2p [2]; // child to parent
  int fd_p2c [2]; // parent to child

  pipe(fd_c2p);
  pipe(fd_p2c);

  int pid = fork();
  int exit_status;

  if (pid < 0) {
    fprintf(2, "fork() failed\n");
    close(fd_c2p[RD]); close(fd_c2p[WR]);
    close(fd_p2c[RD]); close(fd_p2c[WR]);
    exit_status = 1;
  }else if (pid == 0){ // child process
    close(fd_p2c[WR]); // close parent write socket
    close(fd_c2p[RD]); // close child read socket

    // child read socket and print
    if (read(fd_p2c[RD], &buff, sizeof(char))!= sizeof(char)){ // one and only one character read
      fprintf(2, "child read() failed!\n");
      exit_status = 1;
    }else{
      fprintf(1, "%d: received ping\n", getpid());
    }

    // after reading, write and push
    if (write(fd_c2p[WR], &buff, sizeof(char))!= sizeof(char)){ // one and only one character written
      fprintf(2, "child write() failed!\n");
      exit_status = 1;
    }

    // close other sockets
    close(fd_p2c[RD]);
    close(fd_c2p[WR]);

  }else { // parent process
    // close p2c read and c2p write
    close(fd_p2c[RD]);
    close(fd_c2p[WR]);

    // write and push the character
    if (write(fd_p2c[WR], &buff, sizeof(char))!= sizeof(char)) {
      fprintf(2, "parent write failed!\n");
      exit_status = 1;
    }

    // parent read the character from child and print
    if (read(fd_c2p[RD], &buff, sizeof(char))!= sizeof(char)) {
      fprintf(2, "parent read failed!\n");
      exit_status = 1;
    }else{
      fprintf(1, "%d: received pong\n", getpid());
    }
  }

  // child process closed them, but parent not yet closed
  close(fd_p2c[WR]);
  close(fd_c2p[RD]);
  exit(exit_status);
}