#include "kernel/types.h"
#include "user/user.h"
#include "kernel/param.h"

#define MAXSZ 512
enum state{
  S_WAITING,
  S_ARG,
  S_ARG_END,
  S_ARG_LINE_END,
  S_LINE_END,   
  S_END
};

enum char_type {
  C_SPACE,
  C_CHAR,
  C_LINE_END
};

/**
 * @brief get char type
 * 
 * @param c to be checked
 * @return enum char_type
*/
enum char_type get_char_type (char c){
  switch (c){
  case ' ': return C_SPACE;
  case '\n': return C_LINE_END;
  default:
    return C_CHAR;
  }
}
/**
 * @brief transform the state
 *
 * @param cur current state
 * @param cc the character to be checked
 * @return enum state the state after transformation
 */
enum state transform_state(enum state cur, enum char_type cc){
  switch (cur){ 
  case S_WAITING:
    if (cc == C_SPACE) return S_WAITING; 
    else if (cc == C_LINE_END) return S_LINE_END;
    else if (cc == C_CHAR) return S_ARG;
    break;
  case S_ARG:
    if (cc == C_SPACE) return S_ARG_END; 
    if (cc == C_LINE_END) return S_ARG_LINE_END;
    if (cc == C_CHAR) return S_ARG;
    break;
  case S_ARG_END:
  case S_ARG_LINE_END:
  case S_LINE_END:
    if (cc == C_SPACE) return S_WAITING;
    if (cc == C_LINE_END) return S_LINE_END;
    if (cc == C_CHAR) return S_ARG;
    break;
  default:
    break;
  }
  return S_END;
}


/**
 * @brief clear all the elements in the parameter list, whenwhen it is used for changing line, will reassign
 * 
 * @param x_argv parameter pointer list
 * @param beg the start of the empty
*/
void clearArgv(char *x_argv[MAXARG], int beg){
  for (int i = beg; i < MAXARG; i++){
    for (int i = beg; i < MAXARG; ++i)
    x_argv[i] = 0;
  }
}

int main(int argc, char *argv[]){
  if (argc - 1 >= MAXARG){
    fprintf(2, "xargs: too many arguments.\n");
    exit(1);
  }
  char lines[MAXSZ];
  char *p = lines;
  char *x_argv[MAXARG] = {0};

  for (int i = 1; i < argc; ++i){
    x_argv[i - 1] = argv[i];
  }
  int arg_beg = 0, arg_end = 0, arg_cnt = argc - 1;
  enum state st = S_WAITING;

  while (st != S_END){
    // empty then quit
    if (read (0, p, sizeof(char)) != sizeof (char)){
      st = S_END;
    }else{
      st = transform_state (st, get_char_type (*p));
    }

    if (++arg_end >= MAXSZ){
      fprintf (2, "xargs: arguments out of range.\n");
      exit(1);
    }

    switch(st){
      case S_WAITING:
        ++arg_beg;
        break;
      case S_ARG_END:
        x_argv[arg_cnt++] = &lines[arg_end];
        arg_beg = arg_end;
        *p = '\0';
        break;
      case S_ARG_LINE_END:
        x_argv[arg_cnt++] = &lines[arg_beg];
      case S_LINE_END:
        arg_beg = arg_end;
        *p = '\0';
        if (fork() == 0) {
          exec(argv[1], x_argv);
        }
        arg_cnt = argc - 1;
        clearArgv(x_argv, arg_cnt);
        wait(0);
        break;
      default:
        break;
    }
    ++p;
  }
  exit(0);

}


