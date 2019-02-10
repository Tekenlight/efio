# efio
## Event driven file IO module ##
Two assumptions leading to the existing design of buffered IO in unix are
1. **Disk access is several orders of magnitude slower than memory access milliseconds vs nano seconds.**
   **Accessing data from processor’s L1 and L2 cache is faster still.**
2. **Second, data accessed once will, with a high likelihood, find itself accessed again in the near future.**
   **This principle—that access to a particular piece of data tends to be clustered in time—is called**
   **temporal locality.**

Server programs may deal with either 
a. incoming files from users (FTP, file upload etc.), or
b. writing output files, logs, or reports that may be sent by any means to recipients. In both situations the principle of temporal locality does not hold. In fact it will be a waste of server memory to hold the pages from these files in cache.

Thus there is a need to develop a different solution suitable for file access by servers, which is asynchronous IO.

With the introduction of POSIX AIO standard and implemenetations, it has become possible to access regular
files in an asynchronous manner.

The AIO interface however has certain limitations.
It accepts a request and does the data transfer to user buffer always in an asynchronous manner
Typical access to files is through buffered IO done by the OS kernel. Many calls to read are fulfilled by mere
memcpy from kernel buffer to user area. It will be very costly for a thread executing such a read to switch context
to some other activity and restore the context to the primary task to continue and experience priority inversion.

At the same time, it is still benficial to switch context and resume later if the data needs to be trasnfered from
secondary storage to primary memory.

This module aims to solve exactly the problem.
The data access functions ef_read and ef_write will return or write the data immdeiately and synchronously if it is
possible to do so with the available buffer. In case the data is not ready for read or buffer is not available for
write, immediately the functions return error by setting errno to EAGAIN.
The caller thread can now continue with other activities and wait for the availability of the file for operation
at a later time.

The module can easily be integrated to an eventing library such as libev, through usage of global and thread safe
queues. The eventing mechanism  is expected to implement a function pointer for signaling The event.

If the function pointer is provided, the EF module will wake up the eventing mudule
The inputs to the function  indicate which all files have become available for read and
which all files have become available for write.

The EF module identifies each opened file by its own index, equivalent of a file descriptor of the Unix kernel.
A caller is expected to open the file and pass the file descriptor to do any further opration.

A closed fd can be reused/reallocated by the library for some other file in the future.

