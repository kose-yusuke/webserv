#pragma once

#include "Multiplexer.hpp"
#include <poll.h>
#include <vector>

/**
 * poll を用いた I/O 多重化
 */
class PollMultiplexer : public Multiplexer {
public:
  static Multiplexer &get_instance();

  void run();

protected:
  void add_to_read_fds(int fd);
  void remove_from_read_fds(int fd);
  void add_to_write_fds(int fd);
  void remove_from_write_fds(int fd);

private:
  typedef std::vector<struct pollfd> PollFdVec;
  typedef std::vector<struct pollfd>::iterator PollFdIt;

  PollFdVec pfds;

  bool is_readable(struct pollfd fd) const;
  bool is_writable(struct pollfd fd) const;

  PollFdIt find_pollfd(int fd);

  PollMultiplexer();
  PollMultiplexer(const PollMultiplexer &other);
  ~PollMultiplexer();

  PollMultiplexer &operator=(const PollMultiplexer &other);
};

/*

The bits that may be set/returned in events and revents are
       defined in <poll.h>:

       POLLIN There is data to read.

       POLLPRI
              There is some exceptional condition on the file descriptor.
              Possibilities include:

              •  There is out-of-band data on a TCP socket (see tcp(7)).

              •  A pseudoterminal master in packet mode has seen a state
                 change on the slave (see ioctl_tty(2)).

              •  A cgroup.events file has been modified (see cgroups(7)).

       POLLOUT
              Writing is now possible, though a write larger than the
              available space in a socket or pipe will still block
              (unless O_NONBLOCK is set).

       POLLRDHUP (since Linux 2.6.17)
              Stream socket peer closed connection, or shut down writing
              half of connection.  The _GNU_SOURCE feature test macro
              must be defined (before including any header files) in
              order to obtain this definition.

       POLLERR
              Error condition (only returned in revents; ignored in
              events).  This bit is also set for a file descriptor
              referring to the write end of a pipe when the read end has
              been closed.

       POLLHUP
              Hang up (only returned in revents; ignored in events).
              Note that when reading from a channel such as a pipe or a
              stream socket, this event merely indicates that the peer
              closed its end of the channel.  Subsequent reads from the
              channel will return 0 (end of file) only after all
              outstanding data in the channel has been consumed.

       POLLNVAL
              Invalid request: fd not open (only returned in revents;
              ignored in events).

       When compiling with _XOPEN_SOURCE defined, one also has the
       following, which convey no further information beyond the bits
       listed above:

       POLLRDNORM
              Equivalent to POLLIN.

       POLLRDBAND
              Priority band data can be read (generally unused on Linux).

       POLLWRNORM
              Equivalent to POLLOUT.

       POLLWRBAND
              Priority data may be written.

       Linux also knows about, but does not use POLLMSG.





       RETURN VALUE         top

       On success, poll() returns a nonnegative value which is the number
       of elements in the pollfds whose revents fields have been set to a
       nonzero value (indicating an event or an error).  A return value
       of zero indicates that the system call timed out before any file
       descriptors became ready.

       On error, -1 is returned, and errno is set to indicate the error.

ERRORS         top

       EFAULT fds points outside the process's accessible address space.
              The array given as argument was not contained in the
              calling program's address space.

       EINTR  A signal occurred before any requested event; see
              signal(7).

       EINVAL The nfds value exceeds the RLIMIT_NOFILE value.

       EINVAL (ppoll()) The timeout value expressed in *tmo_p is invalid
              (negative).

       ENOMEM Unable to allocate memory for kernel data structures.

*/
