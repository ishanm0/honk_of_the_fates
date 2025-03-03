#include "Board.h"
#include "BT.h"
#include <stdio.h>

// test file for testing BT module messaging

// #define Echo_main

#ifdef Echo_main
int main(void)
{
    BOARD_Init();
    BT_Init();
    printf("init done\n");

    uint8_t data[DATA_SIZE];

    while (1)
    {
        int len = BT_Recv(data);
        if (len != ERROR)
        {
            printf("data: '");
            for (int i = 0; i < len; i++)
            {
                printf("%c", data[i]);
            }
            printf("'\n");
            printf("data: '");
            for (int i = 0; i < len; i++)
            {
                printf("%x ", data[i]);
            }
            printf("'\n");
            BT_Send(data, len);
        }
    }
}
#endif
