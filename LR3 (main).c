#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>  
#include <stdlib.h> 
#include <unistd.h>
int main ()
{
    pid_t pid;
    pid = fork();
    if (pid > 0){
       	pid = fork();
    }
    if (pid == 0){
       	printf ("Это дочерний процесс его pid=%d\n", getpid());
 		printf ("А pid его родительского процесса=%d\n", getppid());
        system("date +\"%T.%3N\"");

    }
    else if (pid > 0)
    {
        printf ("Это родительский процесс pid=%d\n", getpid());
        system("date +\"%T.%3N\"");
        system("ps -x");
    }
    else
        printf ("Ошибка вызова fork, потомок не создан\n");
    wait(NULL);
    wait(NULL);
}
