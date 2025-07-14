#ifndef FILESYNC_NET_SOCKET_H
#define FILESYNC_NET_SOCKET_H

#include <stdlib.h>

/*
 * Error Handling:
 * All functions return -1 on failure and set errno.
 * Common errno values:
 *   - ECONNRESET: Connection was lost or closed by peer
 *   - EINVAL: Invalid parameter
 *   - ENOMEM: Allocation failure
 * Call net_socket_destroy() to clean up after any failure.
 */



/**
 * @brief Opaque socket wraper for TCP sockets.
 *
 * Must create with net_socket_create() and destory with
 * net_socket_destroy(). If used as a client first set ip/host 
 * with relevent function, then call net_socket_connect().
 * If used as a server fist set ip/host with relevent function.
 * Next, in order, bind, listen, and accept.
 *
 */
struct net_socket;


/**
 * @brief Creates a net_socket struct.
 *
 * Allocates a default net_socket struct and returns
 * a pointer to it. Must use net_socket_destroy()
 * on it to avoid memory leak. Returns NULL if allocation
 * error occurs.
 *
 * Errno: malloc(), socket().
 *
 * @return New net_socket struct. NULL on error.
 */
struct net_socket* net_socket_create();


/**
 * @brief Safely cleans up net_socket struct.
 *
 * Destroys one net_socket struct. NULL's the pointer
 * that is passed in. This will call close on the socket.
 *
 * Errno: EINVAL if any part of nsock is NULL.
 *
 * @param nsock Pointer to a net_socket pointer to be destroyed.
 * @return 0 on success -1 on falure.
 */
int net_socket_destroy(struct net_socket **nsock);


/**
 * @brief Calls shutdown on the socket.
 *
 * Calls shutdown on the fd inside nsock. Uses SHUT_WR
 * as the how param in shutdown().
 *
 * Errno: shutdown().
 *
 * @param nsock A net_socket struct to shutdown.
 * @return 0 on success -1 on falure.
 */
int net_socket_shutdown(const struct net_socket *nsock);


/**
 * @brief Uses a hostname to set the address info for a net_socket.
 *
 * Takes a hostname string and tries to resolve it with getaddrinfo.
 * Takes the resulting sockaddr and copies it to nsock.
 *
 * Errno: malloc(), mapped errors getaddrinfo(), EINVAL from out of range port.
 *
 * @param nsock The net_socket struct to get a sockaddr.
 * @param port The port for the sockaddr, must be between 1024 and 49125.
 * @param hostname The name of the host to resolve.
 * @return 0 on success -1 on falure.
 */
int net_socket_set_host(struct net_socket* nsock, int port, const char *hostname);


/**
 * @brief Sets sockaddr info for net_socket from ip.
 *
 * Directly create and assigns a sockaddr to nsock with the ip and port.
 *
 * Errno: malloc(), EINVAL from out of range port
 *
 * @param nsock The net_socket that gets the sockaddr.
 * @param port The port to be used for the sockaddr, must be betwen 1024 and 49125.
 * @param ip The ipv4 string to be used for the sockaddr.
 * @return 0 on sucess -1 on falure.
 */
int net_socket_set_ip(struct net_socket *nsock, int port, const char *ip);


/**
 * @brief Binds the netsocket.
 *
 * Calls bind() on the net_socket's fd.
 * This will only work if the net_socket had previously
 * had net_socket_set_host() or net_socket_set_ip() called
 * on it.
 *
 * Errno: bind()
 *
 * @param nsock The net_socket to bind.
 * @return 0 on success and -1 on falure.
 */
int net_socket_bind(const struct net_socket *nsock);


/**
 * @brief Starts listening on the net_socket.
 *
 * Calls listen() on the net_sockets fd.
 * This will only work if the net_socket had previously
 * had net_socket_set_host() or net_socket_set_ip() called
 * on it. Then further net_socket_bind().
 *
 * Errno: listen()
 *
 * @param nsock The net_socket to listen on.
 * @return 0 on success and -1 on falure.
 */
int net_socket_listen(const struct net_socket *nsock);


/**
 * @brief Accepts connection from a net_socket.
 *
 * Calls accpet() on the net_socket fd.
 * This will only work if the net_socket had previously
 * had net_socket_set_host() or net_socket_set_ip() called
 * on it. Then further had net_socket_bind() then net_socket_listen()
 * called on it.
 *
 * The client pointer will be assigned a new net_socket. It is the callers
 * job to call net_socket_shutdown() and net_socket_destroy() to properly
 * close the connection and avoid a memory leak.
 *
 * Errno: accept(), malloc()
 *
 * @param serv A net_socket that can accept connections.
 * @param client net_socket pointer pointer that will be assigned a new net_socket.
 * @return 0 on success and -1 on falure.
 */
int net_socket_accept(const struct net_socket *serv, struct net_socket **client);


/**
 * @brief Connects a net_socket to a listening address.
 *
 * Calls connect() on the net_socket fd.
 * This will only work if the net_socket had previously
 * had net_socket_set_host() or net_socket_set_ip() called
 * on it.
 *
 * Errno: connect()
 *
 * @param nsock The net_socket to connect with.
 * @return 0 on success and -1 on falure.
 */
int net_socket_connect(const struct net_socket *nsock);


/**
 * @brief Receives raw bytes from a net_socket.
 *
 * Reads in size bytes into the buf from nsock. The net_socket
 * must either have had a successful net_socket_connect() or be
 * the client from net_socket_accept().
 *
 * The size param should not be larger than the allocated memory for
 * buf or else segfaults can happen.
 *
 * Returns 0 when the net_socket has been shutdown from the other side.
 *
 *
 * Errno: EINVAL if size is 0 or greater than SSIZE_MAX
 *        recv()
 *
 * @param nsock The net_socket to read from.
 * @param buf The byte buffer to read into.
 * @param size The number of bytes to read from nsock.
 * @return Number of bytes read or -1 on falure. 0 on close.
 */
ssize_t net_socket_recv(const struct net_socket *nsock, void *buf, size_t size);


/**
 * @brief Sends raw bytes to a net_socket.
 *
 * Writes size bytes from buf into the net_socket. The net_socket must either
 * have net_socket_connect() called on it or be the client from net_socket_accept().
 *
 * The flags are just all the flags that send() can take and is passed directly in.
 *
 * Returns The number of bytes that were sent. If this number is less than size then an error
 * occurred and errno should be checked.
 *
 * The param size should not be larger than the allocated memory for buf or segfaluts can happen.
 *
 * Errno: EINVAL if size is 0 or greater than SSIZE_MAX.
 *        send()
 *
 * @param nsock The net_socket that to send bytes to.
 * @param buf A byte array of data to write.
 * @param size Number of bytes to write from buf.
 * @param flags Flags from send().
 * @return 0 on success and -1 on falure.
 */
ssize_t net_socket_send(const struct net_socket *nsock, const void *buf, size_t size, int flags);


/**
 * @brief Receives a file from a net_socket.
 *
 * Reads in size bytes from nsock and writes them to file_fd. It is important
 * that size be either equal to or less thatn the size of the file file_fd.
 * Will record the total bytes received from nsock in total_recv. Will record
 * the total bytes written into file_fd in total_written.
 *
 * If the return value is -1, -2, or -3 when an error has occured.
 *
 * If the return was -1 then the error is from recv(). The errno will either
 * be ECONNRESET meaning the peer closed the connection or an errno from recv().
 * If total_recv is less than size, then size - total_recv bytes are still buffered
 * in nsock from the file being sent.
 *
 * If the return was -2 then the error is from write(). The errno will be from
 * write(). If total_recv is less than size, then size - total_recv bytes are still
 * buffered in nsock fromm the file being sent. If total_written is less than
 * total_recv then total_recv - total_written bytes have been lost.
 *
 * If the return was -3 then the net_socket connection was closed by the peer.
 *
 *
 * The net_socket must either have had a successful net_socket_connect() or be
 * the client from net_socket_accept().
 *
 * Errno: recv()
 *        write()
 *
 * @param nsock The net_socket to send to.
 * @param file_fd The file to 
 * @param size Size of the file to be received.
 * @param total_recv Number of bytes successfully received.
 * @param total_written Number of bytes successfully written to file_fd.
 * @return 0 on success
 *         -1 on recv falure
 *         -2 on write falure
 *         -3 on connection close
 */
int net_socket_file_recv(const struct net_socket *nsock, int file_fd, size_t size, size_t *total_recv, size_t *total_written);


/**
 * @brief Sends a file through a net_socket.
 *
 * Reads in size bytes form file_fd and writes them to nsock. Will record the number
 * of bytes received and writen to file_fd in toatl_recv. If total_recv is less than
 * size it means a partial received occured and there are still size - toatl_
 *
 * Reads in size bytes from nsock and writes them to file_fd. It is important
 * that size be either equal to or less thatn the size of the file file_fd.
 * Will record the total bytes received from nsock in total_recv. In the event
 * of an error total_recv might be larger than zero, in this case assume that
 * size - total_recv bytes were never sent to nsock.
 *
 * The net_socket must either have had a successful net_socket_connect() or be
 * the client from net_socket_accept().
 *
 * Errno: ECONNRESET if the connection has been closed
 *        read()
 *        send()
 *
 * @param nsock The net_socket to send to.
 * @param file_fd File to read from.
 * @param size Size of the file to send.
 * @param total_sent Number of bytes successfully sent.
 * @return 0 on success
 *         -1 on read falure
 *         -2 on send falure
 */
int net_socket_file_send(const struct net_socket *nsock, int file_fd, size_t size, size_t *total_sent);
#endif
