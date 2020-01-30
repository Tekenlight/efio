#ifndef EF_IO_H_INCLUDED
#define EF_IO_H_INCLUDED

#include <ev_include.h>
#include <sys/cdefs.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

#include <thread_pool.h>

#define FILE_OPER_OPEN 0
#define FILE_OPER_READ 1
#define FILE_OPER_CLOSE 2
#define FILE_OPER_READAHEAD 3
#define FILE_OPER_WRITE 4

__BEGIN_DECLS

/*
 * Two assumptions leading to the existing design of buffered IO in unix are
 * 1. Disk access is several orders of magnitude slower than memory access milliseconds vs nano seconds.
 *    Accessing data from processor’s L1 and L2 cache is faster still.
 * 2. Second, data accessed once will, with a high likelihood, find itself accessed again in the near future.
 *    This principle—that access to a particular piece of data tends to be clustered in time—is called
 *    temporal locality.
 *
 * Server programs may deal with either (a) incoming files from users (FTP, file upload etc.), or
 * (b) writing output files, logs, or reports that may be sent by any means to recipients.
 * In both situations the principle of temporal locality does not hold. In fact it will be a waste of server
 * memory to hold the pages from these files in cache.
 *
 * Thus there is a need to develop a different solution suitable for file access by servers, which is asynchronous IO.
 *
 * With the introduction of POSIX AIO standard and implemenetations, it has become possible to access regular
 * files in an asynchronous manner.
 *
 * The AIO interface however has certain limitations.
 * It accepts a request and does the data transfer to user buffer always in an asynchronous manner
 * Typical access to files is through buffered IO done by the OS kernel. Many calls to read are fulfilled by mere
 * memcpy from kernel buffer to user area. It will be very costly for a thread executing such a read to switch context
 * to some other activity and restore the context to the primary task to continue and experience priority inversion.
 *
 * At the same time, it is still benficial to switch context and resume later if the data needs to be trasnfered from
 * secondary storage to primary memory.
 *
 * This module aims to solve exactly the problem.
 * The data access functions ef_read and ef_write will return or write the data immdeiately and synchronously if it is
 * possible to do so with the available buffer. In case the data is not ready for read or buffer is not available for
 * write, immediately the functions return error by setting errno to EAGAIN.
 * The caller thread can now continue with other activities and wait for the availability of the file for operation
 * at a later time.
 *
 * The module can easily be integrated to an eventing library such as libev, through usage of global and thread safe
 * queues. The eventing mechanism  is expected to implement a function pointer for signaling The event.
 *
 * If the function pointer is provided, the EF module will wake up the eventing mudule
 * The inputs to the function  indicate which all files have become available for read and
 * which all files have become available for write.
 *
 * The EF module identifies each opened file by its own index, equivalent of a file descriptor of the Unix kernel.
 * A caller is expected to open the file and pass the file descriptor to do any further opration.
 *
 * A closed fd can be reused/reallocated by the library for some other file in the future.
 *
 */

typedef void (*ef_notification_f_type)(int fd, int completed_oper, void * cb_data);
/*
 * This is the prototype of the function that the subscriber needs to implement
 * In order to receive notifications of completion of an initiated action.
 * If left unimplemented, the caller can find the status of the file, by calling
 * ef_poll.
 *
 * The inputs are 
 * 1. int fd: for which some operation is completed.
 * 2. int completed_oper: The code for the operation that is completed.
 *    0 - open
 *    1 - read
 *    2 - close
 * 3. void * cb_data: This is contextual data provided at the time of funcptr registration.
 *    This can be used to implement any specific call back logic.
 *
 * Note that write operation is non blocking, thus it does not entail a callback.
 */

void ef_init();
/*
 * This function initializes the File IO subsystem within a process
 * It starts a thread which will be responsible for, handling
 * 1. incoming requests
 * 2. Events of completed IO tasks
 * 
 * The notification function pointer is passed as an input to this function.
 * If the argument is passed as null. No notification will be available.
 * Instead the caller thread will have to poll to find the completion status.
 *
 * The module is expected to be a singleton for the entire process.
 */

void ef_unset_cb_func();
void ef_set_cb_func(ef_notification_f_type cb_func, void * cb_data);
/*
 * The caller can set a function to be called back whenever any of the asynchrronous activites
 * gets completed.
 *
 * This function takes 2 parameters, ef_notification_f_type cb_func the function to be called back 
 * and void * cb_data, the contextual data needed to process  the call back.
 *
 * The notification function pointer is passed as an input to this function.
 * If the argument is passed as null. No notification will be available.
 * Instead the caller thread will have to poll to find the completion status.
 *
 * The module is expected to be a singleton for the entire process. */

int ef_open(const char * path, int oflag, ...);
/*
 * Creates a handle to read or write data to a file. The handle is in the form of a file descriptor
 * This function registers a request from the calling thread with the IO subsystem to open
 * a file in secondary memory, pointed by the input parameter "path".
 *
 * path parameter is either an abolute path or path realtive to the current working directory.
 *
 * oflag can be bitwise OR of
 * O_RDONLY		open for reading only
 * O_RDWR		open for reading and writing
 * O_APPEND		append on each write
 * O_CREAT		create file if it does not exist.
 *
 * The third parameter needs to be passed if oflag is orred with O_CREAT. The third parameter is of
 * type mode_t mode. Which is the same as mode in chmod.
 *
 * Upon calling, ef_open will register a request to open the file and return immediately.
 * In case of any errors it will return -1 and set errno. global variable.
 * In case of successsful execution, this function returns an fd, which is subsequently used as a handle for all
 * further operations on this file.
 */

int ef_open_status(int fd);
/*
 * This function initiates the handle on the opened file (with ef_open).
 * The parameter fd is the return value of the function ef_open.
 * If the open operation requested by ef_open is complete, this function returns the fd
 * If the open oeration is not yet complete it will return -1 and set errno to EAGAIN
 * In case of any error it will return -1 and set errno.
 */

int ef_close(int fd);
/*
 * Deletes a descriptor from the per process file descriptor table.
 * If a process exits without closing all opened fd. The data written may be lost.
 * Upon close, all the data in the write buffer is synced with the physical file
 * on the device.
 *
 * Return values
 * Upon successful closure, return value of 0 is returned.
 * Otherwise -1 is returned with global variable errno set.
 */

int ef_close_immediate(int fd);
/*
 * Deletes a descriptor from the per process file descriptor table.
 * If a process exits without closing all opened fd. The data written may be lost.
 * Upon close, all the data in the write buffer is synced with the physical file
 * on the device.
 *
 * Unlinke its async counterpart ef_close, This function blocks until the data is transfered
 * to the disk and the underlying file is closed.
 *
 * Return values
 * Upon successful closure, return value of 0 is returned.
 * Otherwise -1 is returned with global variable errno set.
 */

ssize_t ef_read(int fd, void * buf, size_t nbyte);
/*
 * This interface of this function is the same as the read system call.
 * The function reads data from a file into a memory buffer in the user space.
 *
 * fd is the file descriptor opened using ef_open and ef_open_status, buf is the pointer
 * to the memory location to which maximum of nbyte number of bytes will be
 * written upon successful compleation of the function.
 *
 * The amount of data transferred to buf, can be less than or equal to nbyte.
 * If the data is not yet ready to be transferred. The function returns error with EAGAIN
 * set in errno. The caller is exepected to wait till the fd is readable again and then
 * invoke the funcion ef_read.
 *
 * The function returns the following values
 * > 0 if it is successful in completing transfer of returned number of bytes.
 * = 0 if the all the data in the file is read and EOF is reached.
 * < 0 if there is any error, the global variable errno is set.
 */

ssize_t ef_file_ready_for_read(int fd);
/*
 * Checks the status of the file for read operation.
 * Following are the return values.
 *
 * 0 : EOF has been reached
 * -1: Status to be determined based on errno
 *  	EAGAIN    : Read action has been triggered and is still in progress
 *  	any other : fd in an erroneous state and read cannot happen.
 * 1 : Data Available for read in the file buffer, and read action will succeed
 */

ssize_t ef_write(int fd, void *buf, size_t nbyte);
/*
 * Write attempts to transfer nbyte data to the internal write buffer of the IO module.
 * Upon completion, it returns the number of bytes transfered. The caller is expected to 
 * check the return value and attempt transferring data in a loop until all of the data 
 * is completely transferred.
 *
 * The IO module internally arranges to sync the transferred bytes to the disk.
 *
 * The return value indicates the status of data transfer as follows.
 *
 * > 0, if successful in transferring returned number of bytes
 * = 0, if the buffer is not ready for data transfer
 * < 0, in case of any error. The value of the global variable errno is set.
 *
 * The ef_write API, improvises the ease of invocation to the caller, by queueing up the data transfer
 * requests and initiating the transfer when the prervious write gets over. With this the caller
 * will ne anlt to invoke ef_write multiple times without waiting for the completion of previous write.
 * This facility will be useful, in case of writing large amount of data where the success of writing is
 * reasonably certain, such as log files etc...
 *
 * NB: The queueing mechanism works concurrently only with another write and not any other
 * actiom.
 */

int ef_poll(int fd);
/*
 * Poll and find out if the fd is ready for io operation.
 * The input to this function is a valid fd opened previously through ef_open and ef_open_status.
 *
 * Return values:
 *
 * = 0, If the fd is not ready
 * = 1, If the fd is ready for write
 * = 2, If the fd is ready for read
 * = 3, If the fd is ready for both read and write
 * < 0, If there is any error.
 */

int ef_sync(int fd);
/*
 * Sync the write buffer to disk.
 * Ensures that the current write buffer is synced to disk. The call returns immediately
 *
 * The completed activity can be detected using ef_poll.
 */

int ef_close_status(int fd);
/* Poll and find out if the close operation initiated for the input fd is complete.
 *
 * Return values:
 *
 * = 0, If the file is in closed status
 * < 0, In case of any error, the value of errno is set.
 *
 * errno = EAGAIN : The operation is still in progress.
 * errno = EINVAL : Close opertaion is not initiated.
 *
 */

void ef_set_thrpool(thread_pool_type thr_pool);
/*
 * The function sets the thread pool to be used for asynchronous activities of file management.
 *
 * This function takes 1 parameter, thread_pool_type , the handle to a thread pool.
 * The thread pool pointed by thread_pool_type, should be initialized and be ready
 * to take events for execution.
 *
 * The module is expected to be a singleton for the entire process. */

__END_DECLS
#endif
