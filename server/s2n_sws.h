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
#include <st.h>

static const char ser_version [] = "st-websocket-server/0.0.1";

struct sws_ctx{
    st_netfd_t fd;
    char * host;
    int port;
    int erno;
};//every session of websocket for server

struct conn_ctx{
    char * acceptKey;
    char * webKey;
    char * recv_data;
    int32_t recv_size;
    char * send_data;
    int32_t send_size;
    st_netfd_t fd;
    char * host;
    int port;
    char * origin;
    char * resource;
    int erno;
    struct sws_ctx * server;
};//every session of websocket for client

int fraw_listen(struct sws_ctx * ws);
/**
* @param ws $ wait for the client handshake to the server successfully, and return the connection session
*/
struct conn_ctx * fwsaccept(struct sws_ctx * ws);
/**
* @param conn, payload, len $ send ping frame
*/
int32_t fsend_ping(struct conn_ctx * conn, void * payload, int32_t len);

/**
* @param conn, payload, len $ send pong frame
*/
int32_t fsend_pong(struct conn_ctx * conn, void * payload, int32_t len);

/**
* @param conn, payload, len $ send text frame
*/
int32_t fsend_text(struct conn_ctx * conn, void * payload, int32_t len);

/**
* @param conn, payload, len $ send binary frame
*/
int32_t fsend_binary(struct conn_ctx * conn, void * payload, int32_t len);

/**
* @param conn, status, reason $ send close frame
*/
int32_t fsend_close(struct conn_ctx * conn, uint16_t status, const char * reason);

/**
* @param conn  $ receive data (the function should be used after connecting successfully)
*/
input_frame_t * frecv_dataframe(struct conn_ctx * conn);

/**
* @param addr, port $ init a sws_ctx structure based on the addr and the port
*/
struct sws_ctx * fnew_sws(const char * addr, int port);
/**
* @param ws $ free and NULL cws_ctx structure
*/
void ffree_sws(struct sws_ctx *ws);

void ffree_conn(struct conn_ctx * conn);

#ifdef	__cplusplus
}
#endif
#endif /* S2N_WS_H */
