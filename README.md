# Jacob's File Sync

## General Naming

- Public tokens must be prepended with: `jfs_[mod id]_`
- Private tokens must be prepended with: `[mod id]_`
- Public typedefs must be postpended with: `_t`

### Struct Naming

- Public structs must be defined using the `struct` keyword and must expose all members.
- Struct members that are themselves structs should use the typedef form where available.

### Double Pointer Ownership Conventions

- `*_ptr` → Pre-allocated (ownership kept)  
- `*_give` → Pre-allocated (ownership lost)  
- `*_take` → Not allocated (ownership gained)  

## System Requirements

- Linux kernel 5.10+ (inotify, epoll)
- `max_user_watches` at least 16,384 recommended

## Other Notes

- All `char` buffers for file names and file paths should use `PATH_MAX + 1` and `NAME_MAX + 1`.
- If an error occurs, it must guarantee that all double pointers remain unchanged and their memory is intact.

## Modules

### Net Socket (`jfs_ns_*`)
- TCP socket management
- `send`/`recv` wrappers
- Socket file descriptor management

### File Walk (`jfs_fw_*`)
- Directory scanning
- `stat` metadata collection
- Controls tree walks

### File IO (`jfs_fio_*`)
- File read/write wrappers

## Errors

- **Error type:** `jfs_error_t`  
- **Format 1:** `JFS_ERR_*`  
- **Format 2:** `JFS_[mod abvr]_ERR_*`

### Defined Errors

- `JFS_OK` (equals 0)  
  _No error._

- `JFS_ERR_RESOURCE`  
  Unrecoverable system resource exhaustion. Program must exit.

