#include <stdlib.h>
#include <unistd.h>
void main(){
    char c;
    while(read(0, &c, 1)!=0){
        write(1, &c ,1);
    }
}
