/*************************************************************************************************************************************************\
                                       Copyright (c) 2015 Weilin.Chen
Author:       Weilin.Chen (QQ:532776869, GitHub: https://github.com/4Second2None)
Date:         2015-05-30 00:16:00
Note:         If you think the code is best for your projects, and you want to use it. Just tell me, otherwise you may make mistake dude.
              It's not about the salary, just for fun!!!!
\*************************************************************************************************************************************************/
#include "s2n_sws.h"
static char rn[] = "\r\n";

struct sws_ctx*  fnew_sws(const char * addr, int port)
{
    struct sws_ctx * ws;
    ws = (struct sws_ctx *)malloc(sizeof(struct sws_ctx));
    memset(ws, 0, sizeof(struct sws_ctx));
    ws->host = (char *)malloc(64);
    memset(ws->host, 0, 64);

    memcpy(ws->host, addr, strlen(addr));
    ws->port = port;

    
    return ws;
}

static void * fANBF_mask(uint32_t mask_key, void * data, int32_t len)
{
    int32_t i = 0;
    uint8_t * _m = (uint8_t *) &mask_key;
    uint8_t * _d = (uint8_t *) data;
    for(; i < len; i++)
    {
        _d[i] ^= _m[i % 4];
    }
    return _d;
}
void ffree_sws(struct sws_ctx *ws)
{
    if(ws)
    {
        _free(ws->host);
        st_netfd_close(ws->fd);
        _free(ws);
    }
}

void ffree_conn(struct conn_ctx *conn)
{
    if(conn)
    {
        st_netfd_close(conn->fd);
        _free(conn->webKey);
        _free(conn->acceptKey);
        _free(conn->recv_data);
        _free(conn->send_data);
        _free(conn->host);
        _free(conn->origin);
        _free(conn->resource);
        _free(conn);

    }
}

static char* fgetup2linefeed(const char *startFrom)
{
    char *writeTo = NULL;
    uint8_t newLength = strstr(startFrom, rn) - startFrom;
    assert(newLength);
    writeTo = (char *)malloc(newLength+1); 
    assert(writeTo);
    memcpy(writeTo, startFrom, newLength);
    writeTo[newLength] = 0;
    return writeTo;
}

static int fsend_frame(struct conn_ctx *conn, uint8_t *outBuff, size_t framesize)
{
    int iret = st_write(conn->fd, outBuff, framesize, ST_UTIME_NO_TIMEOUT);
    if (iret != framesize)
    {
        conn->erno = 300;
        return 0;
    }
    return 1;
}

int fraw_listen(struct sws_ctx * ws)
{
    st_netfd_t fd;
    struct sockaddr_in local_addr;
    memset(&local_addr, 0, sizeof(local_addr));
    local_addr.sin_family = AF_INET;
    local_addr.sin_addr.s_addr = inet_addr(ws->host);
    local_addr.sin_port = htons(ws->port);
    int sock;
    if((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("socket() error\n");
        return -1;
    }
    int n = 1;
    if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *) &n, sizeof(n)) < 0)
    {
        printf("setsockopt() error\n");
        return -1;
    }

    if(bind(sock, (struct sockaddr *)&local_addr, sizeof(struct sockaddr)) < 0)
    {
        printf("bind() error\n");
        return -1;
    }
    listen(sock, 128);
    if((fd = st_netfd_open_socket(sock)) == NULL)
    {
       
        printf("st_netfd_open_socket error\n");
        close(sock);
        return -1;
    }
        ws->fd = fd;
    
    return 0;
}


struct conn_ctx * fraw_accept(struct sws_ctx * ws)
{
    struct sockaddr_in cli_addr;
    int s = sizeof(struct sockaddr);
    st_netfd_t cli_fd = st_accept(ws->fd, (struct sockaddr *) &cli_addr, &s, ST_UTIME_NO_TIMEOUT);
    if(cli_fd == NULL)
    {
        printf("st_accept error\n");
      return NULL;
    }
    int sock = st_netfd_fileno(cli_fd);


    struct conn_ctx * client = (struct conn_ctx *)malloc(sizeof(struct conn_ctx));
    memset(client, 0, sizeof(struct conn_ctx));
    client->fd = cli_fd;
    client->port = ntohs(cli_addr.sin_port);
    client->server = ws;

    return client;
}

static uint8_t * fsws_recvheader(struct conn_ctx * conn)
{
    struct sws_ctx * server = conn->server;
    int ireaded = 0;
    int iret = 0;
    uint8_t * headerBuf = (uint8_t*)malloc(BUF_LEN * sizeof(uint8_t));
    memset(headerBuf, 0, BUF_LEN);
    while(1)
    {
        int iret = st_read(conn->fd, headerBuf+ireaded, BUF_LEN-ireaded, ST_UTIME_NO_TIMEOUT);
        if (iret <= 0)
        {
            conn->erno = 100;
            free(headerBuf);
            return NULL;
        }
        ireaded += iret;
        if (ireaded == BUF_LEN)
        {
            conn->erno = 200;
            free(headerBuf);
            return NULL;
        }
        if(strcmp(headerBuf + ireaded - 4, "\r\n\r\n") == 0)
          return headerBuf;
    }
}

static int fvalidate_header(struct conn_ctx * conn, uint8_t * headerData)
{
    int status;
    char resbuf[1024] = {0};
    uint8_t *header = headerData;
    if(memcmp(headerData, "GET ", 4) != 0)
    {
        printf("Error frame\n");
        return -1;
    }
    if(sscanf(headerData, "GET %s HTTP/1.1\r\n", resbuf) != 1)
    {
        printf("No resource\n");
        return -1;
    }
    conn->resource = (char *)malloc(strlen(resbuf) + 1);
    memset(conn->resource, 0, strlen(resbuf) + 1);
    memcpy(conn->resource, resbuf, strlen(resbuf));
    
    header = strstr(header, "\r\n") + 2;

    int connflag = 0;
    int versionflag = 0;
    int upgradeflag = 0;
    int protocalflag = 0;
    uint8_t *end = header + strlen(header);

    while(header < end && header[0] != '\r' && header[1] != '\n')
    {
        if(memcmp(header, host_field, strlen(host_field)) == 0)
        {
            header += strlen(host_field);
            conn->host = fgetup2linefeed(header);
        }
        if(memcmp(header, origin_field, strlen(origin_field)) == 0)
        {
            header += strlen(origin_field);
            conn->origin = fgetup2linefeed(header);
        }
        if(memcmp(header, connection_field, strlen(connection_field)) ==  0)
        {
            header += strlen(connection_field);
            char * tmp = fgetup2linefeed(header);
            if(tmp && (strcmp(tmp, "Upgrade") == 0))
            {
                connflag = 1;
            }
            if(tmp)
                free(tmp);
        }
        if(memcmp(header, version_field, strlen(version_field)) ==  0)
        {
            header += strlen(version_field);
            char * tmp = fgetup2linefeed(header);
            if(tmp && (strcmp(tmp, "13") == 0))
            {
                versionflag = 1;
            }
            if(tmp)
                free(tmp);
        }
        /*if(memcmp(header, protocol_field, strlen(protocol_field)) ==  0)
        {
            header += strlen(protocol_field);
            char * tmp = fgetup2linefeed(header);
            if(tmp)
            {
                free(tmp);
                protocalflag = 1;
            }
        }*/
        if(memcmp(header, upgrade_field, strlen(upgrade_field)) == 0)
        {
            header += strlen(upgrade_field);
            char * tmp = fgetup2linefeed(header);
            if(tmp && (strcmp(tmp, "websocket") == 0))
            {
                upgradeflag = 1;
            }
            if(tmp)
                free(tmp);
        }
        if(memcmp(header, key_field, strlen(key_field)) == 0)
        {
            header += strlen(key_field);
            conn->webKey = fgetup2linefeed(header);
        }
        header = strstr(header, "\r\n") + 2;
    }
    //if(!conn->host || !conn->origin || !conn->webKey || connflag == 0 || upgradeflag == 0 || protocalflag == 0 || versionflag == 0)
    if(!conn->host || !conn->origin || !conn->webKey || connflag == 0 || upgradeflag == 0 || versionflag == 0)
    {
        free(headerData);
        conn->erno = 800;
        return -1;
    }
    char * validatekey = (char *)malloc(strlen(conn->webKey) + strlen(secret));
    memcpy(validatekey, conn->webKey, strlen(conn->webKey));
    memcpy(validatekey + strlen(conn->webKey), secret, strlen(secret));
    char shaHash[20];
    memset(shaHash, 0, 20);
    sha1(shaHash, validatekey, strlen(conn->webKey) + strlen(secret));
    size_t llen = base64(validatekey, strlen(conn->webKey) + strlen(secret), shaHash, 20);
    validatekey[llen] = '\0';
    conn->acceptKey = validatekey;
    free(headerData);
    conn->erno = 0;
    return 0;
}

static int fwait_handshake(struct conn_ctx * conn)
{
    uint8_t *rs_header = fsws_recvheader(conn);
    if(rs_header == NULL)
    {
      return -1;
    }
    if(fvalidate_header(conn, rs_header) != 0)
    {
        return -1;
    }
    return 0;

}

static int fresponse_handshake(struct conn_ctx * conn)
{
    int offset = 0;
    char header_str[512] = {0};
    offset += sprintf(header_str + offset, "HTTP/1.1 101 Switching Protocols\r\n%s%s\r\n%s%s\r\n%s%s\r\nSec-WebSocket-Accept: %s\r\n\r\n",server_field, ser_version, upgrade_field, websocket, connection_field, upgrade2, conn->acceptKey);
    int iret = fsend_frame(conn, header_str, strlen(header_str));
    return iret;
}


struct conn_ctx * fwsaccept(struct sws_ctx * ws)
{
    struct conn_ctx * client = fraw_accept(ws);
    if(!client)
    {
        printf("fraw_accept error\n");
      return NULL;
    }
    if(fwait_handshake(client) != 0)
    {
        printf("fwait_handshake error\n");
        ffree_conn(client);
        return NULL;
    }
    if(!fresponse_handshake(client))
    {
        printf("fresponse_handshake error\n");
        ffree_conn(client);
        return NULL;
    }
    return client;
}

static int fcreate_frame(ANBF* frame, int fin, int rsv1, int rsv2, int rsv3, int opcode, int has_mask, void *data, int len)
{
    frame->fin = fin;
    frame->rsv1 = rsv1;
    frame->rsv2 = rsv2;
    frame->rsv3 = rsv3;
    frame->mask = has_mask;
    frame->opcode = opcode;
    frame->data = data;
    frame->length = len;

    return 0;
}

static void * fformat_frame(ANBF * frame, int *size)
{
    int offset = 0;
    int bytelen;
    uint32_t mask = 13;
    char * frame_header = NULL;
    uint16_t header = 
         (frame->fin << 15) | 
         (frame->rsv1 << 14) | 
         (frame->rsv2 << 13) | 
         (frame->rsv3 << 12) | 
         (frame->opcode <<8);
    char byteLen = 0;
    if(frame->length < LENGTH_7)
    {
        header |= frame->mask << 7 | (uint8_t)frame->length;
        bytelen = 0;
    }
    else if(frame->length < LENGTH_16)
    {
        header |= frame->mask << 7 | 0x7e;
        bytelen = 2;
    }
    else
    {
        header |= frame->mask << 7 | 0x7f;
        bytelen = 8;
    }
    if(frame->mask == 1 && frame->data != NULL)
    {
        fANBF_mask(mask, frame->data, frame->length);
    }

    frame_header = (char *)malloc(sizeof(header) + bytelen + (uint32_t) frame->length + 4);
    header = htons(header);
    memcpy(frame_header + offset, &header, sizeof(header));
    offset += sizeof(header);
    if(bytelen == 2)
    {
        uint16_t len = htons((uint16_t)frame->length);
        memcpy(frame_header + offset, &len, sizeof(len));
        offset += sizeof(len);
    }
    else if (bytelen == 8)
    {
        uint64_t len = htonl(frame->length);
        memcpy(frame_header + offset, &len, sizeof(len));
        offset += sizeof(len);
    }
    memcpy(frame_header + offset, &mask, sizeof(mask));
    offset += 4;
    memcpy(frame_header + offset, frame->data, (uint32_t)frame->length);
    *size = offset + (uint32_t)frame->length;
    return frame_header;
}



static int fsend_dataframe(struct conn_ctx * conn, void * payload, int32_t len, int32_t opcode)
{
    int32_t length = 0;
    int32_t iret = 0;
    ANBF frame = {0};
    char * data_out = NULL;

    char * data = (char *)malloc(len);
    memset(data, 0, len);
    memcpy(data, payload, len);
    fcreate_frame(&frame, 1, 0, 0, 0, opcode, 1, data, len);
    data_out = (char *)fformat_frame(&frame, &length);
    iret =  st_write(conn->fd, data_out, length, ST_UTIME_NO_TIMEOUT);
    free(data_out);
    free(data);
    return iret;
}

static int frestrict_recv(struct conn_ctx * conn, void * buff, int32_t size)
{
    int32_t offset = 0;
    int iret = 0;
    while(offset < size)
    {
        iret = st_read(conn->fd, ((char *)buff) + offset, (int32_t)(size - offset), ST_UTIME_NO_TIMEOUT);
        if(iret > 0)
          offset += iret;
        else
        {
            offset = -1;
            break;
        }
    }
    return offset;
}

static int32_t frecvframe(struct conn_ctx * conn, ANBF * frame)
{
    uint8_t b1, b2, fin, rsv1, rsv2, rsv3, opcode, has_mask;
    uint64_t frame_length = 0;
    uint16_t length_data_16 = 0;
    uint64_t length_data_64 = 0;
    uint32_t frame_mask = 0;
    uint8_t length_bits = 0;
    uint8_t frame_header[2] = {0};
    int32_t iret = 0;
    char * payload = NULL;

    iret = frestrict_recv(conn, &frame_header, 2);
    if(iret < 0)
      goto end;

    b1 = frame_header[0];
    b2 = frame_header[1];
    length_bits = b2 & 0x7f;
    fin = b1 >> 7 & 1;
    rsv1 = b1 >> 6 & 1;
    rsv2 = b1 >> 5 & 1;
    rsv3 = b1 >> 4 & 1;
    opcode = b1 & 0xf;
    has_mask = b2 >> 7 & 1;

    if(length_bits == 0x7e)
    {
        iret = frestrict_recv(conn, &length_data_16, 2);
        if(iret < 0)
          goto end;
        frame_length = ntohs(length_data_16);
    }
    else if(length_bits == 0x7f)
    {
        iret = frestrict_recv(conn, &length_data_64, 8);
        if(iret < 0)
          goto end;
        frame_length = ntohl(length_data_64);
    }
    else
    {
      frame_length = length_bits;
    }
    if(has_mask)
    {
        iret = frestrict_recv(conn, &frame_mask, 4);
        if(iret <  0)
          goto end;
    }
    if(frame_length > 0)
    {
        payload = (char * )malloc((int32_t)frame_length);
        iret = frestrict_recv(conn, payload, (int32_t)frame_length);
        if(iret < 0)
        {
            free(payload);
            goto end;
        }
    }

    if(has_mask)
      fANBF_mask(frame_mask, payload, (uint32_t)frame_length);

    return fcreate_frame(frame, fin, rsv1, rsv2, rsv3, opcode, has_mask, payload, (int32_t)frame_length);
end:
    return -1;
}

input_frame_t * frecv_dataframe(struct conn_ctx * conn)
{
    char * buff;
    input_frame_t * recv_frame = NULL;
    int data_len = -1;
    int iret = -1;
    ANBF _frame = {0};
    ANBF *frame = &_frame;


    while(1)
    {
        memset(frame, 0, sizeof(frame));
        iret = frecvframe(conn, frame);
        if(iret < 0)
        {
          goto end;
        }
        if(frame->opcode == WS_TEXT_FRAME || frame->opcode == WS_BINARY_FRAME || frame->opcode == WS_CONTINURATION_FRAME)
        {
            if(frame->opcode == WS_CONTINURATION_FRAME && NULL == conn->recv_data)
                goto end;
            else if(conn->recv_data)
            {
                conn->recv_data = (char *)realloc(conn->recv_data, conn->recv_size + (uint32_t)frame->length);
                memcpy(conn->recv_data + conn->recv_size, frame->data, (uint32_t)frame->length);
                conn->recv_size += (uint32_t)frame->length;
                _free(frame->data);
            }
            else
            {
                conn->recv_data = frame->data;
                frame->data = NULL;
                conn->recv_size = (uint32_t)frame->length;
            }
            if(frame->fin)
            {
                buff = (char*)malloc(sizeof(char) * conn->recv_size + 1);
				memset(buff, 0, conn->recv_size + 1);
                memcpy(buff, conn->recv_data, conn->recv_size);
                recv_frame = (input_frame_t *)malloc(sizeof(input_frame_t));
				buff[conn->recv_size + 1] = '\0';
                recv_frame->buff = buff;
                recv_frame->length = conn->recv_size;
                recv_frame->opcode = frame->opcode;
                goto end;
            }
        }
        else if(frame->opcode == WS_CLOSING_FRAME)
        {
            recv_frame = (input_frame_t *)malloc(sizeof(input_frame_t));
            recv_frame->buff = frame->data;
            recv_frame->length = frame->length;
            recv_frame->opcode = WS_CLOSING_FRAME;
            return recv_frame;
        }
        else if(frame->opcode == WS_PING_FRAME)
        {
            recv_frame = (input_frame_t *)malloc(sizeof(input_frame_t));
            recv_frame->buff = frame->data;
            recv_frame->length = frame->length;
            recv_frame->opcode = WS_PING_FRAME;
            return recv_frame;
        }
        else
          goto end;
    }
end:
    _free(conn->recv_data);
    _free(frame->data);
    data_len = conn->recv_size;
    conn->recv_size = 0;
    return recv_frame;
}

int32_t fsend_ping(struct conn_ctx * conn, void * payload, int32_t len)
{
    return fsend_dataframe(conn, payload, len, WS_PING_FRAME);
}

int32_t fsend_pong(struct conn_ctx * conn, void * payload, int32_t len)
{
    return fsend_dataframe(conn, payload, len, WS_PONG_FRAME);
}

int32_t fsend_text(struct conn_ctx * conn, void * payload, int32_t len)
{
    return fsend_dataframe(conn, payload, len, WS_TEXT_FRAME);
}

int32_t fsend_binary(struct conn_ctx * conn, void * payload, int32_t len)
{
    return fsend_dataframe(conn, payload, len, WS_BINARY_FRAME);
}

int32_t fsend_close(struct conn_ctx * conn, uint16_t status, const char * reason)
{
    char * p = NULL;
    int len = 0;
    char payload[64] = {0};
    status = htons(status);
    p = (char *) &status;
    len = snprintf(payload, 64, "\\x%02x\\x%02x%s", p[0], p[1], reason);
    return fsend_dataframe(conn, payload, len, WS_CLOSING_FRAME);
}

