#include <s2n_cws.h>
#include <fcntl.h>

char* filebuf;
off_t len;
int file;
void * func(void * args)
{

    struct cws_ctx *ws = (struct cws_ctx *)args;

    int i;
    for(i = 0; i < 100; i++)
    {

        char buff1[1024] = "Hello server！";
        fsend_text(ws, buff1, strlen(buff1));

    }
}

void * func1(void * args)
{
    struct cws_ctx *ws = (struct cws_ctx *)args;
    while(1)
    {
        input_frame_t * inframe = frecv_dataframe(ws);
        if(inframe)
        {
            if(inframe->opcode == WS_CLOSING_FRAME)
            {
                ffree_cws(ws);
                break;
            }
            else
            {
                printf("RESULT------------------%s------------------\n",inframe->buff);
                free(inframe->buff);
                free(inframe);
            }
        }
        else
        {
            printf("recv error\n");
            ffree_cws(ws);
            break;
        }
    }
}

int main(int argc, char * argv[])
{

    len -= 44;
    st_set_eventsys(ST_EVENTSYS_ALT);
    if(st_init() < 0)
      printf("init error\n");
    int i;
    for(i = 0; i < 10; i++)
    {

        struct cws_ctx *ws = fnew_cws(argv[1]);
        int iret = fwsconnect(ws);
        if(iret == -1)
        {
            printf("connect error\n");
            return -1;
        }

        if(st_thread_create(func, (void *)ws, 0, 0) == NULL)
          printf("create thread error\n");
        if(st_thread_create(func1, (void *)ws, 0, 0) == NULL)
          printf("create thread error\n");
    }
    st_thread_exit(NULL);
    return 0;
}
