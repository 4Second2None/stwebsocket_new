#include <s2n_sws.h>

void set_concurrency(int num)
{
    pid_t pid;
    int i;
    for(i = 0; i < num; i++)
    {
        if((pid = fork()) < 0)
        {
            printf("fork error \n");
            exit(1);
        }
        if(pid == 0)
          return;
    }
}

void * func(void * args)
{
    char buff1[16] = "Hello";
    struct conn_ctx * client = (struct conn_ctx* )args;
    while(1)
    {
        input_frame_t * inframe = frecv_dataframe(client);

        if(inframe)
        {
            if(inframe->opcode == WS_CLOSING_FRAME)
            {
                ffree_conn(client);
                break;
            }
            else
            {
                if(inframe->opcode == WS_TEXT_FRAME)
                printf("DATA-------------------------------%s\n",inframe->buff);
                free(inframe->buff);
                free(inframe);
            }
            fsend_text(client, "Hello", 5);
        }
        else
        {
            ffree_conn(client);
            break;
        }
    }
    st_thread_exit(NULL);
}

int main(int argc, char * argv[])
{
    st_set_eventsys(ST_EVENTSYS_ALT);
    if(st_init() < 0)
      printf("st_init error\n");
    struct sws_ctx * server = fnew_sws(argv[1], atoi(argv[2]));
    if (server == NULL)
      printf("fnew_sws error\n");
    if(fraw_listen(server) != 0)
      printf("listen error\n");

    int num_procs = 0;
    if(argv[3])
      num_procs = atoi(argv[3]);


    set_concurrency(num_procs);


    while(1)
    {
        struct conn_ctx * client = fwsaccept(server);
        if(client)
        {
            if(st_thread_create(func, (void *)client, 0, 0) == NULL)
              printf("create st thread error\n");
        }
    }
    return 0;
}
