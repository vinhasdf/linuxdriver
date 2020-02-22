#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <string.h>

int main()
{
    char *buff;
    char *buffmap;
    char option;
    int fd, pagesize;
    char finish = 1;

    fd = open("/dev/memdev", O_RDWR);
    if(fd < 0)
    {
        printf("Cannot open mem device\n");
        return -1;
    }

    pagesize = getpagesize();
    buff = malloc(pagesize);

    buffmap = mmap(NULL, pagesize, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0); // default offset = 0

    if(buffmap == NULL || buff == NULL)
    {
        printf("Mapping memory failed\n");
        return -1;
    }

    printf("Test app to testing the memory map driver\n");
    printf("Enter the following option character:\n");
    printf("'w' : To write to the device with memory mapping\n");
    printf("'r' : To read from device with memory mapping\n");
    printf("'c' : To clear device with memory mapping\n");          // update ioctl later
    printf("'W' : To write to the device with normal operation\n");
    printf("'R' : To read from device with normal operation\n");
    printf("'C' : To clear device with normal operation\n");

    printf("\nPress q to quit\n");

    while(finish)
    {
        printf("Enter your option: ");
        fflush(stdin);
        scanf(" %c", &option);
        fflush(stdin);
        switch (option)
        {
        case 'w':
            memset(buffmap, 0, pagesize);
            printf("Enter the message to wirte: ");
            scanf(" %[^\n]s", buffmap);
            break;
        case 'r':
            printf("Your massage: %s\n", buffmap);
            break;
        
        case 'c':
            memset(buffmap, 0, pagesize);
            break;

        case 'W':
            memset(buff, 0, pagesize);
            printf("Enter the message to wirte: ");
            scanf(" %[^\n]s", buff);
            write(fd, buff, pagesize);
            break;
        case 'R':
            memset(buff, 0, pagesize);
            read(fd, buff, pagesize);
            printf("Your massage: %s\n", buff);
            break;
        
        case 'C':
            memset(buff, 0, pagesize);
            write(fd, buff, pagesize);
            break;
        
        case 'q':
            finish = 0;
            break;

        default:
            printf("Wrong option\n");
            break;
        }
    }

    free(buff);
    munmap(buffmap, pagesize);
    close(fd);
    return 0;
}