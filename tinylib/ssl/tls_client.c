
#include "tinylib/ssl/tls_client.h"
#include "tinylib/util/log.h"

#if defined(WIN32)
  #include "tinylib/windows/net/socket.h"
  #include <winsock2.h>

  #define errno WSAGetLastError()
#elif defined(__linux__)
  #include "tinylib/linux/net/socket.h"
  #include <unistd.h>
  #include <errno.h>
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <arpa/inet.h>
  #include <sys/epoll.h>

  #define SOCKET int
  #define INVALID_SOCKET (-1)
  #define closesocket close
  
  #define POLLIN EPOLLIN
  #define POLLOUT EPOLLOUT
  #define POLLHUP EPOLLHUP
  #define POLLERR EPOLLERR
#endif

#include <openssl/ssl.h>

#include <stdlib.h>
#include <string.h>
#include <assert.h>

enum tls_state
{
    TLS_STATE_INIT,
    TLS_STATE_HANDSHAKE,
    TLS_STATE_SNDRCV,
};

struct tls_client
{
    loop_t *loop;
    SOCKET fd;    
    channel_t *channel;

    tls_client_on_connect_f connectcb;
    tls_client_on_data_f datacb;
    tls_client_on_close_f closecb;
    void *userdata;

    char server_ip[16];
    unsigned short server_port;

    SSL_CTX *ssl_ctx;
    SSL *ssl;
    enum tls_state tls_state;
    char *ca_password;

    int is_in_callback;
    int is_alive;

    char temp_buffer[4096];
    buffer_t *in_buffer;
    buffer_t *out_buffer;
};

static inline
void delete_tls_client(tls_client_t* tls_client, int is_passive_close)
{
    if (tls_client->tls_state == TLS_STATE_SNDRCV && is_passive_close == 0)
    {
        SSL_shutdown(tls_client->ssl);
    }

    SSL_free(tls_client->ssl);
    SSL_CTX_free(tls_client->ssl_ctx);
    free(tls_client->ca_password);

    buffer_destory(tls_client->in_buffer);
    buffer_destory(tls_client->out_buffer);

    channel_detach(tls_client->channel);
    channel_destroy(tls_client->channel);
    closesocket(tls_client->fd);
    free(tls_client);

    return;
}

tls_client_t* tls_client_new
(
    loop_t *loop, const char *server_ip, unsigned short server_port, 
    tls_client_on_connect_f connectcb, tls_client_on_data_f datacb, tls_client_on_close_f closecb, void *userdata
)
{
    tls_client_t *tls_client;
    
    SSL_CTX *ssl_ctx;
    SSL *ssl;

    if (loop == NULL || server_ip == NULL  || server_port == 0 || connectcb == NULL || datacb == NULL || closecb == NULL)
    {
        return NULL;
    }

    ssl_ctx = SSL_CTX_new(TLSv1_2_client_method());
    if (ssl_ctx == NULL)
    {
        log_error("tls_client_new: failed to alloc TLSv1.2 ssl context, server addr: %s:%u, errno: %d", 
            server_ip, server_port, errno);
        return NULL;
    }

    SSL_CTX_set_mode(ssl_ctx, SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER | SSL_MODE_ENABLE_PARTIAL_WRITE | SSL_MODE_AUTO_RETRY);

    ssl = SSL_new(ssl_ctx);
    if (ssl == NULL)
    {
        SSL_CTX_free(ssl_ctx);
        return NULL;
    }

    /* more ssl options can be placed here */

    tls_client = (tls_client_t*)malloc(sizeof(*tls_client));
    memset(tls_client, 0, sizeof(*tls_client));

    tls_client->loop = loop;
    tls_client->fd = -1;

    tls_client->connectcb = connectcb;
    tls_client->datacb = datacb;
    tls_client->closecb = closecb;
    tls_client->userdata = userdata;

    strncpy(tls_client->server_ip, server_ip, sizeof(tls_client->server_ip));
    tls_client->server_port = server_port;

    tls_client->ssl_ctx = ssl_ctx;
    tls_client->ssl = ssl;
    tls_client->tls_state = TLS_STATE_INIT;
    tls_client->ca_password = NULL;

    tls_client->is_in_callback = 0;
    tls_client->is_alive = 1;

    tls_client->in_buffer = buffer_new(4096);
    tls_client->out_buffer = buffer_new(4096);

    return tls_client;
}

static
int default_pem_passwd_cb(char *buf, int size, int rwflag, void *password)
{
    size_t password_len = strlen(password);
    strncpy(buf, password, (password_len)+1);

    return (int)password_len;
}

int tls_client_use_ca(tls_client_t* tls_client, const char* ca_file, const char *private_key_file, const char *ca_pwd)
{
    int ssl_ret;
    int ssl_error;

    if (tls_client == NULL || ca_file == NULL)
    {
        return -1;
    }
    
    /* 仅当是证书加密过的才需要提供密码 */
    if (ca_pwd != NULL)
    {
        SSL_CTX_set_default_passwd_cb(tls_client->ssl_ctx, default_pem_passwd_cb);
        tls_client->ca_password = strdup(ca_pwd);
        SSL_CTX_set_default_passwd_cb_userdata(tls_client->ssl_ctx, tls_client->ca_password);
    }

    /* client端是否需要配置证书，要看server的协商要求
     * 类似普通https之类的通信，无需client提供证书证明自己的身份
     * 而类似Apple APNS消息推送和网银等场景，server端在协商结果里则要求client提供身份证书
     */
    ssl_ret = SSL_use_certificate_file(tls_client->ssl, ca_file, SSL_FILETYPE_PEM);
    if (ssl_ret != 1)
    {
        ssl_error = SSL_get_error(tls_client->ssl, ssl_ret);
        log_error("tls_client_new: failed to load CA, ssl errno: %d, tls_client: %p", 
            ssl_error, tls_client);
        return -1;
    }

    if (private_key_file && (ssl_ret = SSL_use_PrivateKey_file(tls_client->ssl, private_key_file, SSL_FILETYPE_PEM)) != 1)
    {
        ssl_error = SSL_get_error(tls_client->ssl, ssl_ret);
        log_error("tls_client_new: failed to load CA private key, ssl errno: %d, tls_client: %p", 
            ssl_error, tls_client);
        return -1;
    }

    ssl_ret = SSL_check_private_key(tls_client->ssl);
    if (ssl_ret != 1)
    {
        ssl_error = SSL_get_error(tls_client->ssl, ssl_ret);
        log_error("tls_client_new: check ssl private key failed, ssl errno: %d, tls_client: %p", ssl_error, tls_client);
        return -1;
    }

    return 0;
}

static
void do_tls_client_destroy(void *userdata)
{
    tls_client_t *tls_client = (tls_client_t*)userdata;
    if (tls_client->is_in_callback)
    {
        tls_client->is_alive = 0;
    }
    else
    {
        delete_tls_client(tls_client, 0);
    }

    return;
}

void tls_client_destroy(tls_client_t* tls_client)
{
    if (tls_client == NULL)
    {
        return;
    }
    loop_run_inloop(tls_client->loop, do_tls_client_destroy, tls_client);
    
    return;
}

static
void tls_client_onevent(SOCKET fd, int event, void* userdata)
{
    tls_client_t *tls_client = (tls_client_t*)userdata;
    int ssl_ret;
    int ssl_error;
    int is_passive_close = 0;

    assert(tls_client->fd == fd);

    if (tls_client->tls_state == TLS_STATE_INIT)
    {
        /* tcp链接失败 */
        if (event & (POLLHUP | POLLERR))
        {
            log_error("tls_client_onevent: fatal to connect to %s:%u, tls_client: %p", tls_client->server_ip, tls_client->server_port, tls_client);
            tls_client->is_in_callback = 1;
            tls_client->connectcb(tls_client, 0, tls_client->userdata);
            tls_client->is_in_callback = 0;
        }
        else if (event & POLLOUT)
        {
            tls_client->tls_state = TLS_STATE_HANDSHAKE;
            channel_clearevent(tls_client->channel, POLLOUT);
            channel_setevent(tls_client->channel, POLLIN);

            SSL_set_fd(tls_client->ssl, tls_client->fd);
            ssl_ret = SSL_connect(tls_client->ssl);
            ssl_error = SSL_get_error(tls_client->ssl, ssl_ret);
            if (ssl_ret == 1)
            {
                tls_client->tls_state = TLS_STATE_SNDRCV;
                tls_client->is_in_callback = 1;
                tls_client->connectcb(tls_client, 1, tls_client->userdata);
                tls_client->is_in_callback = 0;
            }
            else if (ssl_ret < 0 && (ssl_error == SSL_ERROR_WANT_READ || ssl_error == SSL_ERROR_WANT_WRITE))
            {
                /* keep going and nothing todo */
            }
            else
            {
                log_error("tls_client_onevent: fatal ssl error: %d, tls state: %d, tls_client: %p", ssl_error, tls_client->tls_state, tls_client);
                tls_client->is_in_callback = 1;
                tls_client->connectcb(tls_client, 0, tls_client->userdata);
                tls_client->is_in_callback = 1;
            }
        }
    }
    else if (tls_client->tls_state == TLS_STATE_HANDSHAKE)
    {
        if (event & (POLLHUP | POLLERR))
        {
            log_error("tls_client_onevent: handeshake state, connection to %s:%u was broken, tls_client: %p", tls_client->server_ip, tls_client->server_port, tls_client);
            tls_client->is_in_callback = 1;
            tls_client->connectcb(tls_client, 0, tls_client->userdata);
            tls_client->is_in_callback = 0;
        }
        else
        {
            ssl_ret = SSL_connect(tls_client->ssl);
            ssl_error = SSL_get_error(tls_client->ssl, ssl_ret);
            if (ssl_ret == 1)
            {
                tls_client->tls_state = TLS_STATE_SNDRCV;
                tls_client->is_in_callback = 1;
                tls_client->connectcb(tls_client, 1, tls_client->userdata);
                tls_client->is_in_callback = 0;
            }
            else if (ssl_ret < 0 && (ssl_error == SSL_ERROR_WANT_READ || ssl_error == SSL_ERROR_WANT_WRITE))
            {
                /* keep going and nothing todo */
            }
            else
            {
                log_error("tls_client_onevent: handeshake state, fatal ssl error: %d, tls state: %d, tls_client: %p", ssl_error, tls_client->tls_state, tls_client);
                tls_client->is_in_callback = 1;
                tls_client->connectcb(tls_client, 0, tls_client->userdata);
                tls_client->is_in_callback = 1;
            }
        }
    }
    else if (tls_client->tls_state == TLS_STATE_SNDRCV)
    {
        if (event & (POLLHUP | POLLERR))
        {
            log_error("tls_client_onevent: snd-recv state, connection to %s:%u was broken, tls_client: %p", tls_client->server_ip, tls_client->server_port, tls_client);
            is_passive_close = 1;
            tls_client->is_in_callback = 1;
            tls_client->closecb(tls_client, tls_client->userdata);
            tls_client->is_in_callback = 0;
        }
        else
        {
            if (event & POLLIN)
            {
                ssl_ret = SSL_read(tls_client->ssl, tls_client->temp_buffer, sizeof(tls_client->temp_buffer));
                if (ssl_ret > 0)
                {
                    buffer_append(tls_client->in_buffer, tls_client->temp_buffer, ssl_ret);
                    
                    tls_client->is_in_callback = 1;
                    tls_client->datacb(tls_client, tls_client->in_buffer, tls_client->userdata);
                    tls_client->is_in_callback = 0;
                }
                else
                {
                    ssl_error = SSL_get_error(tls_client->ssl, ssl_ret);
                    if (ssl_error == SSL_ERROR_ZERO_RETURN)
                    {
                        is_passive_close = 1;
                        tls_client->is_in_callback = 1;
                        tls_client->closecb(tls_client, tls_client->userdata);
                        tls_client->is_in_callback = 0;
                    }
                    else if (ssl_error == SSL_ERROR_WANT_READ || ssl_error == SSL_ERROR_WANT_WRITE)
                    {
                        /* nothing todo */
                    }
                    else
                    {
                        log_error("tls_client_onevent: ssl read error: %d, tls_client: %p, server: %s:%u", 
                            ssl_error, tls_client, tls_client->server_ip, tls_client->server_port);
                        tls_client->is_in_callback = 1;
                        tls_client->closecb(tls_client, tls_client->userdata);
                        tls_client->is_in_callback = 0;
                    }
                }
            }
            if (event & POLLOUT)
            {
                void *data = buffer_peek(tls_client->out_buffer);
                int data_size = (int)buffer_readablebytes(tls_client->out_buffer);
                assert(data_size > 0);

                ssl_ret = SSL_write(tls_client->ssl, data, data_size);
                if (ssl_ret > 0)
                {
                    assert(ssl_ret <= data_size);
                    buffer_retrieve(tls_client->out_buffer, (unsigned)ssl_ret);
                    if (ssl_ret == data_size)
                    {
                        channel_clearevent(tls_client->channel, POLLOUT);
                    }
                }
                else
                {
                    ssl_error = SSL_get_error(tls_client->ssl, ssl_ret);
                    if (ssl_error == SSL_ERROR_ZERO_RETURN)
                    {
                        is_passive_close = 1;
                        tls_client->is_in_callback = 1;
                        tls_client->closecb(tls_client, tls_client->userdata);
                        tls_client->is_in_callback = 0;
                    }
                    else if (ssl_error == SSL_ERROR_WANT_READ || ssl_error == SSL_ERROR_WANT_WRITE)
                    {
                        /* nothing todo */
                    }
                    else
                    {
                        log_error("tls_client_onevent: ssl write error: %d, tls_client: %p, server: %s:%u", 
                            ssl_error, tls_client, tls_client->server_ip, tls_client->server_port);
                        tls_client->is_in_callback = 1;
                        tls_client->closecb(tls_client, tls_client->userdata);
                        tls_client->is_in_callback = 0;
                    }
                }
            }
        }
    }
    else
    {
        log_warn("tls_client_onevent: unknown io event: %d, tls_client: %p", event, tls_client);
    }

    if (tls_client->is_alive == 0)
    {
        delete_tls_client(tls_client, is_passive_close);
    }

    return;
}

static
void do_tls_client_connect(void *userdata)
{
    tls_client_t *tls_client = (tls_client_t*)userdata;

    int result;
    int error;

    SOCKET fd;
    struct sockaddr_in server_addr;
    int buffer_size;
  #ifdef WIN32
    int len;
  #else
    socklen_t len;
  #endif

    fd = create_client_socket();
    if (fd == INVALID_SOCKET)
    {
        log_error("do_tls_client_connect: create_client_socket() failed, server addr: %s:%u, errno: %d", 
            tls_client->server_ip, tls_client->server_port, errno);

        tls_client->is_in_callback = 1;
        tls_client->connectcb(tls_client, 0, tls_client->userdata);
        tls_client->is_in_callback = 0;

        if (tls_client->is_alive == 0)
        {
            delete_tls_client(tls_client, 0);
        }

        return;
    }
    len = sizeof(buffer_size);
    if (getsockopt(fd, SOL_SOCKET, SO_SNDBUF, (char*)&buffer_size, &len) == 0)
    {
        buffer_size += 4096;
        setsockopt(fd, SOL_SOCKET, SO_SNDBUF, (char*)&buffer_size, sizeof(buffer_size));
    }
    len = sizeof(buffer_size);
    if (getsockopt(fd, SOL_SOCKET, SO_RCVBUF, (char*)&buffer_size, &len) == 0)
    {
        buffer_size += 4096;
        setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (char*)&buffer_size, sizeof(buffer_size));
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(tls_client->server_ip);
    server_addr.sin_port = htons(tls_client->server_port);
    result = connect(fd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    error = (result == 0) ? result : errno;
    switch(error)
    {
        case 0:
      #ifdef WIN32
        case WSAEWOULDBLOCK:
        {
            tls_client->fd = fd;
            break;
        }
      #else
        case EINPROGRESS:
        case EINTR:
        case EISCONN:
        {
            tls_client->fd = fd;
            break;
        }
      #endif
        default:
        {
            log_error("do_tls_client_connect: connect() failed, errno: %d, server: %s:%u, tls_client: %p", 
                error, tls_client->server_ip, tls_client->server_port, tls_client);

            tls_client->is_in_callback = 1;
            tls_client->connectcb(tls_client, 0, tls_client->userdata);
            tls_client->is_in_callback = 0;

            if (tls_client->is_alive == 0)
            {
                delete_tls_client(tls_client, 0);
            }

            return;
        }
    }

    tls_client->channel = channel_new(fd, tls_client->loop, tls_client_onevent, tls_client);
    channel_setevent(tls_client->channel, POLLOUT);

    return;
}

int tls_client_connect(tls_client_t *tls_client)
{
    if (tls_client == NULL)
    {
        return -1;
    }
    if (tls_client->tls_state != TLS_STATE_INIT)
    {
        return -1;
    }

    loop_run_inloop(tls_client->loop, do_tls_client_connect, tls_client);

    return 0;
}

struct tls_client_send_msg
{
    tls_client_t* tls_client;
    void *data;
    unsigned size;
};

static
int do_ssl_send_inloop(tls_client_t* tls_client, const void *data, int size)
{
    int ssl_ret;
    int ssl_error;
    int is_passive_close = 0;

    void *pending_data = buffer_peek(tls_client->out_buffer);
    int pending_data_size = (int)buffer_readablebytes(tls_client->out_buffer);

    if (pending_data_size > 0)
    {
        ssl_ret = SSL_write(tls_client->ssl, pending_data, pending_data_size);
        if (ssl_ret > 0)
        {
            assert(ssl_ret <= pending_data_size);
            buffer_retrieve(tls_client->out_buffer, (unsigned)ssl_ret);
            pending_data_size -= ssl_ret;

            if (pending_data_size > 0)
            {
                buffer_append(tls_client->out_buffer, data, (unsigned)size);
                channel_setevent(tls_client->channel, POLLOUT);
                return 0;
            }

            /* 至此表明之前的pending的数据已经全部发送完成，接下来发送新提交的数据 */

            ssl_ret = SSL_write(tls_client->ssl, data, size);
            if (ssl_ret > 0)
            {
                assert(ssl_ret <= size);
                size -= ssl_ret;
                if (size > 0)
                {
                    buffer_append(tls_client->out_buffer, (const char*)data+ssl_ret, (unsigned)size);
                    channel_setevent(tls_client->channel, POLLOUT);   
                }
                else
                {
                    /* 本次的新提交的数据全部发送完毕，一些安好，nothing todo */
                }

                return 0;
            }

            /* 至此表明发送操作未能成功进行, 需要根据 error code 区分处理 */

            ssl_error = SSL_get_error(tls_client->ssl, ssl_ret);
            if (ssl_error == SSL_ERROR_WANT_READ || ssl_error == SSL_ERROR_WANT_WRITE)
            {
                buffer_append(tls_client->out_buffer, data, (unsigned)size);
                channel_setevent(tls_client->channel, POLLOUT);
                return 0;
            }
            else
            {
                log_error("do_ssl_send_inloop: ssl write error: %d, tls_client: %p, server: %s:%u", 
                    ssl_error, tls_client, tls_client->server_ip, tls_client->server_port);
                tls_client->is_in_callback = 1;
                tls_client->closecb(tls_client, tls_client->userdata);
                tls_client->is_in_callback = 0;
            }
        }
        else
        {
            ssl_error = SSL_get_error(tls_client->ssl, ssl_ret);
            if (ssl_error == SSL_ERROR_ZERO_RETURN)
            {
                is_passive_close = 1;
                tls_client->is_in_callback = 1;
                tls_client->closecb(tls_client, tls_client->userdata);
                tls_client->is_in_callback = 0;
            }
            else if (ssl_error == SSL_ERROR_WANT_READ || ssl_error == SSL_ERROR_WANT_WRITE)
            {
                buffer_append(tls_client->out_buffer, data, (unsigned)size);
                channel_setevent(tls_client->channel, POLLOUT);
                return 0;
            }
            else
            {
                log_error("tls_client_send: ssl write error: %d, tls_client: %p, server: %s:%u", 
                    ssl_error, tls_client, tls_client->server_ip, tls_client->server_port);
                tls_client->is_in_callback = 1;
                tls_client->closecb(tls_client, tls_client->userdata);
                tls_client->is_in_callback = 0;
            }
        }
    }
    else
    {
        ssl_ret = SSL_write(tls_client->ssl, data, size);
        if (ssl_ret > 0)
        {
            assert(ssl_ret <= size);
            size -= ssl_ret;
            if (size > 0)
            {
                buffer_append(tls_client->out_buffer, (const char*)data+ssl_ret, (unsigned)size);
                channel_setevent(tls_client->channel, POLLOUT);   
            }
            else
            {
                /* 本次的新提交的数据全部发送完毕，一些安好，nothing todo */
            }
            return 0;
        }

        /* 至此表明发送操作未能成功进行 */

        ssl_error = SSL_get_error(tls_client->ssl, ssl_ret);
        if (ssl_error == SSL_ERROR_WANT_READ || ssl_error == SSL_ERROR_WANT_WRITE)
        {
            buffer_append(tls_client->out_buffer, data, (unsigned)size);
            channel_setevent(tls_client->channel, POLLOUT);
            return 0;
        }
        else
        {
            log_error("do_ssl_send_inloop: ssl write error: %d, tls_client: %p, server: %s:%u", 
                ssl_error, tls_client, tls_client->server_ip, tls_client->server_port);
            tls_client->is_in_callback = 1;
            tls_client->closecb(tls_client, tls_client->userdata);
            tls_client->is_in_callback = 0;
        }
    }
    
    if (tls_client->is_alive == 0)
    {
        delete_tls_client(tls_client, is_passive_close);
    }

    return 0;
}

static
void do_tls_client_send(void *userdata)
{
    struct tls_client_send_msg *send_msg = (struct tls_client_send_msg*)userdata;
    do_ssl_send_inloop(send_msg->tls_client, send_msg->data, (int)send_msg->size);
    free(send_msg);
    return;
}

int tls_client_send(tls_client_t* tls_client, const void *data, unsigned size)
{
    if (tls_client == NULL || data == NULL || size == 0)
    {
        log_error("tls_client_send: bad args, tls_client: %p, packet: %p, size: %u", tls_client, data, size);
        return -1;
    }

    if (tls_client->tls_state != TLS_STATE_SNDRCV)
    {
        log_error("tls_client_send: bad dtls state: %d, tls_client: %p", tls_client->tls_state, tls_client);
        return -1;
    }

    if (loop_inloopthread(tls_client->loop))
    {
        do_ssl_send_inloop(tls_client, data, (int)size);
    }
    else
    {
        struct tls_client_send_msg *send_msg = (struct tls_client_send_msg*)malloc(sizeof(*send_msg) + size);
        send_msg->tls_client = tls_client;
        send_msg->data = &send_msg[1];
        send_msg->size = size;
        memcpy(send_msg->data, data, size);
        loop_run_inloop(tls_client->loop, do_tls_client_send, send_msg);
    }

    return 0;
}
