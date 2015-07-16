/*************************************************************************************************************************************************\
                                       Copyright (c) 2015 Weilin.Chen
Author:       Weilin.Chen (QQ:532776869, GitHub: https://github.com/4Second2None)
Date:         2015-05-25 00:24:00
Note:         If you think the code is best for your projects, and you want to use it. Just tell me, otherwise you may make mistake dude.
              It's not about the salary, just for fun!!!!
\*************************************************************************************************************************************************/
#include "s2n_cws.h"
static char rn[] = "\r\n";

struct cws_ctx*  fnew_cws(const char * url)
{
    struct cws_ctx * ws;
    ws = (struct cws_ctx *)malloc(sizeof(struct cws_ctx));
    memset(ws, 0, sizeof(struct cws_ctx));
    ws->host = (char *)malloc(64);
    ws->resource = (char *)malloc(1024);
    memset(ws->host, 0, 64);
    memset(ws->resource, 0, 1024);

    char tmpbuf[1024] = {0};
    int iret = sscanf(url, "%*7[^://]%*c%*c%*c%63[^:]%*c%d/%1023s",ws->host, &(ws->port), tmpbuf);
    sprintf(ws->resource, "/%s", tmpbuf);

    if(iret == 1)
    {
        int iret = sscanf(url, "%*[^://]%*c%*c%*c%63s/%1023s",ws->host, tmpbuf);
        ws->port = 80;
    }
    sprintf(ws->resource, "/%s", tmpbuf);
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
void ffree_cws(struct cws_ctx *ws)
{
    if(ws)
    {
        _free(ws->acceptKey);
        _free(ws->webKey);
        _free(ws->server);
        _free(ws->date);
        _free(ws->recv_data);
        _free(ws->host);
        _free(ws->resource);
        st_netfd_close(ws->fd);
        _free(ws);
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

static int fsend_frame(struct cws_ctx *ws, uint8_t *outBuff, size_t framesize)
{
    int iret = st_write(ws->fd, outBuff, framesize, ST_UTIME_NO_TIMEOUT);
    if (iret != framesize)
    {
        ws->erno = 300;
        return 0;
    }
    return 1;
}

static int fraw_connect(struct cws_ctx * ws)
{
    st_netfd_t fd;
    struct sockaddr_in remote_addr;
    memset(&remote_addr, 0, sizeof(remote_addr));
    remote_addr.sin_family = AF_INET;
    remote_addr.sin_addr.s_addr = inet_addr(ws->host);
    remote_addr.sin_port = htons(ws->port);
    int sock;
    if((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("socket() error\n");
        return -1;
    }
    if((fd = st_netfd_open_socket(sock)) == NULL)
    {
       
        printf("st_netfd_open_socket error\n");
        close(sock);
        return -1;
    }
        ws->fd = fd;
    if(st_connect(fd, (struct sockaddr *)&remote_addr, sizeof(remote_addr), ST_UTIME_NO_TIMEOUT) < 0 )
    {
        printf("connect error\n");
        st_netfd_close(fd);
        return -1;
    }
    return 0;
}

static int fws_start_handshake(struct cws_ctx * ws)
{
    char webkey[256] = {0};
    char key_index[16] = {0};
    int z;
    for(z = 0; z < 16; z++)
    {
        key_index[z] = b64[(rand() + time(NULL)) % 61];
    }
    int ri = base64(webkey, 256, key_index, 16);
    int offset = 0;
    uint8_t header_str[512] = {0};
    ws->webKey = (char *)malloc(25);
    memset(ws->webKey, 0, 25);
    memcpy(ws->webKey, webkey, ri);
    offset += sprintf(header_str + offset, "GET %s HTTP/1.1\r\n", ws->resource);
    offset += sprintf(header_str + offset, "Upgrade: websocket\r\n");
    offset += sprintf(header_str + offset, "Connection: Upgrade\r\n");
    offset += sprintf(header_str + offset, "Host: %s:%u\r\n", ws->host, ws->port);
    offset += sprintf(header_str + offset, "Origin: http://%s:%u\r\n", ws->host, ws->port);
    offset += sprintf(header_str + offset, "Sec-WebSocket-Key: %s\r\n", ws->webKey);
    offset += sprintf(header_str + offset, "Sec-WebSocket-Version: 13\r\n\r\n");

    return fsend_frame(ws, header_str, strlen(header_str));
}

static uint8_t * fws_recvheader(struct cws_ctx * ws)
{
    int ireaded = 0;
    int iret = 0;
    uint8_t * headerBuf = (uint8_t*)malloc(BUF_LEN * sizeof(uint8_t));
    while(1)
    {
        int iret = st_read(ws->fd, headerBuf+ireaded, BUF_LEN-ireaded, ST_UTIME_NO_TIMEOUT);
        if (iret <= 0)
        {
            ws->erno = 100;
            free(headerBuf);
            return NULL;
        }
        ireaded += iret;
        if (ireaded == BUF_LEN)
        {
            ws->erno = 200;
            free(headerBuf);
            return NULL;
        }
        if(strcmp(headerBuf + ireaded - 4, "\r\n\r\n") == 0)
        {
//            char mm[128] = {0};
//            memset(mm, 0, 128);
//            int iret0 = st_read(ws->fd, headerBuf+ireaded, BUF_LEN-ireaded, ST_UTIME_NO_TIMEOUT);
//            printf("*******************************%d-----------------%s^^^^^^^^^\n", iret0, mm);
            return headerBuf;
        }
    }
}

static int fvalidate_header(struct cws_ctx * ws, uint8_t * headerData)
{
    int status;
    uint8_t *header = headerData;
    if(sscanf(header, "HTTP/1.1 %d Switching Protocols\r\n", &status) != 1)
    {
        ws->erno = 400;
        return -1;
    }
    if (status != 101)
    {
        ws->erno = 500;
        return -1;
    }
    header = strstr(header, "\r\n") + 2;

    int connflag = 0;
    int upgradeflag = 0;
    uint8_t *end = header + strlen(header);

    while(header < end && header[0] != '\r' && header[1] != '\n')
    {
        if(memcmp(header, server_field, strlen(server_field)) == 0)
        {
            header += strlen(server_field);
            ws->server = fgetup2linefeed(header);
        }
        /*if(memcmp(header, date_field, strlen(date_field)) == 0)
        {
            header += strlen(date_field);
            ws->date = fgetup2linefeed(header);
        }*/
        if(memcmp(header, connection_field, strlen(connection_field)) ==  0)
        {
            header += strlen(connection_field);
            char * tmp = fgetup2linefeed(header);
            if(tmp && ((strcmp(tmp, "upgrade") == 0) || (strcmp(tmp, "Upgrade") == 0)))
            {
                connflag = 1;
            }
            if(tmp)
            {
                free(tmp);
            }
        }
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
        if(memcmp(header, accept_key_field, strlen(accept_key_field)) == 0)
        {
            header += strlen(accept_key_field);
            ws->acceptKey = fgetup2linefeed(header);
        }
        header = strstr(header, "\r\n") + 2;
    }
    //if(!ws->server || !ws->date || !ws->acceptKey || connflag == 0 || upgradeflag == 0)
    if(!ws->server || !ws->acceptKey || connflag == 0 || upgradeflag == 0)
    {
        printf("----%d-----%d\n", connflag, upgradeflag);
        free(headerData);
        ws->erno = 800;
        return -1;
    }
    char * validatekey = (char *)malloc(strlen(ws->webKey) + strlen(secret));
    memcpy(validatekey, ws->webKey, strlen(ws->webKey));
    memcpy(validatekey + strlen(ws->webKey), secret, strlen(secret));
    char shaHash[20];
    memset(shaHash, 0, 20);
    sha1(shaHash, validatekey, strlen(ws->webKey) + strlen(secret));
    size_t llen = base64(validatekey, strlen(ws->webKey) + strlen(secret), shaHash, 20);
    validatekey[llen] = '\0';
    if(strcmp(validatekey, ws->acceptKey) != 0)
    {
        free(headerData);
        free(validatekey);
        ws->erno = 900;
        return -1;
    }
    free(validatekey);
    free(headerData);
    ws->erno = 0;
    return 0;
}

int fwsconnect(struct cws_ctx * ws)
{
    if(fraw_connect(ws) != 0)
      printf("fraw_connect error!\n");
    if(fws_start_handshake(ws))
    {
        uint8_t *rs_header = fws_recvheader(ws);
        if(rs_header == NULL)
        {
            ffree_cws(ws);
          return -1;
        }
        if(fvalidate_header(ws, rs_header) != 0)
        {
            ffree_cws(ws);
          return -1;
        }
        return 0;

    }
    return -1;
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



static int fsend_dataframe(struct cws_ctx * ws, void * payload, int32_t len, int32_t opcode)
{
    int32_t length = 0;
    int32_t iret = 0;
    ANBF frame = {0};
    char * data_out = NULL;

    char* data = (char *)malloc(len);
    memset(data, 0, len);
    memcpy(data, payload, len);
    fcreate_frame(&frame, 1, 0, 0, 0, opcode, 1, payload, len);
    data_out = (char *)fformat_frame(&frame, &length);
    iret =  st_write(ws->fd, data_out, length, ST_UTIME_NO_TIMEOUT);
    free(data_out);
    free(data);
    return iret;
}

static int frestrict_recv(struct cws_ctx * ws, void * buff, int32_t size)
{
    int32_t offset = 0;
    int iret = 0;
    while(offset < size)
    {
        iret = st_read(ws->fd, ((char *)buff) + offset, (int32_t)(size - offset), ST_UTIME_NO_TIMEOUT);
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

static int32_t frecvframe(struct cws_ctx * ws, ANBF * frame)
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

    iret = frestrict_recv(ws, &frame_header, 2);
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
        iret = frestrict_recv(ws, &length_data_16, 2);
        if(iret < 0)
          goto end;
        frame_length = ntohs(length_data_16);
    }
    else if(length_bits == 0x7f)
    {
        iret = frestrict_recv(ws, &length_data_64, 8);
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
        iret = frestrict_recv(ws, &frame_mask, 4);
        if(iret <  0)
          goto end;
    }
    if(frame_length > 0)
    {
        payload = (char * )malloc((int32_t)frame_length);
        iret = frestrict_recv(ws, payload, (int32_t)frame_length);
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

input_frame_t * frecv_dataframe(struct cws_ctx * ws)
{
    input_frame_t * recv_frame = NULL;
    int data_len = -1;
    int iret = -1;
    ANBF _frame = {0};
    ANBF *frame = &_frame;


    while(1)
    {
        memset(frame, 0, sizeof(frame));
        iret = frecvframe(ws, frame);
        if(iret < 0)
        {
          goto end;
        }
        if(frame->opcode == WS_TEXT_FRAME || frame->opcode == WS_BINARY_FRAME || frame->opcode == WS_CONTINURATION_FRAME)
        {
            if(frame->opcode == WS_CONTINURATION_FRAME && NULL == ws->recv_data)
                goto end;
            else if(ws->recv_data)
            {
                ws->recv_data = (char *)realloc(ws->recv_data, ws->data_size + (uint32_t)frame->length);
                memcpy(ws->recv_data + ws->data_size, frame->data, (uint32_t)frame->length);
                ws->data_size += (uint32_t)frame->length;
                _free(frame->data);
            }
            else
            {
                ws->recv_data = frame->data;
                frame->data = NULL;
                ws->data_size = (uint32_t)frame->length;
            }
            if(frame->fin)
            {
                recv_frame = (input_frame_t *)malloc(sizeof(input_frame_t));
                recv_frame->buff = (char*)malloc(sizeof(char) * ws->data_size + 1);
                memset(recv_frame->buff, 0, ws->data_size + 1);
                memcpy(recv_frame->buff, ws->recv_data, ws->data_size);

                (recv_frame->buff)[ws->data_size] = '\0';
                recv_frame->length = ws->data_size;
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
    _free(ws->recv_data);
    _free(frame->data);
    data_len = ws->data_size;
    ws->data_size = 0;
    return recv_frame;
}

int32_t fsend_ping(struct cws_ctx * ws, void * payload, int32_t len)
{
    return fsend_dataframe(ws, payload, len, WS_PING_FRAME);
}

int32_t fsend_pong(struct cws_ctx * ws, void * payload, int32_t len)
{
    return fsend_dataframe(ws, payload, len, WS_PONG_FRAME);
}

int32_t fsend_text(struct cws_ctx * ws, void * payload, int32_t len)
{
    return fsend_dataframe(ws, payload, len, WS_TEXT_FRAME);
}

int32_t fsend_binary(struct cws_ctx * ws, void * payload, int32_t len)
{
    return fsend_dataframe(ws, payload, len, WS_BINARY_FRAME);
}

int32_t fsend_close(struct cws_ctx * ws, uint16_t status, const char * reason)
{
    char * p = NULL;
    int len = 0;
    char payload[64] = {0};
    status = htons(status);
    p = (char *) &status;
    len = snprintf(payload, 64, "\\x%02x\\x%02x%s", p[0], p[1], reason);
    return fsend_dataframe(ws, payload, len, WS_CLOSING_FRAME);
}

