/*++

    Copyright (c) Microsoft Corporation.
    Licensed under the MIT License.

Abstract:

    QuicSpeed Dynamic Link Library Entry Point

--*/

#include "quic_platform.h"
#include "msquic.hpp"

const MsQuicApi* MsQuic;
bool WorkerInitialized;
CXPLAT_THREAD WorkerThread;
CXPLAT_THREAD_CALLBACK(QSWorker, Context);
QUIC_CONNECTION_CALLBACK ConnectionCallback;

extern "C"
void
QUIC_API
QSInitialize() {
    MsQuic = new MsQuicApi();
    if (QUIC_FAILED(MsQuic->GetInitStatus())) {
        delete MsQuic;
        MsQuic = nullptr;
        return;
    }

    CXPLAT_THREAD_CONFIG ThreadConfig;
    CxPlatZeroMemory(&ThreadConfig, sizeof(ThreadConfig));
    ThreadConfig.Name = "QS-Worker";
    ThreadConfig.Callback = QSWorker;
    if (QUIC_SUCCEEDED(CxPlatThreadCreate(&ThreadConfig, &WorkerThread))) {
        WorkerInitialized = true;
    }
}

extern "C"
void
QUIC_API
QSShutdown() {
    if (WorkerInitialized) {
        CxPlatThreadDelete(&WorkerThread);
    }
    delete MsQuic;
}

#define BAIL_ON_INIT_FAILURE(X) if (QUIC_FAILED(X.GetInitStatus())) return 0

CXPLAT_THREAD_CALLBACK(QSWorker, /* Context */) {

    MsQuicRegistration Registration("QS", QUIC_EXECUTION_PROFILE_LOW_LATENCY, true);
    BAIL_ON_INIT_FAILURE(Registration);

    MsQuicConfiguration Config(
        Registration,
        "perf",
        MsQuicSettings().SetSendBufferingEnabled(false),
        QUIC_CREDENTIAL_FLAG_CLIENT | QUIC_CREDENTIAL_FLAG_NO_CERTIFICATE_VALIDATION);
    BAIL_ON_INIT_FAILURE(Config);

    {
        ConnectionScope Connection;
        if (QUIC_FAILED(MsQuic->ConnectionOpen(Registration, ConnectionCallback, nullptr, &Connection.Handle))) {
            return 0;
        }
        if (QUIC_FAILED(MsQuic->ConnectionStart(Connection, Config, QUIC_ADDRESS_FAMILY_UNSPEC, "msquic.net", 4450))) {
            return 0;
        }

        CxPlatSleep(5000);

        ;
    }

    CXPLAT_THREAD_RETURN(0);
}

_IRQL_requires_max_(PASSIVE_LEVEL)
_Function_class_(QUIC_CONNECTION_CALLBACK)
QUIC_STATUS
QUIC_API
ConnectionCallback(
    _In_ HQUIC /* Connection */,
    _In_opt_ void* /* Context */,
    _Inout_ QUIC_CONNECTION_EVENT* Event
    )
{
    if (Event->Type == QUIC_CONNECTION_EVENT_CONNECTED) {
        ; // YAY!!
    }
    return QUIC_STATUS_SUCCESS;
}
