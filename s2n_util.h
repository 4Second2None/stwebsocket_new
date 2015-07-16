#ifndef S2n_UTIL_H
#define S2N_UTIL_H
#ifdef	__cplusplus
extern "C" {
#endif
#include <assert.h>
#include <stdint.h> /* uint8_t */
#include <stdlib.h> /* strtoul */
#include <netinet/in.h> /*htons*/
#include <string.h>
#include <stdio.h> /* sscanf */
#include <ctype.h> /* isdigit */
#define BUF_LEN 0xFFFF
#define _free(p) do{if(p) free(p); p = NULL;}while(0)

static const char connection_field[]  = "Connection: ";
static const char server_field[]  = "Server: ";
static const char date_field[]  = "Date: ";
static const char accept_key_field[]  = "Sec-WebSocket-Accept: ";
static const char upgrade[]  = "upgrade";
static const char upgrade2[]  = "Upgrade";
static const char upgrade_field[]  = "Upgrade: ";
static const char websocket[]  = "websocket";
static const char host_field[]  = "Host: ";
static const char origin_field[]  = "Origin: ";
static const char key_field[]  = "Sec-WebSocket-Key: ";
static const char protocol_field[]  = "Sec-WebSocket-Protocol: ";
static const char version_field[]  = "Sec-WebSocket-Version: ";
static const char version[]  = "13";
static const char secret[]  = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
static const char b64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";


enum wsframe_type {
    WS_EMPTY_FRAME = 0xF0,
    WS_ERROR_FRAME = 0xF1,
    WS_INCOMPLETE_FRAME = 0xF2,
    WS_CONTINURATION_FRAME = 0x00,
    WS_TEXT_FRAME = 0x01,
    WS_BINARY_FRAME = 0x02,
    WS_PING_FRAME = 0x09,
    WS_PONG_FRAME = 0x0A,
    WS_OPENING_FRAME = 0xF3,
    WS_CLOSING_FRAME = 0x08
};
enum wsstate {
    WS_STATE_OPENING,
    WS_STATE_NORMAL,
    WS_STATE_CLOSING
};

typedef struct ANBF_t{
    int fin;
    int rsv1;
    int rsv2;
    int rsv3;
    int mask;
    int opcode;
    char *data;
    uint64_t length;
}ANBF;//lay out in abstract the header of websocket during sending or receive data

typedef enum
{
    LENGTH_7 = 0x7d,
    LENGTH_16 = (1 << 16)
}threshold_t;

typedef enum
{
    STATUS_NORMAL = 1000,
    STATUS_GOING_AWAY = 1001,
    STATUS_PROTOCOL_ERROR = 1002,
    STATUS_UNSUPPORTED_DATA_TYPE = 1003,
    STATUS_STATUS_NOT_AVAILABLE = 1005,
    STATUS_ABNORMAL_CLOSED = 1006,
    STATUS_INVALID_PAYLOAD = 1007,
    STATUS_POLICY_VIOLATION = 1008,
    STATUS_MESSAGE_TOO_BIG = 1009,
    STATUS_INVALID_EXTENSION = 1010,
    STATUS_UNEXPECTED_CONDITION = 1011,
    STATUS_TLS_HANDSHAKE_ERROR = 1015
}close_code_t;

typedef struct
{
    char * buff;
    int length;
    enum wsframe_type opcode;
}input_frame_t;


#ifdef	__cplusplus
}
#endif
#endif /* S2N_UTIL_H */
