#ifndef ERRORCODE_H
#define ERRORCODE_H

/* TODO: Now that some error codes are a part of Task derivatives, probably should re-index them so that each
 * task has its own base number just like commands do, instead of sharing Core's base of 0, especially now
 * that both docker and QEMU are checked.
 */

enum ErrorCode{
    // Common
    NO_ERR = 0,
    ALREADY_OPEN = 1,
    INVALID_ARGS = 2,
    LAUNCHER_OPEN = 3,
    INSTALL_INVALID = 4,
    CONFIG_SERVER_MISSING = 5,
    SQL_ERROR = 6,
    SQL_MISMATCH = 7,
    EXECUTABLE_NOT_FOUND = 8,
    PROCESS_START_FAIL = 9,
    BIDE_PROCESS_NOT_HANDLED = 10,
    BIDE_PROCESS_NOT_HOOKED = 11,
    CANT_READ_BAT_FILE = 12,
    ID_NOT_VALID = 13,
    ID_NOT_FOUND = 14,
    ID_DUPLICATE = 15,
    TITLE_NOT_FOUND = 16,
    CANT_OBTAIN_DATA_PACK = 17,
    DATA_PACK_INVALID = 18,
    EXTRA_NOT_FOUND = 19,
    QMP_CONNECTION_FAIL = 20,
    QMP_COMMUNICATION_FAIL = 21,
    QMP_COMMAND_FAIL = 22,
    PHP_MOUNT_FAIL = 23,
    PACK_EXTRACT_FAIL = 24,
    CANT_QUERY_DOCKER = 25,
    CANT_LISTEN_DOCKER = 26,
    DOCKER_DIDNT_START = 27,
    TOO_MANY_RESULTS = 28,

    // CPlay
    RAND_FILTER_NOT_VALID = 101,
    PARENT_INVALID = 102,

    // CLink
    CANT_CREATE_SHORTCUT = 201,
};

#endif // ERRORCODE_H
