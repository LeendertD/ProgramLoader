/**
 * \file loader.c
 * \brief File housing loader non elf functions.
 *  Leendert van Duijn
 *  UvA
 *
 *  Functions used in support of loading programs.
 *
 * Basic and composit functions.
 *
 **/

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include <svp/mgsim.h>
#include <stddef.h>
#include <svp/abort.h>
#include <fcntl.h>

#include "ELF.h"
#include "loader.h"
#include "basfunc.h"

int parse_setting(const char* key, const char* val,
  unsigned long *settings,
  struct admin_s *out){

  if (streq(key, "noprogname") &&
      streq(val, "true")){
    *settings |= e_noprogname;
    return 0;
  }
  if (streq(key, "core_start")){
    out->core_start = strtoul(val, NULL, 0);
    return 0;
  }
  if (streq(key, "core_size")){
    out->core_size = strtoul(val, NULL, 0);
    return 0;
  }
  if (streq(key, "filename")){
    out->fname = strdup(val);
    return 0;
  }

  if (streq(key, "verbose") &&
      streq(val, "true")){
    locked_print_string("Warning verbosity set to quite high\n",PRINTERR);
    out->verbose = VERB_TRACE + 1;
    return 0;
  }
  if (streq(key, "verbose")){
    out->verbose = strtol(val, NULL,0);
    return 0;
  }
  if (streq(key, "timeit") &&
      streq(val, "true")){
    *settings |= e_timeit;
    return 0;
  }
  if (streq(key, "exclusive") &&
      streq(val, "true")){
    *settings |= e_exclusive;
    return 0;
  }


  return -1;
}

int read_settings(int fd, struct admin_s *out){
  char b;
  int i;
  char *buff;
  char abuff[1024];
  char bbuff[1024];
  int mode, esc;
  if (fd == -1) return -1;
  if (read(fd,0,0)) return -1;
  abuff[0] = bbuff[0] = 0;
  i = 0;
  mode = 0;
  esc = 0;
  buff = abuff;
  b = 1;
  while (b){
    int val;
    if (! read(fd, &b, 1)) b = 0;
    buff[i] = b;
    buff[i+1] = 0;
    if (esc){
      esc = 0;
      continue;
    }
  
    switch (b){
      case '#':
        while (b != '\n'){
          b = 0;
          if (! read(fd, &b, 1)) break;
        }
        //Continue into newline
      case '\n'://apply
        buff[i] = 0;
        buff = abuff;
        i = -1;

        //set settin
        val = parse_setting(abuff, bbuff, &(out->settings), out);
        if (val) return 0;
        abuff[0] = bbuff[0] = 0;

        break;
      case '=': //set
        buff[i] = 0;
        buff = bbuff;
        i = -1;
        break;
      case '\\': //escaped next
        esc = 0;
        i--;
        break;
    }

    i++;
  }
  return 0;
}

int read_env(int fd, struct admin_s *out){
  int i = 0;
  int esc = 0;
  char b = 1;
  off_t osta = 0;
  off_t oend = 0;

  //Get needed room, might be more than needed, but is enough
  //newlines get turned into null's
  //comments take no room
  //escaped chars take half their file space in memory
  //this might be way too much space

  // osta = lseek(fd, 0, SEEK_CUR);
  //oend = lseek(fd, 0, SEEK_END);
  //lseek(fd, osta, SEEK_SET);
  osta = 0;
  oend = 1024*1024; // Ugly fix, no seek?
  out->envp = malloc(2 + (oend - osta));

  while (b){
    if (! read(fd, &b, 1)) break;

    //Initialize on the go
    out->envp[i] = b;
    out->envp[i+1] = 0;
    out->envp[i+2] = 0;
    if (esc){
      esc = 0;
      continue;
    }
    switch (b){
      case '#':
        out->envp[i] = 0;
        while (b != '\n'){
          if (! read(fd, &b, 1)) break;
          //Continue into newline
        }
        if (i==0){
          //first character a comment, no nullbyte needed
          i = -1;
        } else {
          //no double nullbytes due to comments please...
          if (! out->envp[i-1]) i--;
        }
        break;
      case '\\':
        esc = 1;
        i--;
        continue;
        break;
      case '\n':
        out->envp[i] = 0;

        //Dual nullbyte is a final terminator
        if (! out->envp[i-1]) b = 0;
        
        i++;
        continue;
      break;
    }
    i++;
  }

  if (out->envp[0] == 0){
    //No env
    free(out->envp);
    out->envp = NULL;
  }


  return 0;
}


int read_argv(int fd, struct admin_s *out){
  char buff[2048];
  int i = 0;
  int esc = 0;
  char b = 1;

  out->argv = malloc(sizeof(char*)*2);
  out->argv[0] = out->fname;
  out->argc = 1;

  buff[0] = 0;
  while (b){

    if (! read(fd, &b, 1)) break;
    buff[i] = b;
    buff[i+1] = 0;
    if (esc){
      esc = 0;
      continue;
    }
    switch (buff[i]){
       case '#':
        while (buff[i] != '\n'){
          buff[i] = 0;
          if (i==0){
            //no empty arg due to comment
            i = -1;
          } else {
            if (buff[i-1] == 0){
              //no empty args due to comments please
              i--;
            }
          }
          if (! read(fd, buff+i, 1)) break;
          //Continue into newline
        }
        break;
      case '\\':
        esc = 1;
        i--;
        continue;
        break;
      case '\n':
        buff[i] = 0;
        if (buff[0] == 0){
          b = 0;
          break;
        }
        //append
        out->argv[out->argc] = strdup(buff);
        out->argc++;
        out->argv = realloc(out->argv, (1 + out->argc) * sizeof(char*));
        out->argv[out->argc] = NULL;
        i = 0;
        continue;
      break;
    }
    i++;
  }

  //If there is just one entry, check for NULL (which would mean 1 useless
  //entry)
  if (out->argc == 1){
    if (out->argv[0] == NULL){
      free(out->argv);
      out->argv = NULL;
      out->argc= 0;
    }
  }
  return 0;
}

void elf_fromconf(int fd){
  //Called for reading config
  struct admin_s params;
  ZERO_ADMINP(&params);
  params.core_start = 0;
  params.core_size = 1;
  //Default verbosity, nice and LOUD
  params.verbose = VERB_TRACE+1;
  params.settings = 0;
  read_settings(fd, &params);
  read_argv(fd, &params);
  if (!params.fname) {
#if ENABLE_DEBUG
    //Params have been read, it might have requested silence
    if (params.verbose > VERB_ERR){
      locked_print_string("No filename in config to run\n", PRINTERR);
    }
#endif
    return;
  }
  if (read_env(fd, &params)){
#if ENABLE_DEBUG
    if (params.verbose > VERB_ERR){
      locked_print_string("Problems reading env from config\n", PRINTERR);
    }
#endif
    return;
  }

  int i = 0;


  elf_loadfile_p(&params, params.settings);

  for (i=0;i < params.argc;i++) {
    free(params.argv[i]);
  }
  free(params.argv);
  free(params.envp);
}

void elf_fromconfname(const char *fn){
  int fd = open(fn, O_RDONLY);

#if ENABLE_DEBUG
  //Verbosity information is not available at this stage...
  char buff[2048];
  snprintf(buff, 2047,"Program from config %s\n", fn);
  locked_print_string(buff, PRINTERR);
#endif
  elf_fromconf(fd);
  close(fd);
}

enum handled_by elf_clientbreakpoint(int id, const char *msg){
  (void)id;
  (void)msg;
  //locked_print_string(msg, PRINTERR);
  //locked_print_int(id, PRINTERR);
  //....
  return HANDLED_USER;
}
 
struct loader_api_s loader_api = {
  &elf_loadfile,
  &locked_print_string,
  &locked_print_int,
  &locked_print_pointer,
  &elf_fromconfname,
  &elf_fromconf,
  &elf_loadfile_p,
  &elf_clientbreakpoint
};

