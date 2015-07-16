/*************************************************************************************************************************************************\
                                                     Copyright (c) 2015 Weilin.Chen

Author:       Weilin.Chen (QQ:532776869, GitHub: https://github.com/4Second2None)
Date:         2015-05-24 23:48:00
Note:         If you think the code is best for your projects, and you want to use it. Just tell me, otherwise you may make mistake dude.
              It's not about the salary, just for fun!!!!
\*************************************************************************************************************************************************/
#ifndef S2N_WS_H
#define S2N_WS_H
#ifdef	__cplusplus
extern "C" {
#endif

#include <s2n_base64.h>
#include <s2n_sha1.h>
#include <s2n_util.h>
#include "../state-threads-master/obj/st.h"

struct cws_ctx{
    char * acceptKey;
    char * webKey;
    char * server;
    char * date;
    enum wsframe_type frametype;
    char * recv_data;
    int32_t data_size;
    st_netfd_t fd;
    char * host;
    int port;
    char * resource;
    int erno;
};//every session of websocket for client


/**
* @param ws $ connect to the websocket's server, the information of the server is in ws
*/
int fws_connect(struct cws_ctx * ws);

/**
* @param ws, payload, len $ send ping frame
*/
int32_t fsend_ping(struct cws_ctx * ws, void * payload, int32_t len);

/**
* @param ws, payload, len $ send pong frame
*/
int32_t fsend_pong(struct cws_ctx * ws, void * payload, int32_t len);

/**
* @param ws, payload, len $ send text frame
*/
int32_t fsend_text(struct cws_ctx * ws, void * payload, int32_t len);

/**
* @param ws, payload, len $ send binary frame
*/
int32_t fsend_binary(struct cws_ctx * ws, void * payload, int32_t len);

/**
* @param ws, status, reason $ send close frame
*/
int32_t fsend_close(struct cws_ctx * ws, uint16_t status, const char * reason);

/**
* @param ws  $ receive data (the function should be used after connecting successfully)
*/
input_frame_t * frecv_dataframe(struct cws_ctx * ws);

/**
* @param url $ init a cws_ctx structure based on the url
*/
struct cws_ctx * fnew_cws(const char * url);
/**
* @param ws $ free and NULL cws_ctx structure
*/
void ffree_cws(struct cws_ctx *ws);

#ifdef	__cplusplus
}
#endif
#endif /* S2N_WS_H */
