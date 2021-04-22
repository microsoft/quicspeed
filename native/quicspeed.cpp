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
EventScope AllDoneEvent(true);
CXPLAT_THREAD_CALLBACK(QSWorker, Context);
QUIC_CONNECTION_CALLBACK ConnectionCallback;
QUIC_STREAM_CALLBACK StreamCallback;
void SendData(HQUIC Stream);
void OnStreamShutdownComplete();

bool DownloadTransfer = true;
bool TimedTransfer = true;
uint64_t TransferLength = 10 * 1000;
uint8_t RawBuffer[0x10000];
QUIC_BUFFER SendBuffer { sizeof(RawBuffer), RawBuffer };

struct StreamContext {
    uint64_t IdealSendBuffer{0x40000};
    uint64_t OutstandingBytes{0};
    uint64_t BytesSent{0};
    uint64_t BytesCompleted{0};
    uint64_t StartTime{0};
    uint64_t EndTime{0};
    QUIC_BUFFER LastBuffer;
    bool Complete{false};
    bool SendShutdown{false};
    bool RecvShutdown{false};
};

StreamContext StreamCtx;

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
}

extern "C"
void
QUIC_API
QSShutdown() {
    if (WorkerInitialized) {
        CxPlatEventSet(AllDoneEvent);
        CxPlatThreadDelete(&WorkerThread);
    }
    delete MsQuic;
}

typedef
void
(QUIC_API QS_RESULT_CALLBACK)(
    _In_ uint64_t SpeedKbps
    );

typedef QS_RESULT_CALLBACK *QS_RESULT_CALLBACK_HANDLER;

QS_RESULT_CALLBACK_HANDLER GlobalHandler;

extern "C"
void
QUIC_API
QSRunTransfer(QS_RESULT_CALLBACK_HANDLER ResultHandler) {

    GlobalHandler = ResultHandler;

    CXPLAT_THREAD_CONFIG ThreadConfig;
    CxPlatZeroMemory(&ThreadConfig, sizeof(ThreadConfig));
    ThreadConfig.Name = "QS-Worker";
    ThreadConfig.Callback = QSWorker;
    if (QUIC_SUCCEEDED(CxPlatThreadCreate(&ThreadConfig, &WorkerThread))) {
        WorkerInitialized = true;
    }
}

CXPLAT_THREAD_CALLBACK(QSWorker, /* Context */) {

    MsQuicRegistration Registration("QS", QUIC_EXECUTION_PROFILE_LOW_LATENCY, true);
    if (QUIC_FAILED(Registration.GetInitStatus())) {
        return 0;
    }

    MsQuicConfiguration Config(
        Registration,
        "perf",
        MsQuicSettings()
            .SetSendBufferingEnabled(false)
            .SetIdleTimeoutMs(1000),
        QUIC_CREDENTIAL_FLAG_CLIENT | QUIC_CREDENTIAL_FLAG_NO_CERTIFICATE_VALIDATION);
    if (QUIC_FAILED(Config.GetInitStatus())) {
        return 0;
    }

    ConnectionScope Connection;
    if (QUIC_FAILED(MsQuic->ConnectionOpen(Registration, ConnectionCallback, nullptr, &Connection.Handle))) {
        return 0;
    }

    if (DownloadTransfer) {
        SendBuffer.Length = sizeof(uint64_t);
        *(uint64_t*)(SendBuffer.Buffer) =
            TimedTransfer ? UINT64_MAX : CxPlatByteSwapUint64(TransferLength);
    } else {
        SendBuffer.Length = sizeof(RawBuffer);
        *(uint64_t*)(SendBuffer.Buffer) = CxPlatByteSwapUint64(0); // Zero-length request
        for (uint32_t i = 0; i < sizeof(RawBuffer) - sizeof(uint64_t); ++i) {
            SendBuffer.Buffer[sizeof(uint64_t) + i] = (uint8_t)i;
        }
    }

    StreamScope Stream;
    StreamCtx.StartTime = CxPlatTimeUs64();
    if (QUIC_FAILED(MsQuic->StreamOpen(Connection, QUIC_STREAM_OPEN_FLAG_NONE, StreamCallback, nullptr, &Stream.Handle))) {
        return 0;
    }
    if (QUIC_FAILED(MsQuic->StreamStart(Stream, QUIC_STREAM_START_FLAG_ASYNC))) {
        return 0;
    }

    if (DownloadTransfer) {
        MsQuic->StreamSend(
            Stream,
            &SendBuffer,
            1,
            QUIC_SEND_FLAG_FIN,
            &SendBuffer);
    } else {
        SendData(Stream);
    }

    if (QUIC_FAILED(MsQuic->ConnectionStart(Connection, Config, QUIC_ADDRESS_FAMILY_UNSPEC, "msquic.net", 4450))) {
        return 0;
    }

    CxPlatEventWaitForever(AllDoneEvent);

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

_IRQL_requires_max_(PASSIVE_LEVEL)
_Function_class_(QUIC_STREAM_CALLBACK)
QUIC_STATUS
QUIC_API
StreamCallback(
    _In_ HQUIC Stream,
    _In_opt_ void* /* Context */,
    _Inout_ QUIC_STREAM_EVENT* Event
    )
{
    switch (Event->Type) {
    case QUIC_STREAM_EVENT_RECEIVE:
        StreamCtx.BytesCompleted += Event->RECEIVE.TotalBufferLength;
        if (TimedTransfer) {
            if (CxPlatTimeDiff64(StreamCtx.StartTime, CxPlatTimeUs64()) >= MS_TO_US(TransferLength)) {
                MsQuic->StreamShutdown(Stream, QUIC_STREAM_SHUTDOWN_FLAG_ABORT_RECEIVE, 0);
                StreamCtx.Complete = true;
            }
        } else if (StreamCtx.BytesCompleted == TransferLength) {
            StreamCtx.Complete = true;
        }
        break;
    case QUIC_STREAM_EVENT_SEND_COMPLETE:
        if (TransferLength) {
            StreamCtx.OutstandingBytes -= ((QUIC_BUFFER*)Event->SEND_COMPLETE.ClientContext)->Length;
            if (!Event->SEND_COMPLETE.Canceled) {
                StreamCtx.BytesCompleted += ((QUIC_BUFFER*)Event->SEND_COMPLETE.ClientContext)->Length;
                SendData(Stream);
            }
        }
        break;
    case QUIC_STREAM_EVENT_PEER_SEND_ABORTED:
    case QUIC_STREAM_EVENT_PEER_RECEIVE_ABORTED:
        if (!StreamCtx.Complete) {
            //WriteOutput("Stream aborted\n");
        }
        MsQuic->StreamShutdown(Stream, QUIC_STREAM_SHUTDOWN_FLAG_ABORT, 0);
        break;
    case QUIC_STREAM_EVENT_SHUTDOWN_COMPLETE:
        OnStreamShutdownComplete();
        break;
    case QUIC_STREAM_EVENT_IDEAL_SEND_BUFFER_SIZE:
        if (TransferLength &&
            StreamCtx.IdealSendBuffer < Event->IDEAL_SEND_BUFFER_SIZE.ByteCount) {
            StreamCtx.IdealSendBuffer = Event->IDEAL_SEND_BUFFER_SIZE.ByteCount;
            SendData(Stream);
        }
        break;
    default:
        break;
    }
    return QUIC_STATUS_SUCCESS;
}

void SendData(HQUIC Stream)
{
    while (!StreamCtx.Complete && StreamCtx.OutstandingBytes < StreamCtx.IdealSendBuffer) {

        uint64_t BytesLeftToSend =
            TimedTransfer ? UINT64_MAX : (TransferLength - StreamCtx.BytesSent);
        uint32_t DataLength = sizeof(RawBuffer);
        QUIC_BUFFER* Buffer = &SendBuffer;
        QUIC_SEND_FLAGS Flags = QUIC_SEND_FLAG_NONE;

        if ((uint64_t)DataLength >= BytesLeftToSend) {
            DataLength = (uint32_t)BytesLeftToSend;
            StreamCtx.LastBuffer.Buffer = Buffer->Buffer;
            StreamCtx.LastBuffer.Length = DataLength;
            Buffer = &StreamCtx.LastBuffer;
            Flags = QUIC_SEND_FLAG_FIN;
            StreamCtx.Complete = TRUE;

        } else if (TimedTransfer &&
                   CxPlatTimeDiff64(StreamCtx.StartTime, CxPlatTimeUs64()) >= MS_TO_US(TransferLength)) {
            Flags = QUIC_SEND_FLAG_FIN;
            StreamCtx.Complete = TRUE;
        }

        StreamCtx.BytesSent += DataLength;
        StreamCtx.OutstandingBytes += DataLength;

        MsQuic->StreamSend(Stream, Buffer, 1, Flags, Buffer);
    }
}

void OnStreamShutdownComplete()
{
    uint64_t EndTime = CxPlatTimeUs64();
    uint64_t ElapsedMicroseconds = EndTime - StreamCtx.StartTime;
    uint32_t SendRate = (uint32_t)((StreamCtx.BytesCompleted * 1000 * 1000 * 8) / (1000 * ElapsedMicroseconds));
    UNREFERENCED_PARAMETER(SendRate);
    //SendRate;

    GlobalHandler(SendRate);

    if (StreamCtx.Complete) {
        /*WriteOutput(
            "Result: %llu bytes @ %u kbps (%u.%03u ms).\n",
            (unsigned long long)StreamCtx.BytesCompleted,
            SendRate,
            (uint32_t)(ElapsedMicroseconds / 1000),
            (uint32_t)(ElapsedMicroseconds % 1000));*/
    } else if (StreamCtx.BytesCompleted) {
        /*WriteOutput(
            "Error: Did not complete all bytes! %llu bytes @ %u kbps (%u.%03u ms).\n",
            (unsigned long long)StreamCtx.BytesCompleted,
            SendRate,
            (uint32_t)(ElapsedMicroseconds / 1000),
            (uint32_t)(ElapsedMicroseconds % 1000));*/
    } else {
        //WriteOutput("Error: Did not complete any bytes! Failed to connect?\n");
    }

    CxPlatEventSet(AllDoneEvent);
}
