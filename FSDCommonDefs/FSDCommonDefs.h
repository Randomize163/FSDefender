#pragma once

//
//  Name of port used to communicate
//
const LPCWSTR g_wszFSDPortName = L"\\FSDCommunicationPort";

#define MAX_STRING_LENGTH 256
#define MAX_FILE_NAME_LENGTH MAX_STRING_LENGTH
#define MAX_FILE_NAME_SIZE (MAX_FILE_NAME_LENGTH * sizeof(WCHAR))

enum MESSAGE_TYPE {
    MESSAGE_TYPE_SET_SCAN_DIRECTORY,
    MESSAGE_TYPE_PRINT_STRING,
    MESSAGE_TYPE_SNIFFER_NEW_IRP,
};

struct FSD_MESSAGE_FORMAT
{
    MESSAGE_TYPE aType;
    union
    {
        WCHAR wszFileName[MAX_FILE_NAME_LENGTH];
    };
};