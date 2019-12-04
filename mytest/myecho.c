#include <stdio.h>

void print_strings(char* prefix, char** strings){
  //char * p = strings[0];
  while(*strings){
    printf("%s\t", prefix);
    printf("%s\n", *strings);
    strings++;
  }
}

int main(int argc, char* argv[], char* envp[]){
  print_strings("argv:", argv);
  print_strings("envp:", envp);
}
