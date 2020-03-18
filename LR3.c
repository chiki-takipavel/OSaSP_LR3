#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <semaphore.h>
#include <dirent.h>
#include <malloc.h>
#include <errno.h>
#include <fcntl.h> 
#include <sys/stat.h>

#define LEN 256

sem_t* semaphore; 

void compareFiles(char* file1, char* file2) 
{
    pid_t pid;
    struct stat statF1, statF2; 

    pid = fork();

    if (pid == 0)
    {
        sem_wait(semaphore);

        stat(file1, &statF1); 
        stat(file2, &statF2); 

        if (statF1.st_size != statF2.st_size) 
        {
            printf("Процесс: %d. Файл 1: %s (%ld b). Файл 2: %s (%ld b). Файлы не равны.\n", getpid(), file1, statF1.st_size, file2, statF2.st_size);
        }
        else
        {
            FILE* F1;
            FILE* F2;
            char c1, c2;

            if((F1 = fopen(file1, "r")) == NULL)
            {
                printf("Ошибка чтения файла %s\n", file1);
                sem_post(semaphore); 
                kill(getpid(), SIGKILL);
                return;
            }
            if((F2 = fopen(file2, "r")) == NULL)
            {
                printf("Ошибка чтения файла %s\n", file2);
                fclose(F1);
                sem_post(semaphore);
                kill(getpid(), SIGKILL);
                return;
            }

            while((!feof(F1)) && (!feof(F2)))
            {
                c1 = fgetc(F1);
                c2 = fgetc(F2);
                if ((c1 != c2) && (!feof(F1)) && (!feof(F2))) 
                { 
                    printf("Процесс: %d. Файл 1: %s (%ld b). Файл 2: %s (%ld b). Файлы не равны.\n", getpid(), file1, statF1.st_size, file2, statF2.st_size);
                    break; 
                }
            }

            if ((feof(F1)) && (feof(F2)))
            {
                printf("Процесс: %d. Файл 1: %s (%ld b). Файл 2: %s (%ld b). ФАЙЛЫ РАВНЫ.\n", getpid(), file1, statF1.st_size, file2, statF2.st_size);    
            }
            
            fclose(F1);
            fclose(F2);
        }
        
        sem_post(semaphore);
        kill(getpid(), SIGKILL);
    }
    else if (pid == -1)
    {
        puts("Ошибка создания процесса");
        return;
    }
}

int main()
{
    struct dirent* structDir; 
    struct stat fileInfo; 
    
    char dir1[LEN] = { '\0' };
    char dir2[LEN] = { '\0' };

    unsigned int maxProc = 0;
    unsigned int files1Counter = 0;
    unsigned int files2Counter = 0;

    DIR* dir;

    char** dir1Files;
    char** dir2Files;

    dir1Files = (char**)malloc(sizeof(char*));
    dir1Files[0] = (char*)malloc(LEN*sizeof(char));
    
    dir2Files = (char**)malloc(sizeof(char*));
    dir2Files[0] = (char*)malloc(LEN*sizeof(char));

    puts("Введите имя первого каталога:");
    scanf("%s", dir1);
    puts("Введите имя второго каталога:");
    scanf("%s", dir2);
    puts("Введите максимальное количество процессов:");
    scanf("%d", &maxProc);

    //Открытие каталога 1
    if ((dir = opendir(dir1)) == NULL) { 
        puts("Открытие dir1. Ошибка");
        return errno;
    } 

    while ((structDir = readdir(dir)) != NULL) { 
        char fileName[LEN] = { '\0' };
        strncat(fileName, dir1, strlen(dir1));
        strncat(fileName, "/", strlen("/"));
        strncat(fileName, structDir->d_name, strlen(structDir->d_name));

        if (stat(fileName, &fileInfo) == -1) { 
            puts("Ошибка при получении информации об файле");
            return -1; 
        } 
    
        if ((fileInfo.st_mode & S_IFMT) == S_IFREG) { 
            files1Counter++;
            dir1Files = (char**)realloc(dir1Files, files1Counter*sizeof(char*));
            dir1Files[files1Counter-1] = (char*)realloc(dir1Files[files1Counter-1], LEN*sizeof(char));
            strncpy(dir1Files[files1Counter-1], fileName, strlen(fileName));  
        }  
    }
 
    closedir(dir);

    //Открытие каталога 2
    if ((dir = opendir(dir2)) == NULL) { 
        puts("Открытие dir2. Ошибка");
        return errno;
    } 
    
    while ((structDir = readdir(dir)) != NULL) { 
        char fileName[LEN] = { '\0' };
        strncat(fileName, dir2, strlen(dir2));
        strncat(fileName, "/", strlen("/"));
        strncat(fileName, structDir->d_name, strlen(structDir->d_name));

        if (stat(fileName, &fileInfo) == -1) { 
            puts("Ошибка при получении информации об файле");
            return -1; 
        } 
    
        if ((fileInfo.st_mode & S_IFMT) == S_IFREG) { 
            files2Counter++;
            dir2Files = (char**)realloc(dir2Files, files2Counter*sizeof(char*));
            dir2Files[files2Counter-1] = (char*)realloc(dir2Files[files2Counter-1], LEN*sizeof(char));
            strncpy(dir2Files[files2Counter-1], fileName, strlen(fileName));  
        }  
    }
 
    closedir(dir);

    semaphore = sem_open("/compare", O_CREAT, S_IRWXU, maxProc); //read, write, execute/search by owner

    for (int i = 0; i < files1Counter; i++) 
    {
        for (int j = 0; j < files2Counter; j++)   
        {
            int semValue; 
            do 
            { 
                sem_getvalue(semaphore, &semValue); 
            } 
            while (!semValue);

            compareFiles(dir1Files[i], dir2Files[j]);
        } 
    }

    int semValue; 
    do 
    { 
        sem_getvalue(semaphore, &semValue); 
    } 
    while (semValue != maxProc);

    sem_unlink("/compare");

    for (int i = 0; i < files1Counter; i++)
    {
        free(dir1Files[i]);
    }
    for (int i = 0; i < files2Counter; i++)
    {
        free(dir2Files[i]);
    }
    free(dir1Files);
    free(dir2Files);

    return 0;
}