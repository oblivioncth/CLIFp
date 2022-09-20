#ifndef ERRORCODE_H
#define ERRORCODE_H

/* TODO: Now that some error codes are a part of Task derivatives, probably should reindex them so that each
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
    EXECUTABLE_NOT_VALID = 9,
    PROCESS_START_FAIL = 10,
    BIDE_PROCESS_NOT_HANDLED = 11,
    BIDE_PROCESS_NOT_HOOKED = 12,
    CANT_READ_BAT_FILE = 13,
    ID_NOT_VALID = 14,
    ID_NOT_FOUND = 15,
    ID_DUPLICATE = 16,
    TITLE_NOT_FOUND = 17,
    CANT_OBTAIN_DATA_PACK = 18,
    DATA_PACK_INVALID = 19,
    EXTRA_NOT_FOUND = 20,
    QMP_CONNECTION_FAIL = 21,
    QMP_COMMUNICATION_FAIL = 22,
    QMP_COMMAND_FAIL = 23,
    PHP_MOUNT_FAIL = 24,
    PACK_EXTRACT_FAIL = 25,
    CANT_QUERY_DOCKER = 26,
    CANT_LISTEN_DOCKER = 27,
    DOCKER_DIDNT_START = 28,

    // CPlay
    RAND_FILTER_NOT_VALID = 101,
    PARENT_INVALID = 102,

    // CLink
    INVALID_SHORTCUT_PARAM = 201,
};

#endif // ERRORCODE_H
