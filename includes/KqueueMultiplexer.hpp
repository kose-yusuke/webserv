#pragma once

#include "Multiplexer.hpp"

/**
 * kqueue を用いたI/O多重化
 */
class KqueueMultiplexer : public Multiplexer {
public:
  static Multiplexer &get_instance();

  void run();

protected:
  void add_to_read_fds(int fd);
  void remove_from_read_fds(int fd);
  void add_to_write_fds(int fd);
  void remove_from_write_fds(int fd);

private:
  typedef std::vector<struct kevent> KeventVec;
  typedef std::vector<struct kevent>::iterator KeventIt;
  typedef std::vector<struct kevent>::const_iterator ConstKeventIt;

  KeventVec change_list;
  KeventVec event_list;

  bool is_readable(struct kevent &ev) const;
  bool is_writable(struct kevent &ev) const;

  KqueueMultiplexer();
  KqueueMultiplexer(const KqueueMultiplexer &other);
  ~KqueueMultiplexer();

  KqueueMultiplexer &operator=(const KqueueMultiplexer &other);
};

// TODO: 作業用メモ. あとで消す
/*

KQUEUE(2)                   BSD System Calls Manual                  KQUEUE(2)

NAME
     kqueue, kevent -- kernel event notification mechanism

LIBRARY
     Standard C Library (libc, -lc)

SYNOPSIS
     #include <sys/types.h>
     #include <sys/event.h>
     #include <sys/time.h>

     int
     kqueue(void);

     int
     kevent(int kq, const struct kevent *changelist, int nchanges,
         struct kevent *eventlist, int nevents,
         const struct timespec *timeout);

     EV_SET(&kev, ident, filter, flags, fflags, data, udata);

DESCRIPTION
     The kqueue() system call provides a generic method of notifying the user
     when an kernel event (kevent) happens or a condition holds, based on the
     results of small pieces of kernel code termed filters.  A kevent is
iden-tified identified tified by an (ident, filter) pair and specifies the
interesting condi-tions conditions tions to be notified about for that pair.  An
(ident, filter) pair can only appear once is a given kqueue.  Subsequent
attempts to register the same pair for a given kqueue will result in the
replacement of the condi-tions conditions tions being watched, not an addition.

     The filter identified in a kevent is executed upon the initial
registra-tion registration tion of that event in order to detect whether a
preexisting condition is present, and is also executed whenever an event is
passed to the filter for evaluation.  If the filter determines that the
condition should be reported, then the kevent is placed on the kqueue for the
user to retrieve.

     The filter is also run when the user attempts to retrieve the kevent from
     the kqueue.  If the filter indicates that the condition that triggered
     the event no longer holds, the kevent is removed from the kqueue and is
     not returned.

     Multiple events which trigger the filter do not result in multiple
     kevents being placed on the kqueue; instead, the filter will aggregate
     the events into a single struct kevent.  Calling close() on a file
     descriptor will remove any kevents that reference the descriptor.

     The kqueue() system call creates a new kernel event queue and returns a
     descriptor.  The queue is not inherited by a child created with fork(2).

     The kevent() system call is used to register events with the queue, and
     return any pending events to the user.  The changelist argument is a
     pointer to an array of kevent structures, as defined in <sys/event.h>.
     All changes contained in the changelist are applied before any pending
     events are read from the queue.  The nchanges argument gives the size of
     changelist.  The eventlist argument is a pointer to an array of kevent
     structures.  The nevents argument determines the size of eventlist.  If
     timeout is a non-NULL pointer, it specifies a maximum interval to wait
     for an event, which will be interpreted as a struct timespec.  If timeout
     is a NULL pointer, kevent() waits indefinitely.  To effect a poll, the
     timeout argument should be non-NULL, pointing to a zero-valued timespec
     structure.  The same array may be used for the changelist and eventlist.

     The EV_SET() macro is provided for ease of initializing a kevent
struc-ture. structure. ture.
 */
// The kevent structure is defined as :

//     struct kevent {
//   uintptr_t ident; /* identifier for this event */
//   short filter;    /* filter for event */
//   u_short flags;   /* action flags for kqueue */
//   u_int fflags;    /* filter flag value */
//   intptr_t data;   /* filter data value */
//   void *udata;     /* opaque user data identifier */
// };
/*
     The fields of struct kevent are:

     ident      Value used to identify this event.  The exact interpretation
                is determined by the attached filter, but often is a file
                descriptor.

     filter     Identifies the kernel filter used to process this event.  The
                pre-defined system filters are described below.

     flags      Actions to perform on the event.

     fflags     Filter-specific flags.

     data       Filter-specific data value.

     udata      Opaque user-defined value passed through the kernel unchanged.

     The flags field can contain the following values:

     EV_ADD         Adds the event to the kqueue.  Re-adding an existing event
                    will modify the parameters of the original event, and not
                    result in a duplicate entry.  Adding an event automati-cally
automatically cally enables it, unless overridden by the EV_DISABLE flag.

     EV_ENABLE      Permit kevent() to return the event if it is triggered.

     EV_DISABLE     Disable the event so kevent() will not return it.  The
                    filter itself is not disabled.

     EV_DELETE      Removes the event from the kqueue.  Events which are
                    attached to file descriptors are automatically deleted on
                    the last close of the descriptor.

     EV_RECEIPT     This flag is useful for making bulk changes to a kqueue
                    without draining any pending events. When passed as input,
                    it forces EV_ERROR to always be returned.  When a filter
                    is successfully added. The data field will be zero.

     EV_ONESHOT     Causes the event to return only the first occurrence of
                    the filter being triggered.  After the user retrieves the
                    event from the kqueue, it is deleted.

     EV_CLEAR       After the event is retrieved by the user, its state is
                    reset.  This is useful for filters which report state
                    transitions instead of the current state.  Note that some
                    filters may automatically set this flag internally.

     EV_EOF         Filters may set this flag to indicate filter-specific EOF
                    condition.

     EV_ERROR       See RETURN VALUES below.

     The predefined system filters are listed below.  Arguments may be passed
     to and from the filter via the fflags and data fields in the kevent
     structure.

     EVFILT_READ    Takes a file descriptor as the identifier, and returns
                    whenever there is data available to read.  The behavior of
                    the filter is slightly different depending on the
descrip-tor descriptor tor type.

                    Sockets
                        Sockets which have previously been passed to listen()
                        return when there is an incoming connection pending.
                        data contains the size of the listen backlog.

                        Other socket descriptors return when there is data to
                        be read, subject to the SO_RCVLOWAT value of the
                        socket buffer.  This may be overridden with a
per-fil-ter per-filter ter low water mark at the time the filter is added by
                        setting the NOTE_LOWAT flag in fflags, and specifying
                        the new low water mark in data.  On return, data
con-tains contains tains the number of bytes of protocol data available to read.

                        If the read direction of the socket has shutdown, then
                        the filter also sets EV_EOF in flags, and returns the
                        socket error (if any) in fflags.  It is possible for
                        EOF to be returned (indicating the connection is gone)
                        while there is still data pending in the socket
                        buffer.

                    Vnodes
                        Returns when the file pointer is not at the end of
                        file.  data contains the offset from current position
                        to end of file, and may be negative.

                    Fifos, Pipes
                        Returns when the there is data to read; data contains
                        the number of bytes available.

                        When the last writer disconnects, the filter will set
                        EV_EOF in flags.  This may be cleared by passing in
                        EV_CLEAR, at which point the filter will resume wait-ing
waiting ing for data to become available before returning.

     EVFILT_WRITE   Takes a file descriptor as the identifier, and returns
                    whenever it is possible to write to the descriptor.  For
                    sockets, pipes and fifos, data will contain the amount of
                    space remaining in the write buffer.  The filter will set
                    EV_EOF when the reader disconnects, and for the fifo case,
                    this may be cleared by use of EV_CLEAR.  Note that this
                    filter is not supported for vnodes.

                    For sockets, the low water mark and socket error handling
                    is identical to the EVFILT_READ case.

     EVFILT_AIO     This filter is currently unsupported.

     EVFILT_VNODE   Takes a file descriptor as the identifier and the events
                    to watch for in fflags, and returns when one or more of
                    the requested events occurs on the descriptor.  The events
                    to monitor are:

                    NOTE_DELETE    The unlink() system call was called on the
                                   file referenced by the descriptor.

                    NOTE_WRITE     A write occurred on the file referenced by
                                   the descriptor.

                    NOTE_EXTEND    The file referenced by the descriptor was
                                   extended.

                    NOTE_ATTRIB    The file referenced by the descriptor had
                                   its attributes changed.

                    NOTE_LINK      The link count on the file changed.

                    NOTE_RENAME    The file referenced by the descriptor was
                                   renamed.

                    NOTE_REVOKE    Access to the file was revoked via
                                   revoke(2) or the underlying fileystem was
                                   unmounted.

                    On return, fflags contains the events which triggered the
                    filter.

     EVFILT_PROC    Takes the process ID to monitor as the identifier and the
                    events to watch for in fflags, and returns when the
                    process performs one or more of the requested events.  If
                    a process can normally see another process, it can attach
                    an event to it.  The events to monitor are:

                    NOTE_EXIT
                       The process has exited.

                    NOTE_FORK
                       The process created a child process via fork(2) or
sim-ilar similar ilar call.

                    NOTE_EXEC
                       The process executed a new process via execve(2) or
                       similar call.

                    NOTE_SIGNAL
                       The process was sent a signal. Status can be checked
                       via waitpid(2) or similar call.

                    NOTE_REAP
                       The process was reaped by the parent via wait(2) or
                       similar call.

                    On return, fflags contains the events which triggered the
                    filter.

     EVFILT_SIGNAL  Takes the signal number to monitor as the identifier and
                    returns when the given signal is delivered to the process.
                    This coexists with the signal() and sigaction() facili-ties,
facilities, ties, and has a lower precedence.  The filter will record all
attempts to deliver a signal to a process, even if the signal has been marked as
SIG_IGN.  Event notification happens after normal signal delivery processing.
data returns the number of times the signal has occurred since the last call to
kevent().  This filter automatically sets the EV_CLEAR flag internally.

     EVFILT_TIMER   This filter is currently unsupported.

RETURN VALUES
     The kqueue() system call creates a new kernel event queue and returns a
     file descriptor.  If there was an error creating the kernel event queue,
     a value of -1 is returned and errno set.

     The kevent() system call returns the number of events placed in the
     eventlist, up to the value given by nevents.  If an error occurs while
     processing an element of the changelist and there is enough room in the
     eventlist, then the event will be placed in the eventlist with EV_ERROR
     set in flags and the system error in data.  Otherwise, -1 will be
     returned, and errno will be set to indicate the error condition.  If the
     time limit expires, then kevent() returns 0.

ERRORS
     The kqueue() system call fails if:

     [ENOMEM]           The kernel failed to allocate enough memory for the
                        kernel queue.

     [EMFILE]           The per-process descriptor table is full.

     [ENFILE]           The system file table is full.

     The kevent() system call fails if:

     [EACCES]           The process does not have permission to register a
                        filter.

     [EFAULT]           There was an error reading or writing the kevent
                        structure.

     [EBADF]            The specified descriptor is invalid.

     [EINTR]            A signal was delivered before the timeout expired and
                        before any events were placed on the kqueue for
                        return.

     [EINVAL]           The specified time limit or filter is invalid.

     [ENOENT]           The event could not be found to be modified or
                        deleted.

     [ENOMEM]           No memory was available to register the event.

     [ESRCH]            The specified process to attach to does not exist.

SEE ALSO
     aio_error(2), aio_read(2), aio_return(2), read(2), select(2),
     sigaction(2), write(2), signal(3)

HISTORY
     The kqueue() and kevent() system calls first appeared in FreeBSD 4.1.

AUTHORS
     The kqueue() system and this manual page were written by Jonathan Lemon
     <jlemon@FreeBSD.org>.

BUGS
     Not all filesystem types support kqueue-style notifications.  And even
     some that do, like some remote filesystems, may only support a subset of
     the notification semantics described here.


*/
