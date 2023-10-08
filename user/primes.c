#include "kernel/types.h"
#include "user/user.h"

#define RD 0
#define WR 1

const uint INT_LEN = sizeof(int);

/** 
 * @brief read data from left channel, will not write to right channel that is not divisible by first
*/
int left_first_data(int left[2], int *dest){
  if (read(left[RD], dest, sizeof(int))== sizeof(int)){
    printf("prime %d\n", * dest);
    return 0;
  }
  return -1;
}

void transmit_data(int left[2], int right[2], int first){
  int data;
  // read from left
  while (read(left[RD], &data, sizeof(int))== sizeof(int)){
    if (data % first){
      write(right[WR], &data, sizeof(int));
    }
  }
  close(left[RD]);
  close(right[WR]);
}

void primes(int left[2]){
  close(left[WR]);
  int first;
  if (left_first_data(left, &first) == 0){
    int fd[2];
    pipe(fd); // current pipe
    transmit_data(left, fd, first);

    if (fork() == 0){
      primes(fd);
    }else{
      close(fd[RD]);
      wait(0);
    }
  }
  exit(0);
}


int main(int argc, char ** argv) {
  int fd[2];
  pipe(fd);

  for (int i = 2; i < 35; i++)
    write(fd[WR], &i, INT_LEN);

  if (fork() == 0){
    primes(fd);
  }else{
    close (fd[WR]);
    close (fd[RD]);
    wait(0);
  }
  exit(0);  
}