#pragma once

#include <fltKernel.h>
#include "CFSDefender.h"

//
//  operation registration
//

CONST FLT_OPERATION_REGISTRATION Callbacks[] = {
    { IRP_MJ_CREATE,
    0,
    CFSDefender::FSDPreOperation,
    CFSDefender::FSDPostOperation },
   
    { IRP_MJ_CLOSE,
    0,
    CFSDefender::FSDPreOperation,
    CFSDefender::FSDPostOperation },

    { IRP_MJ_READ,
    0,
    CFSDefender::FSDPreOperation,
    CFSDefender::FSDPostOperation },

    { IRP_MJ_WRITE,
    0,
    CFSDefender::FSDPreOperation,
    CFSDefender::FSDPostOperation },

   /*{ IRP_MJ_QUERY_INFORMATION,
    0,
    CFSDefender::FSDPreOperation,
    CFSDefender::FSDPostOperation },*/

    { IRP_MJ_SET_INFORMATION,
    0,
    CFSDefender::FSDPreOperation,
    CFSDefender::FSDPostOperation },

    /*{ IRP_MJ_DIRECTORY_CONTROL,
    0,
    CFSDefender::FSDPreOperation,
    CFSDefender::FSDPostOperation },

    { IRP_MJ_CREATE_NAMED_PIPE,
    0,
    CFSDefender::FSDPreOperation,
    CFSDefender::FSDPostOperation },

    { IRP_MJ_QUERY_EA,
    0,
    CFSDefender::FSDPreOperation,
    CFSDefender::FSDPostOperation },

    { IRP_MJ_SET_EA,
    0,
    CFSDefender::FSDPreOperation,
    CFSDefender::FSDPostOperation },

    { IRP_MJ_FLUSH_BUFFERS,
    0,
    CFSDefender::FSDPreOperation,
    CFSDefender::FSDPostOperation },

    { IRP_MJ_QUERY_VOLUME_INFORMATION,
    0,
    CFSDefender::FSDPreOperation,
    CFSDefender::FSDPostOperation },

    { IRP_MJ_SET_VOLUME_INFORMATION,
    0,
    CFSDefender::FSDPreOperation,
    CFSDefender::FSDPostOperation },

    { IRP_MJ_FILE_SYSTEM_CONTROL,
    0,
    CFSDefender::FSDPreOperation,
    CFSDefender::FSDPostOperation },

    { IRP_MJ_DEVICE_CONTROL,
    0,
    CFSDefender::FSDPreOperation,
    CFSDefender::FSDPostOperation },

    { IRP_MJ_INTERNAL_DEVICE_CONTROL,
    0,
    CFSDefender::FSDPreOperation,
    CFSDefender::FSDPostOperation },

    { IRP_MJ_SHUTDOWN,
    0,
    CFSDefender::FSDPreOperationNoPostOperation,
    NULL },                               //post operations not supported

    { IRP_MJ_LOCK_CONTROL,
    0,
    CFSDefender::FSDPreOperation,
    CFSDefender::FSDPostOperation },
    */
    { IRP_MJ_CLEANUP,
    0,
    CFSDefender::FSDPreOperation,
    CFSDefender::FSDPostOperation },
    /*
    { IRP_MJ_CREATE_MAILSLOT,
    0,
    CFSDefender::FSDPreOperation,
    CFSDefender::FSDPostOperation },

    { IRP_MJ_QUERY_SECURITY,
    0,
    CFSDefender::FSDPreOperation,
    CFSDefender::FSDPostOperation },

    { IRP_MJ_SET_SECURITY,
    0,
    CFSDefender::FSDPreOperation,
    CFSDefender::FSDPostOperation },

    { IRP_MJ_QUERY_QUOTA,
    0,
    CFSDefender::FSDPreOperation,
    CFSDefender::FSDPostOperation },

    { IRP_MJ_SET_QUOTA,
    0,
    CFSDefender::FSDPreOperation,
    CFSDefender::FSDPostOperation },

    { IRP_MJ_PNP,
    0,
    CFSDefender::FSDPreOperation,
    CFSDefender::FSDPostOperation },

    { IRP_MJ_ACQUIRE_FOR_SECTION_SYNCHRONIZATION,
    0,
    CFSDefender::FSDPreOperation,
    CFSDefender::FSDPostOperation },

    { IRP_MJ_RELEASE_FOR_SECTION_SYNCHRONIZATION,
    0,
    CFSDefender::FSDPreOperation,
    CFSDefender::FSDPostOperation },

    { IRP_MJ_ACQUIRE_FOR_MOD_WRITE,
    0,
    CFSDefender::FSDPreOperation,
    CFSDefender::FSDPostOperation },

    { IRP_MJ_RELEASE_FOR_MOD_WRITE,
    0,
    CFSDefender::FSDPreOperation,
    CFSDefender::FSDPostOperation },

    { IRP_MJ_ACQUIRE_FOR_CC_FLUSH,
    0,
    CFSDefender::FSDPreOperation,
    CFSDefender::FSDPostOperation },

    { IRP_MJ_RELEASE_FOR_CC_FLUSH,
    0,
    CFSDefender::FSDPreOperation,
    CFSDefender::FSDPostOperation },

    { IRP_MJ_FAST_IO_CHECK_IF_POSSIBLE,
    0,
    CFSDefender::FSDPreOperation,
    CFSDefender::FSDPostOperation },
    
    { IRP_MJ_NETWORK_QUERY_OPEN,
    0,
    CFSDefender::FSDPreOperation,
    CFSDefender::FSDPostOperation },

    { IRP_MJ_MDL_READ,
    0,
    CFSDefender::FSDPreOperation,
    CFSDefender::FSDPostOperation },

    { IRP_MJ_MDL_READ_COMPLETE,
    0,
    CFSDefender::FSDPreOperation,
    CFSDefender::FSDPostOperation },

    { IRP_MJ_PREPARE_MDL_WRITE,
    0,
    CFSDefender::FSDPreOperation,
    CFSDefender::FSDPostOperation },

    { IRP_MJ_MDL_WRITE_COMPLETE,
    0,
    CFSDefender::FSDPreOperation,
    CFSDefender::FSDPostOperation },

    { IRP_MJ_VOLUME_MOUNT,
    0,
    CFSDefender::FSDPreOperation,
    CFSDefender::FSDPostOperation },

    { IRP_MJ_VOLUME_DISMOUNT,
    0,
    CFSDefender::FSDPreOperation,
    CFSDefender::FSDPostOperation },*/

{ IRP_MJ_OPERATION_END }
};

//
//  This defines what we want to filter with FltMgr
//

CONST FLT_REGISTRATION FilterRegistration = {

    sizeof(FLT_REGISTRATION),         //  Size
    FLT_REGISTRATION_VERSION,           //  Version
    0,                                  //  Flags

    NULL,                               //  Context
    Callbacks,                          //  Operation callbacks

    CFSDefender::FSDUnload,                           //  MiniFilterUnload

    CFSDefender::FSDInstanceSetup,                    //  InstanceSetup
    CFSDefender::FSDInstanceQueryTeardown,            //  InstanceQueryTeardown
    CFSDefender::FSDInstanceTeardownStart,            //  InstanceTeardownStart
    CFSDefender::FSDInstanceTeardownComplete,         //  InstanceTeardownComplete

    NULL,                               //  GenerateFileName
    NULL,                               //  GenerateDestinationFileName
    NULL                                //  NormalizeNameComponent

};