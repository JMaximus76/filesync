Project name: Jacob's File Sync

General Naming:
    Public tokens must be prepended with (jfs_[mod id]_)
    Private tokens must be prepended with ([mod id]_)
    Public typedefs must be postpended with (_t)

Struct Naming:
    Public structs must be defined using the struct keyword, and must also
    expose all members. Struct members that are themseles structs should use
    the typedef form where available.

Double ptr should be named with:
    *_ptr  (pre allocated) (ownership kept)
    *_give (pre allocated) (ownership lost)
    *_take (not allocated) (ownership gained)


System Requirements:
    - Linux kernel 5.10+ (inotify, epoll)
    - max_user_watches at least 16384 recommended

Other:
    - All char buffers for file names and file paths should use PATH_MAX + 1 and NAME_MAX + 1
    - If an error occurs MUST guarantee that all double ptrs remain unchanged AND their memory is intact


Modules:
    Net Socket (jfs_ns_*)
        - TCP socket management
        - send/recv wrappers
        - socket file descriptor manageent

    File Walk (jfs_fw_*)
        - directory scanning
        - stat metadata collection
        - controlls tree walks

    File IO (jfs_fio_*)
        - file read/write wrappers


Errors:
    Error type: jfs_error_t
    Format 1: JFS_ERR_*
    Format 2: JFS_[mod abvr]_ERR_*

    JFS_OK (equals 0)
        _ No error.

    JFS_ERR_RESOURCE
        - Unrecoverable system resource exhaustion. Program must exit.



