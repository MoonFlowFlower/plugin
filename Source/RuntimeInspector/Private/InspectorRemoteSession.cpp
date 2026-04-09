#include "InspectorWorldSubsystem.h"
#include "InspectorPropertyUtils.h"

#include "Algo/Sort.h"
#include "Dom/JsonObject.h"
#include "Engine/Engine.h"
#include "Engine/Blueprint.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "HAL/PlatformProcess.h"
#include "HAL/PlatformMisc.h"
#include "IPAddress.h"
#include "JsonObjectConverter.h"
#include "Misc/Base64.h"
#include "Misc/DateTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "SocketSubsystem.h"
#include "Sockets.h"

#if WITH_EDITOR
#include "Editor.h"
#endif

namespace
{
    static const FName RI_RemoteCompareMatrixId_Default(TEXT("mainline_remote_session_compare_matrix_default"));
    static constexpr int32 RI_RemoteExpectedExternalProtocolVersion = 1;
    static constexpr TCHAR RI_RemoteExternalHost[] = TEXT("127.0.0.1");
    static constexpr int32 RI_RemoteExternalMinPort = 9897;
    static constexpr int32 RI_RemoteExternalMaxPort = 9901;
    static constexpr double RI_RemoteExternalProbeCacheSeconds = 15.0;
    static constexpr double RI_RemoteTargetQueryCacheSeconds = 5.0;
    static constexpr double RI_RemoteExternalDefaultTimeoutSeconds = 1.5;
    static constexpr double RI_RemoteExternalProbeTimeoutSeconds = 0.05;
    static constexpr double RI_RemotePackagedValidationEnsureTimeoutSeconds = 30.0;

    static FString RI_RemotePackagedValidationRoot()
    {
        return FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("RuntimeInspector"), TEXT("PackagedRuntimeValidation")));
    }

    static FString RI_RemotePackagedValidationStatePath()
    {
        return FPaths::Combine(RI_RemotePackagedValidationRoot(), TEXT("state.json"));
    }

    static FString RI_RemotePackagedValidationScriptPath(const TCHAR* ScriptName)
    {
        return FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectPluginsDir(), TEXT("RuntimeInspector"), TEXT("Scripts"), ScriptName));
    }

    static FString RI_RemotePackagedValidationLogSuffix(const FString& InText, int32 MaxLen = 480)
    {
        FString Compact = InText;
        Compact = Compact.Replace(TEXT("\r"), TEXT(" "));
        Compact = Compact.Replace(TEXT("\n"), TEXT(" "));
        Compact = Compact.TrimStartAndEnd();
        if (Compact.Len() > MaxLen)
        {
            Compact = Compact.Left(MaxLen) + TEXT("...");
        }
        return Compact;
    }

    static bool RI_RemoteExecuteWindowsScript(const FString& ScriptPath, FString& OutStdOut, FString& OutStdErr, int32& OutExitCode)
    {
        OutStdOut.Reset();
        OutStdErr.Reset();
        OutExitCode = -1;

#if !PLATFORM_WINDOWS
        OutStdErr = TEXT("Packaged validation scripts require Windows editor authority");
        return false;
#else
        if (!FPaths::FileExists(ScriptPath))
        {
            OutStdErr = FString::Printf(TEXT("Script not found: %s"), *ScriptPath);
            return false;
        }

        const FString ComSpec = FPlatformMisc::GetEnvironmentVariable(TEXT("ComSpec"));
        const FString CmdExe = ComSpec.IsEmpty() ? TEXT("cmd.exe") : ComSpec;
        const FString Args = FString::Printf(TEXT("/c \"%s\""), *ScriptPath);
        return FPlatformProcess::ExecProcess(*CmdExe, *Args, &OutExitCode, &OutStdOut, &OutStdErr);
#endif
    }

    static FString RI_RemoteWorldTypeLabel(EWorldType::Type WorldType)
    {
        switch (WorldType)
        {
        case EWorldType::Editor: return TEXT("Editor");
        case EWorldType::PIE: return TEXT("PIE");
        case EWorldType::Game: return TEXT("Game");
        case EWorldType::GamePreview: return TEXT("GamePreview");
        case EWorldType::EditorPreview: return TEXT("EditorPreview");
        case EWorldType::GameRPC: return TEXT("GameRPC");
        case EWorldType::Inactive: return TEXT("Inactive");
        case EWorldType::None: return TEXT("None");
        default: return TEXT("Unknown");
        }
    }

    static FString RI_RemoteNetModeLabel(ENetMode NetMode)
    {
        switch (NetMode)
        {
        case NM_Standalone: return TEXT("Standalone");
        case NM_DedicatedServer: return TEXT("DedicatedServer");
        case NM_ListenServer: return TEXT("ListenServer");
        case NM_Client: return TEXT("Client");
        default: return TEXT("Unknown");
        }
    }

    static FString RI_RemoteBuildLocalSessionId(const FRIRuntimeSessionSummary& SessionSummary)
    {
        return SessionSummary.bIsPIEWorld ? TEXT("local_pie_current") : TEXT("local_runtime_current");
    }

    static FString RI_RemoteBuildLocalSessionDisplayName(const FRIRuntimeSessionSummary& SessionSummary)
    {
        return SessionSummary.bIsPIEWorld ? TEXT("Local PIE Current") : TEXT("Local Runtime Current");
    }

    static FString RI_RemoteActorLabel(const AActor* Actor)
    {
        if (!Actor)
        {
            return FString();
        }

#if WITH_EDITOR
        return Actor->GetActorLabel();
#else
        return Actor->GetName();
#endif
    }

    static APlayerController* RI_RemoteFindLocalPlayerController(const UWorld* World)
    {
        if (!World)
        {
            return nullptr;
        }

        for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
        {
            APlayerController* PlayerController = It->Get();
            if (PlayerController && PlayerController->IsLocalController())
            {
                return PlayerController;
            }
        }

        return nullptr;
    }

    static FRIRuntimeSessionSummary RI_RemoteBuildSessionSummaryForWorld(const UWorld* World)
    {
        FRIRuntimeSessionSummary Result;
        if (!World)
        {
            Result.Summary = TEXT("World unavailable");
            return Result;
        }

        Result.bSessionAvailable = true;
        Result.bIsGameWorld = World->IsGameWorld();
        Result.bIsPIEWorld = World->WorldType == EWorldType::PIE;
        Result.WorldTypeLabel = RI_RemoteWorldTypeLabel(World->WorldType);
        Result.NetModeLabel = RI_RemoteNetModeLabel(World->GetNetMode());
        Result.MapName = World->GetMapName();

        if (APlayerController* LocalPC = RI_RemoteFindLocalPlayerController(World))
        {
            Result.bHasLocalPlayerController = true;
            Result.LocalPlayerControllerPath = LocalPC->GetPathName();
        }

        Result.Summary = FString::Printf(
            TEXT("%s | Net=%s | LocalPC=%s"),
            *Result.WorldTypeLabel,
            *Result.NetModeLabel,
            Result.bHasLocalPlayerController ? TEXT("yes") : TEXT("no"));
        return Result;
    }

    static FString RI_RemoteConnectionStateLabel(ERIRuntimeSessionConnectionState InState)
    {
        switch (InState)
        {
        case ERIRuntimeSessionConnectionState::Disconnected:
            return TEXT("Disconnected");
        case ERIRuntimeSessionConnectionState::Connected:
            return TEXT("Connected");
        case ERIRuntimeSessionConnectionState::Error:
            return TEXT("Error");
        default:
            return TEXT("Unknown");
        }
    }

    static FString RI_RemoteBuildSessionSummaryText(const FRIRuntimeSessionInfo& Session)
    {
        return FString::Printf(
            TEXT("%s | %s | Runtime=%s | Unlock=%s | Connect=%s"),
            *Session.DisplayName,
            Session.NetModeLabel.IsEmpty() ? TEXT("Unknown") : *Session.NetModeLabel,
            Session.bRuntimeEnabled ? TEXT("enabled") : TEXT("disabled"),
            Session.bUnlockRequired ? (Session.bUnlocked ? TEXT("unlocked") : TEXT("locked")) : TEXT("not-required"),
            *RI_RemoteConnectionStateLabel(Session.ConnectionState));
    }

    static FString RI_RemoteBuildTargetSummaryText(const FRIRuntimeTargetInfo& Target)
    {
        return FString::Printf(
            TEXT("%s | Class=%s | Role=%s/%s | Rep=%s"),
            Target.ActorLabel.IsEmpty() ? *Target.ActorName : *Target.ActorLabel,
            Target.ActorClass.IsEmpty() ? TEXT("Unknown") : *Target.ActorClass,
            Target.LocalRoleLabel.IsEmpty() ? TEXT("None") : *Target.LocalRoleLabel,
            Target.RemoteRoleLabel.IsEmpty() ? TEXT("None") : *Target.RemoteRoleLabel,
            Target.bReplicates ? TEXT("yes") : TEXT("no"));
    }

    static UWorld* RI_RemoteGetEditorWorld()
    {
#if WITH_EDITOR
        return GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
#else
        return nullptr;
#endif
    }

    static UWorld* RI_RemoteGetPIEWorld()
    {
#if WITH_EDITOR
        return GEditor ? GEditor->PlayWorld : nullptr;
#else
        return nullptr;
#endif
    }

    static UWorld* RI_RemoteResolveWorldForSessionId(const UInspectorWorldSubsystem* Subsystem, const FString& SessionId)
    {
        const FString NormalizedSessionId = SessionId.TrimStartAndEnd();
        if (NormalizedSessionId == TEXT("local_pie_current"))
        {
            return RI_RemoteGetPIEWorld();
        }

        if (NormalizedSessionId == TEXT("local_editor_current"))
        {
            return RI_RemoteGetEditorWorld();
        }

        if (NormalizedSessionId == TEXT("local_runtime_current"))
        {
            if (UWorld* EditorWorld = RI_RemoteGetEditorWorld())
            {
                return EditorWorld;
            }

            return Subsystem ? Subsystem->GetWorld() : nullptr;
        }

        return nullptr;
    }

    static const FRIRuntimeTargetInfo* RI_RemoteFindTargetByQuery(const TArray<FRIRuntimeTargetInfo>& Targets, const FString& Query)
    {
        const FString NormalizedQuery = Query.TrimStartAndEnd();
        if (NormalizedQuery.IsEmpty())
        {
            return nullptr;
        }

        const FRIRuntimeTargetInfo* ExactMatch = Targets.FindByPredicate([&NormalizedQuery](const FRIRuntimeTargetInfo& Target)
        {
            return Target.ActorPath == NormalizedQuery
                || Target.ActorLabel == NormalizedQuery
                || Target.ActorName == NormalizedQuery;
        });
        if (ExactMatch)
        {
            return ExactMatch;
        }

        const FString QueryLower = NormalizedQuery.ToLower();
        return Targets.FindByPredicate([&QueryLower](const FRIRuntimeTargetInfo& Target)
        {
            return Target.ActorPath.ToLower().Contains(QueryLower)
                || Target.ActorLabel.ToLower().Contains(QueryLower)
                || Target.ActorName.ToLower().Contains(QueryLower)
                || Target.ActorClass.ToLower().Contains(QueryLower)
                || Target.ActorClassPath.ToLower().Contains(QueryLower);
        });
    }

    static AActor* RI_RemoteFindActorByRequest(UWorld* World, const FString& Query)
    {
        if (!World)
        {
            return nullptr;
        }

        const FString NormalizedQuery = Query.TrimStartAndEnd();
        if (NormalizedQuery.IsEmpty())
        {
            return nullptr;
        }

        const FString QueryLower = NormalizedQuery.ToLower();
        for (TActorIterator<AActor> It(World); It; ++It)
        {
            AActor* Actor = *It;
            if (!Actor)
            {
                continue;
            }

            const FString ActorName = Actor->GetName();
            const FString ActorLabel = Actor->GetActorNameOrLabel();
            const FString ActorPath = Actor->GetPathName();
            const FString ActorClass = Actor->GetClass() ? Actor->GetClass()->GetName() : FString();
            const FString ActorClassPath = Actor->GetClass() ? Actor->GetClass()->GetPathName() : FString();

            if (ActorPath == NormalizedQuery
                || ActorName == NormalizedQuery
                || ActorLabel == NormalizedQuery
                || ActorPath.ToLower().Contains(QueryLower)
                || ActorName.ToLower().Contains(QueryLower)
                || ActorLabel.ToLower().Contains(QueryLower)
                || ActorClass.ToLower().Contains(QueryLower)
                || ActorClassPath.ToLower().Contains(QueryLower))
            {
                return Actor;
            }
        }

        return nullptr;
    }

    static FString RI_RemoteBuildTargetSetCompareKey(const FRIRuntimeTargetInfo& Target)
    {
        return FString::Printf(TEXT("%s|%s"), *Target.ActorName, *Target.ActorClass);
    }

    static void RI_RemoteAddCompareField(
        FRIRuntimeSessionTargetCompareReport& Report,
        const TCHAR* FieldName,
        const FString& LeftValue,
        const FString& RightValue)
    {
        FRIRuntimeSessionCompareField Field;
        Field.FieldName = FieldName;
        Field.LeftValue = LeftValue.IsEmpty() ? TEXT("-") : LeftValue;
        Field.RightValue = RightValue.IsEmpty() ? TEXT("-") : RightValue;
        Field.bDifferent = Field.LeftValue != Field.RightValue;
        Field.Message = Field.bDifferent ? TEXT("Different") : TEXT("Same");
        Report.Fields.Add(Field);
        Report.FieldCount = Report.Fields.Num();
        if (Field.bDifferent)
        {
            ++Report.DifferenceCount;
        }
    }

    static FRIRuntimeSessionTargetSetCompareRequest RI_RemoteResolveTargetSetCompareRequest(const FRIRuntimeSessionTargetSetCompareRequest& InRequest)
    {
        FRIRuntimeSessionTargetSetCompareRequest Resolved = InRequest;
        Resolved.LeftSessionId = Resolved.LeftSessionId.TrimStartAndEnd();
        Resolved.RightSessionId = Resolved.RightSessionId.TrimStartAndEnd();
        Resolved.NameFilter = Resolved.NameFilter.TrimStartAndEnd();
        Resolved.ClassFilter = Resolved.ClassFilter.TrimStartAndEnd();
        if (Resolved.LeftSessionId.IsEmpty())
        {
            Resolved.LeftSessionId = TEXT("local_editor_current");
        }
        if (Resolved.RightSessionId.IsEmpty())
        {
            Resolved.RightSessionId = TEXT("local_pie_current");
        }
        return Resolved;
    }

    static bool RI_IsExternalPackagedSessionId(const FString& SessionId)
    {
        return SessionId.StartsWith(TEXT("external_packaged_"), ESearchCase::CaseSensitive);
    }

    static bool RI_TryParseExternalPackagedPort(const FString& SessionId, int32& OutPort)
    {
        OutPort = 0;
        if (!RI_IsExternalPackagedSessionId(SessionId))
        {
            return false;
        }

        const FString PortText = SessionId.RightChop(FString(TEXT("external_packaged_")).Len());
        return LexTryParseString(OutPort, *PortText) && OutPort > 0;
    }

    template <typename TStructType>
    static bool RI_RemoteJsonObjectToStruct(const TSharedPtr<FJsonObject>& Object, TStructType& OutStruct)
    {
        OutStruct = TStructType();
        return Object.IsValid()
            && FJsonObjectConverter::JsonObjectToUStruct<TStructType>(Object.ToSharedRef(), &OutStruct, 0, 0);
    }

    static bool RI_RemoteSendAll(FSocket* Socket, const uint8* Data, int32 ByteCount, FString& OutError)
    {
        OutError.Reset();
        if (!Socket || !Data || ByteCount <= 0)
        {
            OutError = TEXT("Invalid socket send request");
            return false;
        }

        int32 TotalSent = 0;
        while (TotalSent < ByteCount)
        {
            int32 Sent = 0;
            if (!Socket->Send(Data + TotalSent, ByteCount - TotalSent, Sent) || Sent <= 0)
            {
                OutError = TEXT("Socket send failed");
                return false;
            }
            TotalSent += Sent;
        }
        return true;
    }

    static bool RI_RemoteConnectWithTimeout(
        ISocketSubsystem* SocketSubsystem,
        FSocket* Socket,
        const TSharedRef<FInternetAddr>& Address,
        double TimeoutSeconds,
        FString& OutError)
    {
        OutError.Reset();
        if (!SocketSubsystem || !Socket || TimeoutSeconds <= 0.0)
        {
            OutError = TEXT("Invalid socket connect request");
            return false;
        }

        Socket->SetNonBlocking(true);

        if (Socket->Connect(*Address))
        {
            Socket->SetNonBlocking(false);
            return true;
        }

        const ESocketErrors InitialError = SocketSubsystem->GetLastErrorCode();
        if (InitialError != SE_EWOULDBLOCK
            && InitialError != SE_EINPROGRESS
            && InitialError != SE_TRY_AGAIN
            && InitialError != SE_NO_ERROR)
        {
            Socket->SetNonBlocking(false);
            OutError = FString::Printf(TEXT("Failed to connect to %s (%d)"), *Address->ToString(true), static_cast<int32>(InitialError));
            return false;
        }

        const double Deadline = FPlatformTime::Seconds() + TimeoutSeconds;
        while (FPlatformTime::Seconds() < Deadline)
        {
            const ESocketConnectionState ConnectionState = Socket->GetConnectionState();
            if (ConnectionState == SCS_Connected)
            {
                Socket->SetNonBlocking(false);
                return true;
            }

            if (ConnectionState == SCS_ConnectionError)
            {
                Socket->SetNonBlocking(false);
                OutError = FString::Printf(TEXT("Failed to connect to %s"), *Address->ToString(true));
                return false;
            }

            const double RemainingSeconds = Deadline - FPlatformTime::Seconds();
            if (RemainingSeconds <= 0.0)
            {
                break;
            }

            Socket->Wait(ESocketWaitConditions::WaitForWrite, FTimespan::FromSeconds(FMath::Min(RemainingSeconds, 0.01)));
        }

        Socket->SetNonBlocking(false);
        OutError = FString::Printf(TEXT("Timed out connecting to %s"), *Address->ToString(true));
        return false;
    }

    static bool RI_RemoteReceiveExact(FSocket* Socket, uint8* Dest, int32 ByteCount, double TimeoutSeconds, FString& OutError)
    {
        OutError.Reset();
        if (!Socket || !Dest || ByteCount <= 0)
        {
            OutError = TEXT("Invalid socket receive request");
            return false;
        }

        const double Deadline = FPlatformTime::Seconds() + TimeoutSeconds;
        int32 TotalRead = 0;
        while (TotalRead < ByteCount)
        {
            const double RemainingSeconds = Deadline - FPlatformTime::Seconds();
            if (RemainingSeconds <= 0.0)
            {
                OutError = TEXT("Socket receive timed out");
                return false;
            }

            if (!Socket->Wait(ESocketWaitConditions::WaitForRead, FTimespan::FromSeconds(FMath::Min(RemainingSeconds, 0.25))))
            {
                continue;
            }

            int32 Read = 0;
            if (!Socket->Recv(Dest + TotalRead, ByteCount - TotalRead, Read) || Read <= 0)
            {
                OutError = TEXT("Socket receive failed");
                return false;
            }
            TotalRead += Read;
        }

        return true;
    }

    static bool RI_RemoteReadHttpResponse(FSocket* Socket, FString& OutResponseText, FString& OutError, double TimeoutSeconds = RI_RemoteExternalDefaultTimeoutSeconds)
    {
        OutResponseText.Reset();
        OutError.Reset();
        if (!Socket)
        {
            OutError = TEXT("Socket unavailable");
            return false;
        }

        TArray<uint8> Buffer;
        const double Deadline = FPlatformTime::Seconds() + TimeoutSeconds;
        while (FPlatformTime::Seconds() < Deadline)
        {
            const double RemainingSeconds = Deadline - FPlatformTime::Seconds();
            if (RemainingSeconds <= 0.0)
            {
                break;
            }

            if (!Socket->Wait(ESocketWaitConditions::WaitForRead, FTimespan::FromSeconds(FMath::Min(RemainingSeconds, 0.025))))
            {
                continue;
            }

            uint32 Pending = 0;
            if (!Socket->HasPendingData(Pending))
            {
                Pending = 1024;
            }

            const int32 ReadSize = FMath::Max<int32>(static_cast<int32>(Pending), 256);
            TArray<uint8> Temp;
            Temp.SetNumUninitialized(ReadSize);

            int32 Read = 0;
            if (!Socket->Recv(Temp.GetData(), Temp.Num(), Read) || Read <= 0)
            {
                OutError = TEXT("Failed to read HTTP handshake response");
                return false;
            }

            Buffer.Append(Temp.GetData(), Read);

            const FString BufferText = FString(UTF8_TO_TCHAR(reinterpret_cast<const char*>(Buffer.GetData())));
            if (BufferText.Contains(TEXT("\r\n\r\n")))
            {
                OutResponseText = BufferText;
                return true;
            }
        }

        OutError = TEXT("Timed out waiting for HTTP handshake response");
        return false;
    }

    static TArray<uint8> RI_RemoteCreateClientWebSocketFrame(const FString& Message)
    {
        TArray<uint8> Frame;
        FTCHARToUTF8 Utf8(*Message);
        const int32 MessageLen = Utf8.Length();

        Frame.Add(0x81);
        if (MessageLen < 126)
        {
            Frame.Add(static_cast<uint8>(MessageLen | 0x80));
        }
        else if (MessageLen < 65536)
        {
            Frame.Add(126 | 0x80);
            Frame.Add((MessageLen >> 8) & 0xFF);
            Frame.Add(MessageLen & 0xFF);
        }
        else
        {
            Frame.Add(127 | 0x80);
            for (int32 Index = 7; Index >= 0; --Index)
            {
                Frame.Add((MessageLen >> (Index * 8)) & 0xFF);
            }
        }

        uint8 MaskKey[4];
        FGuid Guid = FGuid::NewGuid();
        FMemory::Memcpy(MaskKey, &Guid, 4);
        Frame.Append(MaskKey, UE_ARRAY_COUNT(MaskKey));

        const uint8* Payload = reinterpret_cast<const uint8*>(Utf8.Get());
        for (int32 Index = 0; Index < MessageLen; ++Index)
        {
            Frame.Add(Payload[Index] ^ MaskKey[Index % 4]);
        }

        return Frame;
    }

    static bool RI_RemoteReadWebSocketMessage(FSocket* Socket, FString& OutMessage, FString& OutError, double TimeoutSeconds = RI_RemoteExternalDefaultTimeoutSeconds)
    {
        OutMessage.Reset();
        OutError.Reset();

        uint8 Header[2] = {};
        if (!RI_RemoteReceiveExact(Socket, Header, UE_ARRAY_COUNT(Header), TimeoutSeconds, OutError))
        {
            return false;
        }

        const bool bMasked = (Header[1] & 0x80) != 0;
        uint64 PayloadLen = Header[1] & 0x7F;
        if (PayloadLen == 126)
        {
            uint8 ExtendedLen[2] = {};
            if (!RI_RemoteReceiveExact(Socket, ExtendedLen, UE_ARRAY_COUNT(ExtendedLen), TimeoutSeconds, OutError))
            {
                return false;
            }
            PayloadLen = (static_cast<uint64>(ExtendedLen[0]) << 8) | static_cast<uint64>(ExtendedLen[1]);
        }
        else if (PayloadLen == 127)
        {
            uint8 ExtendedLen[8] = {};
            if (!RI_RemoteReceiveExact(Socket, ExtendedLen, UE_ARRAY_COUNT(ExtendedLen), TimeoutSeconds, OutError))
            {
                return false;
            }
            PayloadLen = 0;
            for (uint8 Byte : ExtendedLen)
            {
                PayloadLen = (PayloadLen << 8) | static_cast<uint64>(Byte);
            }
        }

        uint8 MaskKey[4] = {};
        if (bMasked && !RI_RemoteReceiveExact(Socket, MaskKey, UE_ARRAY_COUNT(MaskKey), TimeoutSeconds, OutError))
        {
            return false;
        }

        if (PayloadLen > static_cast<uint64>(INT32_MAX))
        {
            OutError = TEXT("WebSocket payload too large");
            return false;
        }

        TArray<uint8> Payload;
        Payload.SetNumUninitialized(static_cast<int32>(PayloadLen));
        if (Payload.Num() > 0 && !RI_RemoteReceiveExact(Socket, Payload.GetData(), Payload.Num(), TimeoutSeconds, OutError))
        {
            return false;
        }

        if (bMasked)
        {
            for (int32 Index = 0; Index < Payload.Num(); ++Index)
            {
                Payload[Index] ^= MaskKey[Index % 4];
            }
        }

        if (Payload.Num() <= 0)
        {
            OutMessage.Reset();
            return true;
        }

        FUTF8ToTCHAR Utf8ToTChar(reinterpret_cast<const char*>(Payload.GetData()), Payload.Num());
        OutMessage = FString(Utf8ToTChar.Length(), Utf8ToTChar.Get());
        return true;
    }

    static bool RI_RemoteCallExternalRuntimeJsonRpc(
        const FString& Host,
        int32 Port,
        const FString& Method,
        const TSharedPtr<FJsonObject>& Params,
        double TimeoutSeconds,
        TSharedPtr<FJsonValue>& OutResultValue,
        FString& OutError)
    {
        OutResultValue.Reset();
        OutError.Reset();

        ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
        if (!SocketSubsystem)
        {
            OutError = TEXT("Socket subsystem unavailable");
            return false;
        }

        TSharedRef<FInternetAddr> Address = SocketSubsystem->CreateInternetAddr();
        bool bValidIp = false;
        Address->SetIp(*Host, bValidIp);
        Address->SetPort(Port);
        if (!bValidIp)
        {
            OutError = FString::Printf(TEXT("Invalid remote host: %s"), *Host);
            return false;
        }

        FSocket* Socket = SocketSubsystem->CreateSocket(NAME_Stream, TEXT("RuntimeInspectorRemoteSession"), false);
        if (!Socket)
        {
            OutError = TEXT("Failed to create remote session socket");
            return false;
        }

        struct FSocketGuard
        {
            ISocketSubsystem* Subsystem = nullptr;
            FSocket* Socket = nullptr;
            ~FSocketGuard()
            {
                if (Subsystem && Socket)
                {
                    Subsystem->DestroySocket(Socket);
                }
            }
        } SocketGuard { SocketSubsystem, Socket };

        Socket->SetNoDelay(true);

        if (!RI_RemoteConnectWithTimeout(SocketSubsystem, Socket, Address, TimeoutSeconds, OutError))
        {
            return false;
        }

        uint8 KeyBytes[16] = {};
        const FGuid Guid = FGuid::NewGuid();
        FMemory::Memcpy(KeyBytes, &Guid, FMath::Min<int32>(sizeof(Guid), UE_ARRAY_COUNT(KeyBytes)));
        const FString WebSocketKey = FBase64::Encode(KeyBytes, UE_ARRAY_COUNT(KeyBytes));
        const FString HttpRequest = FString::Printf(
            TEXT("GET / HTTP/1.1\r\nHost: %s:%d\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Key: %s\r\nSec-WebSocket-Version: 13\r\n\r\n"),
            *Host,
            Port,
            *WebSocketKey);

        FTCHARToUTF8 HttpRequestUtf8(*HttpRequest);
        if (!RI_RemoteSendAll(Socket, reinterpret_cast<const uint8*>(HttpRequestUtf8.Get()), HttpRequestUtf8.Length(), OutError))
        {
            return false;
        }

        FString HandshakeResponse;
        if (!RI_RemoteReadHttpResponse(Socket, HandshakeResponse, OutError, TimeoutSeconds))
        {
            return false;
        }

        if (!HandshakeResponse.Contains(TEXT("101"), ESearchCase::IgnoreCase)
            || !HandshakeResponse.Contains(TEXT("Sec-WebSocket-Accept"), ESearchCase::IgnoreCase))
        {
            OutError = FString::Printf(TEXT("Invalid websocket handshake response from %s:%d"), *Host, Port);
            return false;
        }

        TSharedPtr<FJsonObject> RequestObject = MakeShared<FJsonObject>();
        RequestObject->SetStringField(TEXT("jsonrpc"), TEXT("2.0"));
        RequestObject->SetStringField(TEXT("id"), TEXT("runtimeinspector"));
        RequestObject->SetStringField(TEXT("method"), Method);
        RequestObject->SetObjectField(TEXT("params"), Params.IsValid() ? Params : MakeShared<FJsonObject>());

        FString RequestPayload;
        {
            TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestPayload);
            if (!FJsonSerializer::Serialize(RequestObject.ToSharedRef(), Writer))
            {
                OutError = TEXT("Failed to serialize remote JSON-RPC request");
                return false;
            }
        }

        const TArray<uint8> RequestFrame = RI_RemoteCreateClientWebSocketFrame(RequestPayload);
        if (!RI_RemoteSendAll(Socket, RequestFrame.GetData(), RequestFrame.Num(), OutError))
        {
            return false;
        }

        FString ResponsePayload;
        if (!RI_RemoteReadWebSocketMessage(Socket, ResponsePayload, OutError, TimeoutSeconds))
        {
            return false;
        }

        TSharedPtr<FJsonObject> ResponseObject;
        {
            const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponsePayload);
            if (!FJsonSerializer::Deserialize(Reader, ResponseObject) || !ResponseObject.IsValid())
            {
                OutError = TEXT("Failed to parse remote JSON-RPC response");
                return false;
            }
        }

        if (const TSharedPtr<FJsonObject>* ErrorObject = nullptr; ResponseObject->TryGetObjectField(TEXT("error"), ErrorObject) && ErrorObject && ErrorObject->IsValid())
        {
            FString ErrorMessage;
            if (!(*ErrorObject)->TryGetStringField(TEXT("message"), ErrorMessage))
            {
                ErrorMessage = TEXT("Remote JSON-RPC error");
            }
            OutError = ErrorMessage;
            return false;
        }

        const TSharedPtr<FJsonValue>* ResultValue = ResponseObject->Values.Find(TEXT("result"));
        if (!ResultValue || !ResultValue->IsValid())
        {
            OutError = TEXT("Remote JSON-RPC response missing result field");
            return false;
        }

        OutResultValue = *ResultValue;
        return true;
    }

    static bool RI_RemoteSessionHasCapability(const FRIRuntimeSessionInfo& Session, const TCHAR* Capability)
    {
        return Session.CapabilityTags.ContainsByPredicate([Capability](const FString& Tag)
        {
            return Tag.Equals(Capability, ESearchCase::IgnoreCase);
        });
    }

    static const FRIRuntimeSessionInfo* RI_RemoteFindPreferredExternalPackagedSession(
        const TArray<FRIRuntimeSessionInfo>& Sessions,
        const FString& PreferredSessionId)
    {
        const FString NormalizedPreferred = PreferredSessionId.TrimStartAndEnd();
        if (!NormalizedPreferred.IsEmpty())
        {
            if (const FRIRuntimeSessionInfo* Preferred = Sessions.FindByPredicate([&NormalizedPreferred](const FRIRuntimeSessionInfo& Session)
            {
                return Session.bIsExternal
                    && Session.SessionOrigin == ERIRuntimeSessionOrigin::ExternalPackaged
                    && Session.SessionId == NormalizedPreferred;
            }))
            {
                return Preferred;
            }
        }

        return Sessions.FindByPredicate([](const FRIRuntimeSessionInfo& Session)
        {
            return Session.bIsExternal && Session.SessionOrigin == ERIRuntimeSessionOrigin::ExternalPackaged;
        });
    }

    static bool RI_RemoteResolvePreferredExternalPackagedSession(
        UInspectorWorldSubsystem* Subsystem,
        FRIRuntimeSessionInfo& OutSession,
        FString& OutError)
    {
        OutSession = FRIRuntimeSessionInfo();
        OutError.Reset();

        if (!Subsystem)
        {
            OutError = TEXT("Subsystem unavailable");
            return false;
        }

        return Subsystem->EnsurePackagedRuntimeValidationSession(OutSession, OutError);
    }

    static bool RI_RemoteCallExternalRuntimeObjectMethod(
        const FRIRuntimeSessionInfo& Session,
        const FString& Method,
        const TSharedPtr<FJsonObject>& Params,
        TSharedPtr<FJsonObject>& OutObject,
        FString& OutError)
    {
        OutObject.Reset();
        TSharedPtr<FJsonValue> ResultValue;
        if (!RI_RemoteCallExternalRuntimeJsonRpc(
                Session.Host.IsEmpty() ? RI_RemoteExternalHost : Session.Host,
                Session.Port,
                Method,
                Params,
                RI_RemoteExternalDefaultTimeoutSeconds,
                ResultValue,
                OutError))
        {
            return false;
        }

        OutObject = ResultValue.IsValid() ? ResultValue->AsObject() : nullptr;
        if (!OutObject.IsValid())
        {
            OutError = FString::Printf(TEXT("Remote method %s did not return an object"), *Method);
            return false;
        }

        return true;
    }

    static bool RI_RemoteResolvePackagedTarget(
        UInspectorWorldSubsystem* Subsystem,
        const FString& SessionId,
        FString& OutActorQuery,
        FRIRuntimeTargetInfo& OutTarget,
        FString& OutError)
    {
        OutActorQuery.Reset();
        OutTarget = FRIRuntimeTargetInfo();
        OutError.Reset();

        if (!Subsystem)
        {
            OutError = TEXT("Subsystem unavailable");
            return false;
        }

        TArray<FRIRuntimeTargetInfo> Targets;
        if (!Subsystem->ListRuntimeTargetsForSession(SessionId, Targets, OutError, TEXT("BP_TestVarsActor"), TEXT("BP_TestVarsActor"), 16))
        {
            return false;
        }

        const FRIRuntimeTargetInfo* Target = Targets.FindByPredicate([](const FRIRuntimeTargetInfo& Candidate)
        {
            return Candidate.ActorLabel.Contains(TEXT("BP_TestVarsActor"), ESearchCase::IgnoreCase)
                || Candidate.ActorName.Contains(TEXT("BP_TestVarsActor"), ESearchCase::IgnoreCase)
                || Candidate.ActorClass.Contains(TEXT("BP_TestVarsActor"), ESearchCase::IgnoreCase)
                || Candidate.ActorClassPath.Contains(TEXT("BP_TestVarsActor"), ESearchCase::IgnoreCase);
        });
        if (!Target && Targets.Num() > 0)
        {
            Target = &Targets[0];
        }

        if (!Target)
        {
            OutError = TEXT("Packaged runtime target BP_TestVarsActor not found");
            return false;
        }

        OutTarget = *Target;
        OutActorQuery = !Target->ActorLabel.IsEmpty()
            ? Target->ActorLabel
            : (!Target->ActorPath.IsEmpty() ? Target->ActorPath : Target->ActorName);
        return !OutActorQuery.IsEmpty();
    }

    static bool RI_RemoteApplyExternalPropertyText(
        const FRIRuntimeSessionInfo& Session,
        const FString& ActorQuery,
        const FString& PropertyName,
        const FString& ValueText,
        FString& OutError)
    {
        OutError.Reset();
        TSharedPtr<FJsonObject> Params = MakeShared<FJsonObject>();
        Params->SetStringField(TEXT("sessionId"), Session.SessionId);
        Params->SetStringField(TEXT("propertyName"), PropertyName);
        Params->SetStringField(TEXT("value"), ValueText);
        if (ActorQuery.Contains(TEXT("/")))
        {
            Params->SetStringField(TEXT("actorPath"), ActorQuery);
        }
        else
        {
            Params->SetStringField(TEXT("actorLabel"), ActorQuery);
        }

        TSharedPtr<FJsonObject> ResultObject;
        if (!RI_RemoteCallExternalRuntimeObjectMethod(Session, TEXT("apply_runtime_property_text"), Params, ResultObject, OutError))
        {
            return false;
        }

        bool bSuccess = false;
        ResultObject->TryGetBoolField(TEXT("success"), bSuccess);
        if (!bSuccess)
        {
            if (!ResultObject->TryGetStringField(TEXT("error"), OutError) || OutError.IsEmpty())
            {
                OutError = TEXT("Runtime property apply failed");
            }
            return false;
        }

        return true;
    }

    static bool RI_RemoteBuildRestoreBundle(const FRIPatchBundle& SourceBundle, FRIPatchBundle& OutRestoreBundle)
    {
        OutRestoreBundle = SourceBundle;
        if (OutRestoreBundle.Operations.Num() <= 0)
        {
            return false;
        }

        OutRestoreBundle.BundleId = SourceBundle.BundleId.IsEmpty()
            ? TEXT("RemoteRestoreBundle")
            : SourceBundle.BundleId + TEXT("_restore");
        OutRestoreBundle.DisplayName = SourceBundle.DisplayName.IsEmpty()
            ? TEXT("Remote Restore Bundle")
            : SourceBundle.DisplayName + TEXT(" Restore");

        for (FRIPatchOperation& Operation : OutRestoreBundle.Operations)
        {
            const FString OriginalPatchedValue = Operation.PatchedValue;
            Operation.PatchedValue = Operation.BaselineValue;
            Operation.BaselineValue = OriginalPatchedValue;
        }

        return true;
    }

    static bool RI_RemoteLoadPackagedSelfTestBlueprint(UBlueprint*& OutBlueprint)
    {
        OutBlueprint = LoadObject<UBlueprint>(nullptr, TEXT("/RuntimeInspector/Test/BP_TestVarsActor.BP_TestVarsActor"));
        if (!OutBlueprint)
        {
            OutBlueprint = LoadObject<UBlueprint>(nullptr, TEXT("/RuntimeInspector/BP_TestVarsActor.BP_TestVarsActor"));
        }
        return OutBlueprint != nullptr;
    }

    static void RI_RemoteInjectBlueprintSourceTag(FRIPatchBundle& InOutBundle, const FString& BlueprintPath)
    {
        if (BlueprintPath.IsEmpty())
        {
            return;
        }

        for (FRIPatchOperation& Operation : InOutBundle.Operations)
        {
            if (Operation.Field.FieldKind == ERIPatchFieldKind::Property
                && !Operation.SourceTag.StartsWith(TEXT("Blueprint:"), ESearchCase::IgnoreCase)
                && !Operation.SourceTag.StartsWith(TEXT("Config:"), ESearchCase::IgnoreCase)
                && !Operation.SourceTag.StartsWith(TEXT("Material:"), ESearchCase::IgnoreCase)
                && !Operation.SourceTag.StartsWith(TEXT("DataAsset:"), ESearchCase::IgnoreCase)
                && !Operation.SourceTag.StartsWith(TEXT("DataTable:"), ESearchCase::IgnoreCase))
            {
                Operation.SourceTag = FString::Printf(TEXT("Blueprint:%s"), *BlueprintPath);
            }
        }
    }

}

bool UInspectorWorldSubsystem::TryResolvePackagedRuntimeValidationSession(FRIRuntimeSessionInfo& OutSession, FString& OutError, bool bForceRefresh) const
{
    OutSession = FRIRuntimeSessionInfo();
    OutError.Reset();

#if !RUNTIME_INSPECTOR_ENABLED
    OutError = TEXT("RuntimeInspector disabled");
    return false;
#elif !WITH_EDITOR
    OutError = TEXT("Editor authority required");
    return false;
#else
    const FString PreferredSessionId = !PreferredRemoteSessionId.IsEmpty() ? PreferredRemoteSessionId : ConnectedRuntimeSessionId;
    const TArray<FRIRuntimeSessionInfo> Sessions = QueryAvailableRuntimeSessions(bForceRefresh, true);
    const FRIRuntimeSessionInfo* Preferred = RI_RemoteFindPreferredExternalPackagedSession(Sessions, PreferredSessionId);
    if (!Preferred)
    {
        OutError = TEXT("No external packaged runtime session discovered on loopback");
        return false;
    }

    OutSession = *Preferred;
    return true;
#endif
}

bool UInspectorWorldSubsystem::EnsurePackagedRuntimeValidationSession(FRIRuntimeSessionInfo& OutSession, FString& OutError)
{
    OutSession = FRIRuntimeSessionInfo();
    OutError.Reset();

#if !RUNTIME_INSPECTOR_ENABLED
    OutError = TEXT("RuntimeInspector disabled");
    return false;
#elif !WITH_EDITOR
    OutError = TEXT("Editor authority required");
    return false;
#else
    if (TryResolvePackagedRuntimeValidationSession(OutSession, OutError, true))
    {
        return true;
    }

    const FString ScriptPath = RI_RemotePackagedValidationScriptPath(TEXT("RunPackagedRuntimeValidation.cmd"));
    FString ScriptStdOut;
    FString ScriptStdErr;
    int32 ScriptExitCode = -1;
    if (!RI_RemoteExecuteWindowsScript(ScriptPath, ScriptStdOut, ScriptStdErr, ScriptExitCode))
    {
        OutError = FString::Printf(
            TEXT("PackagedLaunch=script-exec-failed | Script=%s | Error=%s"),
            *ScriptPath,
            *RI_RemotePackagedValidationLogSuffix(ScriptStdErr));
        return false;
    }

    if (ScriptExitCode != 0)
    {
        OutError = FString::Printf(
            TEXT("PackagedLaunch=run-script-failed | Exit=%d | StdOut=%s | StdErr=%s"),
            ScriptExitCode,
            *RI_RemotePackagedValidationLogSuffix(ScriptStdOut),
            *RI_RemotePackagedValidationLogSuffix(ScriptStdErr));
        return false;
    }

    const double Deadline = FPlatformTime::Seconds() + RI_RemotePackagedValidationEnsureTimeoutSeconds;
    FString ResolveError;
    while (FPlatformTime::Seconds() <= Deadline)
    {
        if (TryResolvePackagedRuntimeValidationSession(OutSession, ResolveError, true))
        {
            OutError.Reset();
            return true;
        }

        FPlatformProcess::Sleep(0.5f);
    }

    FString StateJson;
    const FString StatePath = RI_RemotePackagedValidationStatePath();
    if (FPaths::FileExists(StatePath))
    {
        FFileHelper::LoadFileToString(StateJson, *StatePath);
    }

    OutError = FString::Printf(
        TEXT("PackagedLaunch=discovery-failed | Resolve=%s | State=%s"),
        *ResolveError,
        *RI_RemotePackagedValidationLogSuffix(StateJson));
    return false;
#endif
}

bool UInspectorWorldSubsystem::StopPackagedRuntimeValidationSession(FString& OutError)
{
    OutError.Reset();

#if !RUNTIME_INSPECTOR_ENABLED
    OutError = TEXT("RuntimeInspector disabled");
    return false;
#elif !WITH_EDITOR
    OutError = TEXT("Editor authority required");
    return false;
#else
    const FString ScriptPath = RI_RemotePackagedValidationScriptPath(TEXT("StopPackagedRuntimeValidation.cmd"));
    FString ScriptStdOut;
    FString ScriptStdErr;
    int32 ScriptExitCode = -1;
    if (!RI_RemoteExecuteWindowsScript(ScriptPath, ScriptStdOut, ScriptStdErr, ScriptExitCode))
    {
        OutError = FString::Printf(TEXT("PackagedStop=script-exec-failed | %s"), *RI_RemotePackagedValidationLogSuffix(ScriptStdErr));
        return false;
    }

    if (ScriptExitCode != 0)
    {
        OutError = FString::Printf(
            TEXT("PackagedStop=script-failed | Exit=%d | StdOut=%s | StdErr=%s"),
            ScriptExitCode,
            *RI_RemotePackagedValidationLogSuffix(ScriptStdOut),
            *RI_RemotePackagedValidationLogSuffix(ScriptStdErr));
        return false;
    }

    return true;
#endif
}

TArray<FRIRuntimeSessionInfo> UInspectorWorldSubsystem::GetAvailableRuntimeSessions() const
{
    return QueryAvailableRuntimeSessions(false, true);
}

TArray<FRIRuntimeSessionInfo> UInspectorWorldSubsystem::QueryAvailableRuntimeSessions(bool bForceRefresh, bool bAllowExternalProbe) const
{
    const double StartSeconds = FPlatformTime::Seconds();
    TArray<FRIRuntimeSessionInfo> Results;
#if RUNTIME_INSPECTOR_ENABLED
    UInspectorWorldSubsystem* MutableThis = const_cast<UInspectorWorldSubsystem*>(this);
    TSet<FString> AddedSessionIds;
    auto AddSession = [this, &Results, &AddedSessionIds](UWorld* World, const FString& SessionId, const FString& DisplayName, const FString& SessionType)
    {
        if (!World || AddedSessionIds.Contains(SessionId))
        {
            return;
        }

        const FRIRuntimeSessionSummary SessionSummary = RI_RemoteBuildSessionSummaryForWorld(World);
        if (!SessionSummary.bSessionAvailable)
        {
            return;
        }

        FRIRuntimeSessionInfo Session;
        Session.SessionId = SessionId;
        Session.DisplayName = DisplayName;
        Session.SessionType = SessionType;
        Session.WorldPath = World->GetPathName();
        Session.MapName = SessionSummary.MapName;
        Session.WorldTypeLabel = SessionSummary.WorldTypeLabel;
        Session.NetModeLabel = SessionSummary.NetModeLabel;
        Session.bSessionAvailable = SessionSummary.bSessionAvailable;
        Session.bRequiresExplicitConnect = true;
        Session.bRuntimeEnabled = IsRIEnabled();
        Session.bUnlockRequired = IsUnlockRequired();
        Session.bUnlocked = !Session.bUnlockRequired || IsRIUnlocked();
        Session.bSupportsTargetListing = true;
        Session.bSupportsPatchApply = true;
        Session.bSupportsVerification = true;

        if (!ConnectedRuntimeSessionId.IsEmpty() && ConnectedRuntimeSessionId == Session.SessionId)
        {
            Session.ConnectionState = ERIRuntimeSessionConnectionState::Connected;
        }
        else if (!LastRemoteRuntimeSessionError.IsEmpty())
        {
            Session.ConnectionState = ERIRuntimeSessionConnectionState::Error;
            Session.LastError = LastRemoteRuntimeSessionError;
        }
        else
        {
            Session.ConnectionState = ERIRuntimeSessionConnectionState::Disconnected;
        }

        Session.Summary = RI_RemoteBuildSessionSummaryText(Session);
        Results.Add(MoveTemp(Session));
        AddedSessionIds.Add(SessionId);
    };

    if (UWorld* PIEWorld = RI_RemoteGetPIEWorld())
    {
        AddSession(PIEWorld, TEXT("local_pie_current"), TEXT("Local PIE Current"), TEXT("LocalPIE"));
        AddSession(RI_RemoteGetEditorWorld(), TEXT("local_editor_current"), TEXT("Local Editor Current"), TEXT("LocalEditor"));
    }
    else if (UWorld* EditorWorld = RI_RemoteGetEditorWorld())
    {
        AddSession(EditorWorld, TEXT("local_runtime_current"), TEXT("Local Runtime Current"), TEXT("LocalRuntime"));
    }
    else
    {
        const UWorld* CurrentWorld = GetWorld();
        const FRIRuntimeSessionSummary SessionSummary = GetRuntimeSessionSummary();
        AddSession(const_cast<UWorld*>(CurrentWorld), RI_RemoteBuildLocalSessionId(SessionSummary), RI_RemoteBuildLocalSessionDisplayName(SessionSummary), SessionSummary.bIsPIEWorld ? TEXT("LocalPIE") : TEXT("LocalRuntime"));
    }

#if WITH_EDITOR
    if (bForceRefresh)
    {
        MutableThis->CachedExternalRuntimeProbeTimestampSeconds = -1.0;
    }

    const double NowSeconds = FPlatformTime::Seconds();
    const bool bProbeCacheExpired = MutableThis->CachedExternalRuntimeProbeTimestampSeconds < 0.0
        || (NowSeconds - MutableThis->CachedExternalRuntimeProbeTimestampSeconds) > RI_RemoteExternalProbeCacheSeconds;
    if (bAllowExternalProbe && bProbeCacheExpired)
    {
        MutableThis->CachedExternalRuntimeSessions.Reset();
        MutableThis->CachedExternalRuntimeProbeError.Reset();

        for (int32 Port = RI_RemoteExternalMinPort; Port <= RI_RemoteExternalMaxPort; ++Port)
        {
            TSharedPtr<FJsonValue> CapabilityValue;
            FString ProbeError;
            if (!RI_RemoteCallExternalRuntimeJsonRpc(
                    RI_RemoteExternalHost,
                    Port,
                    TEXT("get_runtime_session_capabilities"),
                    MakeShared<FJsonObject>(),
                    RI_RemoteExternalProbeTimeoutSeconds,
                    CapabilityValue,
                    ProbeError))
            {
                continue;
            }

            const TSharedPtr<FJsonObject> CapabilityObject = CapabilityValue.IsValid() ? CapabilityValue->AsObject() : nullptr;
            if (!CapabilityObject.IsValid())
            {
                continue;
            }

            FRIRuntimeSessionInfo ExternalSession;
            const TSharedPtr<FJsonObject>* SessionObject = nullptr;
            if (!CapabilityObject->TryGetObjectField(TEXT("session"), SessionObject)
                || !SessionObject
                || !SessionObject->IsValid()
                || !RI_RemoteJsonObjectToStruct<FRIRuntimeSessionInfo>(*SessionObject, ExternalSession))
            {
                continue;
            }

            ExternalSession.SessionId = ExternalSession.SessionId.IsEmpty()
                ? FString::Printf(TEXT("external_packaged_%d"), Port)
                : ExternalSession.SessionId;
            ExternalSession.DisplayName = ExternalSession.DisplayName.IsEmpty()
                ? FString::Printf(TEXT("External Packaged Runtime (%d)"), Port)
                : ExternalSession.DisplayName;
            ExternalSession.SessionType = ExternalSession.SessionType.IsEmpty() ? TEXT("ExternalPackaged") : ExternalSession.SessionType;
            ExternalSession.Host = ExternalSession.Host.IsEmpty() ? RI_RemoteExternalHost : ExternalSession.Host;
            ExternalSession.Port = ExternalSession.Port > 0 ? ExternalSession.Port : Port;
            ExternalSession.SessionOrigin = ERIRuntimeSessionOrigin::ExternalPackaged;
            ExternalSession.bIsExternal = true;
            ExternalSession.bLoopbackOnly = true;
            ExternalSession.bSessionAvailable = true;
            ExternalSession.bRequiresExplicitConnect = true;
            ExternalSession.ConnectionState = ERIRuntimeSessionConnectionState::Disconnected;

            if (ExternalSession.ProtocolVersion != RI_RemoteExpectedExternalProtocolVersion)
            {
                ExternalSession.ConnectionState = ERIRuntimeSessionConnectionState::Error;
                ExternalSession.LastError = FString::Printf(
                    TEXT("Protocol mismatch | Expected=%d Actual=%d"),
                    RI_RemoteExpectedExternalProtocolVersion,
                    ExternalSession.ProtocolVersion);
            }
            else if (!ConnectedRuntimeSessionId.IsEmpty() && ConnectedRuntimeSessionId == ExternalSession.SessionId)
            {
                ExternalSession.ConnectionState = ERIRuntimeSessionConnectionState::Connected;
            }

            ExternalSession.Summary = RI_RemoteBuildSessionSummaryText(ExternalSession);
            MutableThis->CachedExternalRuntimeSessions.Add(MoveTemp(ExternalSession));
        }

        MutableThis->CachedExternalRuntimeProbeTimestampSeconds = NowSeconds;
    }

    for (const FRIRuntimeSessionInfo& ExternalSession : MutableThis->CachedExternalRuntimeSessions)
    {
        if (AddedSessionIds.Contains(ExternalSession.SessionId))
        {
            continue;
        }

        FRIRuntimeSessionInfo Session = ExternalSession;
        if (!ConnectedRuntimeSessionId.IsEmpty() && ConnectedRuntimeSessionId == Session.SessionId)
        {
            Session.ConnectionState = ERIRuntimeSessionConnectionState::Connected;
            Session.LastError.Reset();
        }
        Session.Summary = RI_RemoteBuildSessionSummaryText(Session);
        Results.Add(MoveTemp(Session));
        AddedSessionIds.Add(Results.Last().SessionId);
    }
#endif
#endif
    UE_LOG(
        LogRuntimeInspector,
        Log,
        TEXT("[RI][Perf] GetAvailableRuntimeSessions %.2f ms | Force=%d AllowProbe=%d Count=%d"),
        (FPlatformTime::Seconds() - StartSeconds) * 1000.0,
        bForceRefresh ? 1 : 0,
        bAllowExternalProbe ? 1 : 0,
        Results.Num());
    return Results;
}

FString UInspectorWorldSubsystem::MakeRuntimeTargetQueryCacheKey(const FString& SessionId, const FString& NameFilter, const FString& ClassFilter, int32 Limit) const
{
    return FString::Printf(
        TEXT("%s|%s|%s|%d"),
        *SessionId.TrimStartAndEnd(),
        *NameFilter.TrimStartAndEnd(),
        *ClassFilter.TrimStartAndEnd(),
        FMath::Clamp(Limit, 1, 500));
}

void UInspectorWorldSubsystem::InvalidateRuntimeTargetQueryCache(const FString& SessionId)
{
    if (SessionId.TrimStartAndEnd().IsEmpty())
    {
        CachedRuntimeTargetQueryResults.Reset();
        return;
    }

    const FString NormalizedSessionId = SessionId.TrimStartAndEnd();
    for (auto It = CachedRuntimeTargetQueryResults.CreateIterator(); It; ++It)
    {
        if (It.Value().SessionId.Equals(NormalizedSessionId, ESearchCase::CaseSensitive))
        {
            It.RemoveCurrent();
        }
    }
}

void UInspectorWorldSubsystem::CacheRuntimeTargetQueryResult(
    const FString& SessionId,
    const FString& NameFilter,
    const FString& ClassFilter,
    int32 Limit,
    const TArray<FRIRuntimeTargetInfo>& Targets,
    const FString& Error,
    bool bSuccess) const
{
    FRIRuntimeTargetQueryCacheEntry Entry;
    Entry.SessionId = SessionId.TrimStartAndEnd();
    Entry.NameFilter = NameFilter.TrimStartAndEnd();
    Entry.ClassFilter = ClassFilter.TrimStartAndEnd();
    Entry.Limit = FMath::Clamp(Limit, 1, 500);
    Entry.Targets = Targets;
    Entry.Error = Error;
    Entry.bSuccess = bSuccess;
    Entry.CachedAtSeconds = FPlatformTime::Seconds();
    CachedRuntimeTargetQueryResults.FindOrAdd(MakeRuntimeTargetQueryCacheKey(SessionId, NameFilter, ClassFilter, Limit)) = MoveTemp(Entry);
}

bool UInspectorWorldSubsystem::TryGetCachedRuntimeTargetQueryResult(
    const FString& SessionId,
    const FString& NameFilter,
    const FString& ClassFilter,
    int32 Limit,
    TArray<FRIRuntimeTargetInfo>& OutTargets,
    FString& OutError,
    bool& bOutSuccess) const
{
    OutTargets.Reset();
    OutError.Reset();
    bOutSuccess = false;

    const FString CacheKey = MakeRuntimeTargetQueryCacheKey(SessionId, NameFilter, ClassFilter, Limit);
    const FRIRuntimeTargetQueryCacheEntry* Entry = CachedRuntimeTargetQueryResults.Find(CacheKey);
    if (!Entry)
    {
        return false;
    }

    if ((FPlatformTime::Seconds() - Entry->CachedAtSeconds) > RI_RemoteTargetQueryCacheSeconds)
    {
        CachedRuntimeTargetQueryResults.Remove(CacheKey);
        return false;
    }

    OutTargets = Entry->Targets;
    OutError = Entry->Error;
    bOutSuccess = Entry->bSuccess;
    return true;
}

bool UInspectorWorldSubsystem::RefreshRuntimeTargetQueryResult(
    const FString& SessionId,
    const FString& NameFilter,
    const FString& ClassFilter,
    int32 Limit,
    TArray<FRIRuntimeTargetInfo>& OutTargets,
    FString& OutError) const
{
    return ListRuntimeTargetsForSession(SessionId, OutTargets, OutError, NameFilter, ClassFilter, Limit);
}

bool UInspectorWorldSubsystem::ConnectRemoteRuntimeSession(const FString& SessionId, FRIRuntimeSessionInfo& OutSession, FString& OutError)
{
    OutSession = FRIRuntimeSessionInfo();
    OutError.Reset();

#if !RUNTIME_INSPECTOR_ENABLED
    OutError = TEXT("RuntimeInspector disabled");
    LastRemoteRuntimeSessionError = OutError;
    return false;
#else
    const FString NormalizedSessionId = SessionId.TrimStartAndEnd();
    if (NormalizedSessionId.IsEmpty())
    {
        OutError = TEXT("Missing session id");
        LastRemoteRuntimeSessionError = OutError;
        return false;
    }

    const TArray<FRIRuntimeSessionInfo> Sessions = QueryAvailableRuntimeSessions(false, true);
    const FRIRuntimeSessionInfo* Session = Sessions.FindByPredicate([&NormalizedSessionId](const FRIRuntimeSessionInfo& Candidate)
    {
        return Candidate.SessionId == NormalizedSessionId;
    });

    if (!Session)
    {
        OutError = FString::Printf(TEXT("Unknown runtime session: %s"), *NormalizedSessionId);
        LastRemoteRuntimeSessionError = OutError;
        return false;
    }

    if (!Session->bSessionAvailable)
    {
        OutError = TEXT("Runtime session unavailable");
        LastRemoteRuntimeSessionError = OutError;
        OutSession = *Session;
        OutSession.ConnectionState = ERIRuntimeSessionConnectionState::Error;
        OutSession.LastError = OutError;
        OutSession.Summary = RI_RemoteBuildSessionSummaryText(OutSession);
        return false;
    }

    if (Session->bIsExternal || RI_IsExternalPackagedSessionId(NormalizedSessionId))
    {
#if WITH_EDITOR
        if (Session->ProtocolVersion != RI_RemoteExpectedExternalProtocolVersion)
        {
            OutError = Session->LastError.IsEmpty()
                ? FString::Printf(TEXT("Protocol mismatch | Expected=%d Actual=%d"), RI_RemoteExpectedExternalProtocolVersion, Session->ProtocolVersion)
                : Session->LastError;
            LastRemoteRuntimeSessionError = OutError;
            OutSession = *Session;
            OutSession.ConnectionState = ERIRuntimeSessionConnectionState::Error;
            OutSession.LastError = OutError;
            OutSession.Summary = RI_RemoteBuildSessionSummaryText(OutSession);
            return false;
        }

        int32 Port = Session->Port;
        if (Port <= 0 && !RI_TryParseExternalPackagedPort(NormalizedSessionId, Port))
        {
            OutError = FString::Printf(TEXT("Unable to resolve port for runtime session: %s"), *NormalizedSessionId);
            LastRemoteRuntimeSessionError = OutError;
            return false;
        }

        TSharedPtr<FJsonValue> ResultValue;
        if (!RI_RemoteCallExternalRuntimeJsonRpc(
                Session->Host.IsEmpty() ? RI_RemoteExternalHost : Session->Host,
                Port,
                TEXT("connect_runtime_session"),
                MakeShared<FJsonObject>(),
                RI_RemoteExternalDefaultTimeoutSeconds,
                ResultValue,
                OutError))
        {
            LastRemoteRuntimeSessionError = OutError;
            OutSession = *Session;
            OutSession.ConnectionState = ERIRuntimeSessionConnectionState::Error;
            OutSession.LastError = OutError;
            OutSession.Summary = RI_RemoteBuildSessionSummaryText(OutSession);
            return false;
        }

        const TSharedPtr<FJsonObject> ResultObject = ResultValue.IsValid() ? ResultValue->AsObject() : nullptr;
        const TSharedPtr<FJsonObject>* SessionObject = nullptr;
        if (!ResultObject.IsValid()
            || !ResultObject->GetBoolField(TEXT("success"))
            || !ResultObject->TryGetObjectField(TEXT("session"), SessionObject)
            || !SessionObject
            || !SessionObject->IsValid()
            || !RI_RemoteJsonObjectToStruct<FRIRuntimeSessionInfo>(*SessionObject, OutSession))
        {
            if (OutError.IsEmpty() && ResultObject.IsValid())
            {
                ResultObject->TryGetStringField(TEXT("error"), OutError);
            }
            if (OutError.IsEmpty())
            {
                OutError = TEXT("Failed to parse external runtime session response");
            }
            LastRemoteRuntimeSessionError = OutError;
            return false;
        }

        OutSession.SessionOrigin = ERIRuntimeSessionOrigin::ExternalPackaged;
        OutSession.bIsExternal = true;
        OutSession.bLoopbackOnly = true;
        OutSession.ConnectionState = ERIRuntimeSessionConnectionState::Connected;
        OutSession.LastError.Reset();
        ConnectedRuntimeSessionId = OutSession.SessionId;
        PreferredRemoteSessionId = OutSession.SessionId;
        LastRemoteSessionSelectionSummary = OutSession.DisplayName.IsEmpty() ? OutSession.SessionId : OutSession.DisplayName;
        LastRemoteRuntimeSessionError.Reset();
        CachedExternalRuntimeProbeTimestampSeconds = -1.0;
        InvalidateRuntimeTargetQueryCache(OutSession.SessionId);
        OutSession.Summary = RI_RemoteBuildSessionSummaryText(OutSession);
        return true;
#else
        OutError = TEXT("External packaged runtime sessions are editor-only");
        LastRemoteRuntimeSessionError = OutError;
        return false;
#endif
    }

    ConnectedRuntimeSessionId = Session->SessionId;
    PreferredRemoteSessionId = Session->SessionId;
    LastRemoteSessionSelectionSummary = Session->DisplayName.IsEmpty() ? Session->SessionId : Session->DisplayName;
    LastRemoteRuntimeSessionError.Reset();
    InvalidateRuntimeTargetQueryCache(Session->SessionId);
    OutSession = *Session;
    OutSession.ConnectionState = ERIRuntimeSessionConnectionState::Connected;
    OutSession.LastError.Reset();
    OutSession.Summary = RI_RemoteBuildSessionSummaryText(OutSession);
    return true;
#endif
}

bool UInspectorWorldSubsystem::ListRuntimeTargetsForSession(const FString& SessionId, TArray<FRIRuntimeTargetInfo>& OutTargets, FString& OutError, const FString& NameFilter, const FString& ClassFilter, int32 Limit) const
{
    const double StartSeconds = FPlatformTime::Seconds();
    OutTargets.Reset();
    OutError.Reset();

#if !RUNTIME_INSPECTOR_ENABLED
    OutError = TEXT("RuntimeInspector disabled");
    return false;
#else
    const FString NormalizedSessionId = SessionId.TrimStartAndEnd();
    if (NormalizedSessionId.IsEmpty())
    {
        OutError = TEXT("Missing session id");
        CacheRuntimeTargetQueryResult(NormalizedSessionId, NameFilter, ClassFilter, Limit, OutTargets, OutError, false);
        return false;
    }

    const TArray<FRIRuntimeSessionInfo> Sessions = QueryAvailableRuntimeSessions(false, true);
    const FRIRuntimeSessionInfo* Session = Sessions.FindByPredicate([&NormalizedSessionId](const FRIRuntimeSessionInfo& Candidate)
    {
        return Candidate.SessionId == NormalizedSessionId;
    });

    if (!Session)
    {
        OutError = FString::Printf(TEXT("Unknown runtime session: %s"), *NormalizedSessionId);
        CacheRuntimeTargetQueryResult(NormalizedSessionId, NameFilter, ClassFilter, Limit, OutTargets, OutError, false);
        return false;
    }

    if (ConnectedRuntimeSessionId != NormalizedSessionId)
    {
        OutError = FString::Printf(TEXT("Runtime session '%s' is not connected"), *NormalizedSessionId);
        CacheRuntimeTargetQueryResult(NormalizedSessionId, NameFilter, ClassFilter, Limit, OutTargets, OutError, false);
        return false;
    }

    if (!Session->bRuntimeEnabled)
    {
        OutError = GetRIDisabledReason().IsEmpty() ? TEXT("RuntimeInspector disabled") : GetRIDisabledReason();
        CacheRuntimeTargetQueryResult(NormalizedSessionId, NameFilter, ClassFilter, Limit, OutTargets, OutError, false);
        return false;
    }

    if (Session->bUnlockRequired && !Session->bUnlocked)
    {
        OutError = TEXT("RuntimeInspector locked");
        CacheRuntimeTargetQueryResult(NormalizedSessionId, NameFilter, ClassFilter, Limit, OutTargets, OutError, false);
        return false;
    }

    if (Session->bIsExternal || RI_IsExternalPackagedSessionId(NormalizedSessionId))
    {
#if WITH_EDITOR
        int32 Port = Session->Port;
        if (Port <= 0 && !RI_TryParseExternalPackagedPort(NormalizedSessionId, Port))
        {
            OutError = FString::Printf(TEXT("Unable to resolve port for runtime session: %s"), *NormalizedSessionId);
            CacheRuntimeTargetQueryResult(NormalizedSessionId, NameFilter, ClassFilter, Limit, OutTargets, OutError, false);
            return false;
        }

        TSharedPtr<FJsonObject> Params = MakeShared<FJsonObject>();
        Params->SetStringField(TEXT("sessionId"), NormalizedSessionId);
        if (!NameFilter.TrimStartAndEnd().IsEmpty())
        {
            Params->SetStringField(TEXT("nameFilter"), NameFilter.TrimStartAndEnd());
        }
        if (!ClassFilter.TrimStartAndEnd().IsEmpty())
        {
            Params->SetStringField(TEXT("classFilter"), ClassFilter.TrimStartAndEnd());
        }
        Params->SetNumberField(TEXT("limit"), FMath::Clamp(Limit, 1, 500));

        TSharedPtr<FJsonValue> ResultValue;
        if (!RI_RemoteCallExternalRuntimeJsonRpc(
                Session->Host.IsEmpty() ? RI_RemoteExternalHost : Session->Host,
                Port,
                TEXT("list_runtime_targets_for_session"),
                Params,
                RI_RemoteExternalDefaultTimeoutSeconds,
                ResultValue,
                OutError))
        {
            CacheRuntimeTargetQueryResult(NormalizedSessionId, NameFilter, ClassFilter, Limit, OutTargets, OutError, false);
            return false;
        }

        const TSharedPtr<FJsonObject> ResultObject = ResultValue.IsValid() ? ResultValue->AsObject() : nullptr;
        const TArray<TSharedPtr<FJsonValue>>* TargetValues = nullptr;
        if (!ResultObject.IsValid()
            || !ResultObject->TryGetArrayField(TEXT("targets"), TargetValues)
            || !TargetValues)
        {
            OutError = TEXT("External runtime target list response missing targets");
            CacheRuntimeTargetQueryResult(NormalizedSessionId, NameFilter, ClassFilter, Limit, OutTargets, OutError, false);
            return false;
        }

        for (const TSharedPtr<FJsonValue>& Value : *TargetValues)
        {
            FRIRuntimeTargetInfo Target;
            if (!Value.IsValid())
            {
                continue;
            }

            const TSharedPtr<FJsonObject> TargetObject = Value->AsObject();
            if (!RI_RemoteJsonObjectToStruct<FRIRuntimeTargetInfo>(TargetObject, Target))
            {
                continue;
            }
            OutTargets.Add(MoveTemp(Target));
        }

        Algo::SortBy(OutTargets, &FRIRuntimeTargetInfo::ActorPath, TLess<FString>());
        const bool bSuccess = OutTargets.Num() > 0 || NameFilter.TrimStartAndEnd().IsEmpty();
        CacheRuntimeTargetQueryResult(NormalizedSessionId, NameFilter, ClassFilter, Limit, OutTargets, OutError, bSuccess);
        UE_LOG(LogRuntimeInspector, Log, TEXT("[RI][Perf] ListRuntimeTargetsForSession %.2f ms | Session=%s Count=%d External=1"), (FPlatformTime::Seconds() - StartSeconds) * 1000.0, *NormalizedSessionId, OutTargets.Num());
        return bSuccess;
#else
        OutError = TEXT("External packaged runtime target listing is editor-only");
        CacheRuntimeTargetQueryResult(NormalizedSessionId, NameFilter, ClassFilter, Limit, OutTargets, OutError, false);
        return false;
#endif
    }

    UWorld* World = RI_RemoteResolveWorldForSessionId(this, NormalizedSessionId);
    if (!World)
    {
        OutError = TEXT("World unavailable");
        CacheRuntimeTargetQueryResult(NormalizedSessionId, NameFilter, ClassFilter, Limit, OutTargets, OutError, false);
        return false;
    }

    const FString NormalizedNameFilter = NameFilter.TrimStartAndEnd();
    const FString NormalizedClassFilter = ClassFilter.TrimStartAndEnd();
    const int32 ClampedLimit = FMath::Clamp(Limit, 1, 500);
    const FString SelectedActorPath = SelectedActor.IsValid() ? SelectedActor->GetPathName() : FString();

    for (TActorIterator<AActor> ActorIt(World); ActorIt; ++ActorIt)
    {
        AActor* Actor = *ActorIt;
        if (!Actor)
        {
            continue;
        }

        FRIRuntimeTargetInfo Target;
        const FRIRuntimeActorRoleSummary RoleSummary = BuildActorRoleSummary(Actor);
        Target.SessionId = NormalizedSessionId;
        Target.ActorName = Actor->GetName();
        Target.ActorLabel = RI_RemoteActorLabel(Actor);
        Target.ActorPath = Actor->GetPathName();
        Target.ActorClass = Actor->GetClass() ? Actor->GetClass()->GetName() : FString();
        Target.ActorClassPath = Actor->GetClass() ? Actor->GetClass()->GetPathName() : FString();
        Target.bSelected = !SelectedActorPath.IsEmpty() && SelectedActorPath == Target.ActorPath;
        Target.bHasAuthority = Actor->HasAuthority();
        Target.bReplicates = Actor->GetIsReplicated();
        Target.bReplicateMovement = Actor->IsReplicatingMovement();
        Target.LocalRoleLabel = RoleSummary.LocalRoleLabel;
        Target.RemoteRoleLabel = RoleSummary.RemoteRoleLabel;
        Target.OwnerPath = Actor->GetOwner() ? Actor->GetOwner()->GetPathName() : FString();

        if (!NormalizedNameFilter.IsEmpty()
            && !Target.ActorName.Contains(NormalizedNameFilter, ESearchCase::IgnoreCase)
            && !Target.ActorLabel.Contains(NormalizedNameFilter, ESearchCase::IgnoreCase)
            && !Target.ActorPath.Contains(NormalizedNameFilter, ESearchCase::IgnoreCase))
        {
            continue;
        }

        if (!NormalizedClassFilter.IsEmpty()
            && !Target.ActorClass.Contains(NormalizedClassFilter, ESearchCase::IgnoreCase)
            && !Target.ActorClassPath.Contains(NormalizedClassFilter, ESearchCase::IgnoreCase))
        {
            continue;
        }

        Target.Summary = RI_RemoteBuildTargetSummaryText(Target);
        OutTargets.Add(MoveTemp(Target));
    }

    Algo::SortBy(OutTargets, &FRIRuntimeTargetInfo::ActorPath, TLess<FString>());
    if (OutTargets.Num() > ClampedLimit)
    {
        OutTargets.SetNum(ClampedLimit, EAllowShrinking::No);
    }

    CacheRuntimeTargetQueryResult(NormalizedSessionId, NameFilter, ClassFilter, Limit, OutTargets, OutError, true);
    UE_LOG(LogRuntimeInspector, Log, TEXT("[RI][Perf] ListRuntimeTargetsForSession %.2f ms | Session=%s Count=%d External=0"), (FPlatformTime::Seconds() - StartSeconds) * 1000.0, *NormalizedSessionId, OutTargets.Num());
    return true;
#endif
}

bool UInspectorWorldSubsystem::PullPatchBundleFromRuntimeSession(const FString& SessionId, const FString& ActorQuery, FRIPatchBundle& OutBundle, FString& OutError)
{
    OutBundle = FRIPatchBundle();
    OutError.Reset();

#if !RUNTIME_INSPECTOR_ENABLED
    OutError = TEXT("RuntimeInspector disabled");
    return false;
#else
    const FString NormalizedSessionId = SessionId.TrimStartAndEnd();
    const FString NormalizedActorQuery = ActorQuery.TrimStartAndEnd();
    if (NormalizedSessionId.IsEmpty())
    {
        OutError = TEXT("Missing session id");
        return false;
    }
    if (NormalizedActorQuery.IsEmpty())
    {
        OutError = TEXT("Missing actor query");
        return false;
    }

    FRIRuntimeSessionInfo Session;
    if (!ConnectRemoteRuntimeSession(NormalizedSessionId, Session, OutError))
    {
        return false;
    }

    PreferredRemoteSessionId = Session.SessionId;
    LastRemoteSessionSelectionSummary = Session.DisplayName.IsEmpty() ? Session.SessionId : Session.DisplayName;
    LastRemoteSessionTargetQuery = NormalizedActorQuery;

    if (Session.bIsExternal || RI_IsExternalPackagedSessionId(NormalizedSessionId))
    {
#if WITH_EDITOR
        int32 Port = Session.Port;
        if (Port <= 0 && !RI_TryParseExternalPackagedPort(NormalizedSessionId, Port))
        {
            OutError = FString::Printf(TEXT("Unable to resolve port for runtime session: %s"), *NormalizedSessionId);
            return false;
        }

        TSharedPtr<FJsonObject> Params = MakeShared<FJsonObject>();
        Params->SetStringField(TEXT("sessionId"), NormalizedSessionId);
        Params->SetStringField(TEXT("actorQuery"), NormalizedActorQuery);

        TSharedPtr<FJsonValue> ResultValue;
        if (!RI_RemoteCallExternalRuntimeJsonRpc(
                Session.Host.IsEmpty() ? RI_RemoteExternalHost : Session.Host,
                Port,
                TEXT("pull_runtime_patch_bundle"),
                Params,
                RI_RemoteExternalDefaultTimeoutSeconds,
                ResultValue,
                OutError))
        {
            LastRemotePatchPullSummary = OutError;
            LastRemoteRuntimeSessionError = OutError;
            InvalidateFileManagementSummaryCache();
            return false;
        }

        const TSharedPtr<FJsonObject> ResultObject = ResultValue.IsValid() ? ResultValue->AsObject() : nullptr;
        const TSharedPtr<FJsonObject>* BundleObject = nullptr;
        if (!ResultObject.IsValid()
            || !ResultObject->TryGetObjectField(TEXT("patchBundle"), BundleObject)
            || !BundleObject
            || !BundleObject->IsValid()
            || !RI_RemoteJsonObjectToStruct<FRIPatchBundle>(*BundleObject, OutBundle))
        {
            if (OutError.IsEmpty() && ResultObject.IsValid())
            {
                ResultObject->TryGetStringField(TEXT("error"), OutError);
            }
            if (OutError.IsEmpty())
            {
                OutError = TEXT("Failed to parse pulled patch bundle");
            }
            LastRemotePatchPullSummary = OutError;
            InvalidateFileManagementSummaryCache();
            return false;
        }

        ResultObject->TryGetStringField(TEXT("summary"), LastRemotePatchPullSummary);
        if (LastRemotePatchPullSummary.IsEmpty())
        {
            LastRemotePatchPullSummary = FString::Printf(TEXT("Pulled patch bundle (%d ops)"), OutBundle.Operations.Num());
        }
        InvalidateFileManagementSummaryCache();
        return OutBundle.Operations.Num() > 0;
#else
        OutError = TEXT("External packaged runtime patch pull is editor-only");
        return false;
#endif
    }

    UWorld* World = RI_RemoteResolveWorldForSessionId(this, NormalizedSessionId);
    if (!World)
    {
        OutError = TEXT("Runtime world unavailable");
        return false;
    }

    AActor* PreviousSelectedActor = SelectedActor.Get();
    if (AActor* RequestedActor = RI_RemoteFindActorByRequest(World, NormalizedActorQuery))
    {
        SetSelectedActor(RequestedActor);
    }
    else
    {
        OutError = FString::Printf(TEXT("Requested actor not found: %s"), *NormalizedActorQuery);
        return false;
    }

    const bool bOk = CaptureSelectionAsPatch(OutBundle, OutError);
    if (SelectedActor.Get() != PreviousSelectedActor)
    {
        SetSelectedActor(PreviousSelectedActor);
    }

    LastRemotePatchPullSummary = bOk
        ? FString::Printf(TEXT("Pulled local runtime patch bundle (%d ops)"), OutBundle.Operations.Num())
        : OutError;
    InvalidateFileManagementSummaryCache();
    return bOk;
#endif
}

bool UInspectorWorldSubsystem::RunWorkflowOnRuntimeSession(const FString& SessionId, FName WorkflowId, const FString& ActorQuery, FRIWorkflowRunResult& OutResult, FString& OutError)
{
    OutResult = FRIWorkflowRunResult();
    OutError.Reset();

#if !RUNTIME_INSPECTOR_ENABLED
    OutError = TEXT("RuntimeInspector disabled");
    OutResult.Summary = OutError;
    OutResult.FullReport = OutError;
    return false;
#else
    const FString NormalizedSessionId = SessionId.TrimStartAndEnd();
    if (NormalizedSessionId.IsEmpty())
    {
        OutError = TEXT("Missing session id");
        OutResult.Summary = OutError;
        OutResult.FullReport = OutError;
        return false;
    }

    FRIRuntimeSessionInfo Session;
    if (!ConnectRemoteRuntimeSession(NormalizedSessionId, Session, OutError))
    {
        OutResult.Summary = OutError;
        OutResult.FullReport = OutError;
        return false;
    }

    PreferredRemoteSessionId = Session.SessionId;
    LastRemoteSessionSelectionSummary = Session.DisplayName.IsEmpty() ? Session.SessionId : Session.DisplayName;
    LastRemoteSessionTargetQuery = ActorQuery.TrimStartAndEnd();
    LastRemoteSessionWorkflowId = WorkflowId.ToString();

    if (Session.bIsExternal || RI_IsExternalPackagedSessionId(NormalizedSessionId))
    {
#if WITH_EDITOR
        int32 Port = Session.Port;
        if (Port <= 0 && !RI_TryParseExternalPackagedPort(NormalizedSessionId, Port))
        {
            OutError = FString::Printf(TEXT("Unable to resolve port for runtime session: %s"), *NormalizedSessionId);
            OutResult.Summary = OutError;
            OutResult.FullReport = OutError;
            return false;
        }

        TSharedPtr<FJsonObject> Params = MakeShared<FJsonObject>();
        Params->SetStringField(TEXT("sessionId"), NormalizedSessionId);
        Params->SetStringField(TEXT("workflowId"), WorkflowId.ToString());
        const FString NormalizedActorQuery = ActorQuery.TrimStartAndEnd();
        if (!NormalizedActorQuery.IsEmpty())
        {
            Params->SetStringField(TEXT("actorQuery"), NormalizedActorQuery);
        }

        TSharedPtr<FJsonValue> ResultValue;
        if (!RI_RemoteCallExternalRuntimeJsonRpc(
                Session.Host.IsEmpty() ? RI_RemoteExternalHost : Session.Host,
                Port,
                TEXT("run_runtime_workflow"),
                Params,
                RI_RemoteExternalDefaultTimeoutSeconds,
                ResultValue,
                OutError))
        {
            OutResult.WorkflowId = WorkflowId;
            OutResult.DisplayName = WorkflowId.ToString();
            OutResult.Summary = OutError;
            OutResult.FullReport = OutError;
            return false;
        }

        const TSharedPtr<FJsonObject> ResultObject = ResultValue.IsValid() ? ResultValue->AsObject() : nullptr;
        const TSharedPtr<FJsonObject>* WorkflowObject = nullptr;
        if (!ResultObject.IsValid()
            || !ResultObject->TryGetObjectField(TEXT("result"), WorkflowObject)
            || !WorkflowObject
            || !WorkflowObject->IsValid()
            || !RI_RemoteJsonObjectToStruct<FRIWorkflowRunResult>(*WorkflowObject, OutResult))
        {
            if (OutError.IsEmpty() && ResultObject.IsValid())
            {
                ResultObject->TryGetStringField(TEXT("error"), OutError);
            }
            if (OutError.IsEmpty())
            {
                OutError = TEXT("Failed to parse remote workflow result");
            }
            OutResult.WorkflowId = WorkflowId;
            OutResult.DisplayName = WorkflowId.ToString();
            OutResult.Summary = OutError;
            OutResult.FullReport = OutError;
            return false;
        }

        return OutResult.bPassed;
#else
        OutError = TEXT("External packaged runtime workflow execution is editor-only");
        OutResult.WorkflowId = WorkflowId;
        OutResult.DisplayName = WorkflowId.ToString();
        OutResult.Summary = OutError;
        OutResult.FullReport = OutError;
        return false;
#endif
    }

    AActor* PreviousSelectedActor = SelectedActor.Get();
    if (!ActorQuery.TrimStartAndEnd().IsEmpty())
    {
        UWorld* World = RI_RemoteResolveWorldForSessionId(this, NormalizedSessionId);
        AActor* RequestedActor = World ? RI_RemoteFindActorByRequest(World, ActorQuery.TrimStartAndEnd()) : nullptr;
        if (!RequestedActor)
        {
            OutError = FString::Printf(TEXT("Requested actor not found: %s"), *ActorQuery.TrimStartAndEnd());
            OutResult.WorkflowId = WorkflowId;
            OutResult.DisplayName = WorkflowId.ToString();
            OutResult.Summary = OutError;
            OutResult.FullReport = OutError;
            return false;
        }
        SetSelectedActor(RequestedActor);
    }

    const bool bOk = RunWorkflowById(WorkflowId, OutResult);
    if (SelectedActor.Get() != PreviousSelectedActor)
    {
        SetSelectedActor(PreviousSelectedActor);
    }
    return bOk;
#endif
}

bool UInspectorWorldSubsystem::CompareRuntimeTargetsAcrossSessions(const FString& LeftSessionId, const FString& RightSessionId, const FString& TargetQuery, FRIRuntimeSessionTargetCompareReport& OutReport, FString& OutError) const
{
    OutReport = FRIRuntimeSessionTargetCompareReport();
    OutError.Reset();

#if !RUNTIME_INSPECTOR_ENABLED
    OutError = TEXT("RuntimeInspector disabled");
    const_cast<UInspectorWorldSubsystem*>(this)->LastRuntimeSessionTargetCompareReport = OutReport;
    return false;
#else
    const FString NormalizedLeftSessionId = LeftSessionId.TrimStartAndEnd();
    const FString NormalizedRightSessionId = RightSessionId.TrimStartAndEnd();
    const FString NormalizedTargetQuery = TargetQuery.TrimStartAndEnd();
    if (NormalizedLeftSessionId.IsEmpty() || NormalizedRightSessionId.IsEmpty())
    {
        OutError = TEXT("Missing left or right session id");
        const_cast<UInspectorWorldSubsystem*>(this)->LastRuntimeSessionTargetCompareReport = OutReport;
        return false;
    }

    if (NormalizedTargetQuery.IsEmpty())
    {
        OutError = TEXT("Missing target query");
        const_cast<UInspectorWorldSubsystem*>(this)->LastRuntimeSessionTargetCompareReport = OutReport;
        return false;
    }

    const FString PreviousConnectedSessionId = ConnectedRuntimeSessionId;
    const FString PreviousRemoteError = LastRemoteRuntimeSessionError;
    UInspectorWorldSubsystem* MutableThis = const_cast<UInspectorWorldSubsystem*>(this);
    const auto RestoreState = [MutableThis, PreviousConnectedSessionId, PreviousRemoteError]()
    {
        MutableThis->ConnectedRuntimeSessionId = PreviousConnectedSessionId;
        MutableThis->LastRemoteRuntimeSessionError = PreviousRemoteError;
    };

    TArray<FRIRuntimeTargetInfo> LeftTargets;
    TArray<FRIRuntimeTargetInfo> RightTargets;
    FString LeftError;
    FString RightError;
    FRIRuntimeSessionInfo LeftSession;
    FRIRuntimeSessionInfo RightSession;

    const bool bLeftConnectOk = const_cast<UInspectorWorldSubsystem*>(this)->ConnectRemoteRuntimeSession(NormalizedLeftSessionId, LeftSession, LeftError);
    const bool bLeftListOk = bLeftConnectOk && ListRuntimeTargetsForSession(NormalizedLeftSessionId, LeftTargets, LeftError, FString(), FString(), 500);
    const bool bRightConnectOk = const_cast<UInspectorWorldSubsystem*>(this)->ConnectRemoteRuntimeSession(NormalizedRightSessionId, RightSession, RightError);
    const bool bRightListOk = bRightConnectOk && ListRuntimeTargetsForSession(NormalizedRightSessionId, RightTargets, RightError, FString(), FString(), 500);

    RestoreState();

    if (!bLeftListOk || !bRightListOk)
    {
        OutError = FString::Printf(
            TEXT("Session compare failed | Left=%s | Right=%s"),
            LeftError.IsEmpty() ? TEXT("ok") : *LeftError,
            RightError.IsEmpty() ? TEXT("ok") : *RightError);
        const_cast<UInspectorWorldSubsystem*>(this)->LastRuntimeSessionTargetCompareReport = OutReport;
        return false;
    }

    const FRIRuntimeTargetInfo* LeftTarget = RI_RemoteFindTargetByQuery(LeftTargets, NormalizedTargetQuery);
    const FRIRuntimeTargetInfo* RightTarget = RI_RemoteFindTargetByQuery(RightTargets, NormalizedTargetQuery);

    OutReport.GeneratedAtUtc = FDateTime::UtcNow().ToIso8601();
    OutReport.LeftSessionId = NormalizedLeftSessionId;
    OutReport.RightSessionId = NormalizedRightSessionId;
    OutReport.TargetQuery = NormalizedTargetQuery;
    OutReport.bLeftTargetFound = LeftTarget != nullptr;
    OutReport.bRightTargetFound = RightTarget != nullptr;
    OutReport.LeftTargetPath = LeftTarget ? LeftTarget->ActorPath : FString();
    OutReport.RightTargetPath = RightTarget ? RightTarget->ActorPath : FString();
    OutReport.LeftTargetLabel = LeftTarget ? LeftTarget->ActorLabel : FString();
    OutReport.RightTargetLabel = RightTarget ? RightTarget->ActorLabel : FString();
    OutReport.LeftTargetClass = LeftTarget ? LeftTarget->ActorClass : FString();
    OutReport.RightTargetClass = RightTarget ? RightTarget->ActorClass : FString();

    if (LeftTarget || RightTarget)
    {
        RI_RemoteAddCompareField(OutReport, TEXT("ActorLabel"), LeftTarget ? LeftTarget->ActorLabel : FString(), RightTarget ? RightTarget->ActorLabel : FString());
        RI_RemoteAddCompareField(OutReport, TEXT("ActorClass"), LeftTarget ? LeftTarget->ActorClass : FString(), RightTarget ? RightTarget->ActorClass : FString());
        RI_RemoteAddCompareField(OutReport, TEXT("Replicates"), LeftTarget ? (LeftTarget->bReplicates ? TEXT("yes") : TEXT("no")) : FString(), RightTarget ? (RightTarget->bReplicates ? TEXT("yes") : TEXT("no")) : FString());
        RI_RemoteAddCompareField(OutReport, TEXT("ReplicateMovement"), LeftTarget ? (LeftTarget->bReplicateMovement ? TEXT("yes") : TEXT("no")) : FString(), RightTarget ? (RightTarget->bReplicateMovement ? TEXT("yes") : TEXT("no")) : FString());
        RI_RemoteAddCompareField(OutReport, TEXT("LocalRole"), LeftTarget ? LeftTarget->LocalRoleLabel : FString(), RightTarget ? RightTarget->LocalRoleLabel : FString());
        RI_RemoteAddCompareField(OutReport, TEXT("RemoteRole"), LeftTarget ? LeftTarget->RemoteRoleLabel : FString(), RightTarget ? RightTarget->RemoteRoleLabel : FString());
        RI_RemoteAddCompareField(OutReport, TEXT("ActorPath"), LeftTarget ? LeftTarget->ActorPath : FString(), RightTarget ? RightTarget->ActorPath : FString());
    }

    TArray<FString> DetailLines;
    DetailLines.Add(FString::Printf(
        TEXT("SessionTargetCompare | Left=%s Right=%s | Query=%s"),
        *NormalizedLeftSessionId,
        *NormalizedRightSessionId,
        *NormalizedTargetQuery));
    DetailLines.Add(FString::Printf(
        TEXT("Targets | Left=%s | Right=%s"),
        OutReport.bLeftTargetFound ? *OutReport.LeftTargetPath : TEXT("missing"),
        OutReport.bRightTargetFound ? *OutReport.RightTargetPath : TEXT("missing")));

    for (const FRIRuntimeSessionCompareField& Field : OutReport.Fields)
    {
        DetailLines.Add(FString::Printf(
            TEXT("%s | Left=%s | Right=%s | %s"),
            *Field.FieldName,
            *Field.LeftValue,
            *Field.RightValue,
            Field.bDifferent ? TEXT("Different") : TEXT("Same")));
    }

    OutReport.Details = FString::Join(DetailLines, TEXT("\n"));
    OutReport.Summary = FString::Printf(
        TEXT("SessionTargetCompare | Left=%s Right=%s Query=%s | LeftFound=%s RightFound=%s | Fields=%d Diff=%d"),
        *NormalizedLeftSessionId,
        *NormalizedRightSessionId,
        *NormalizedTargetQuery,
        OutReport.bLeftTargetFound ? TEXT("yes") : TEXT("no"),
        OutReport.bRightTargetFound ? TEXT("yes") : TEXT("no"),
        OutReport.FieldCount,
        OutReport.DifferenceCount);

    const bool bPassed = OutReport.bLeftTargetFound && OutReport.bRightTargetFound && OutReport.FieldCount > 0;
    const_cast<UInspectorWorldSubsystem*>(this)->LastRuntimeSessionTargetCompareReport = OutReport;
    return bPassed;
#endif
}

bool UInspectorWorldSubsystem::CompareRuntimeTargetSetsAcrossSessions(const FString& LeftSessionId, const FString& RightSessionId, const FString& NameFilter, const FString& ClassFilter, FRIRuntimeSessionTargetSetCompareReport& OutReport, FString& OutError) const
{
    OutReport = FRIRuntimeSessionTargetSetCompareReport();
    OutError.Reset();

#if !RUNTIME_INSPECTOR_ENABLED
    OutError = TEXT("RuntimeInspector disabled");
    const_cast<UInspectorWorldSubsystem*>(this)->LastRuntimeSessionTargetSetCompareReport = OutReport;
    return false;
#else
    const FString NormalizedLeftSessionId = LeftSessionId.TrimStartAndEnd();
    const FString NormalizedRightSessionId = RightSessionId.TrimStartAndEnd();
    const FString NormalizedNameFilter = NameFilter.TrimStartAndEnd();
    const FString NormalizedClassFilter = ClassFilter.TrimStartAndEnd();
    if (NormalizedLeftSessionId.IsEmpty() || NormalizedRightSessionId.IsEmpty())
    {
        OutError = TEXT("Missing left or right session id");
        const_cast<UInspectorWorldSubsystem*>(this)->LastRuntimeSessionTargetSetCompareReport = OutReport;
        return false;
    }

    const FString PreviousConnectedSessionId = ConnectedRuntimeSessionId;
    const FString PreviousRemoteError = LastRemoteRuntimeSessionError;
    UInspectorWorldSubsystem* MutableThis = const_cast<UInspectorWorldSubsystem*>(this);
    const auto RestoreState = [MutableThis, PreviousConnectedSessionId, PreviousRemoteError]()
    {
        MutableThis->ConnectedRuntimeSessionId = PreviousConnectedSessionId;
        MutableThis->LastRemoteRuntimeSessionError = PreviousRemoteError;
    };

    TArray<FRIRuntimeTargetInfo> LeftTargets;
    TArray<FRIRuntimeTargetInfo> RightTargets;
    FString LeftError;
    FString RightError;
    FRIRuntimeSessionInfo LeftSession;
    FRIRuntimeSessionInfo RightSession;

    const bool bLeftConnectOk = MutableThis->ConnectRemoteRuntimeSession(NormalizedLeftSessionId, LeftSession, LeftError);
    const bool bLeftListOk = bLeftConnectOk && ListRuntimeTargetsForSession(NormalizedLeftSessionId, LeftTargets, LeftError, NormalizedNameFilter, NormalizedClassFilter, 500);
    const bool bRightConnectOk = MutableThis->ConnectRemoteRuntimeSession(NormalizedRightSessionId, RightSession, RightError);
    const bool bRightListOk = bRightConnectOk && ListRuntimeTargetsForSession(NormalizedRightSessionId, RightTargets, RightError, NormalizedNameFilter, NormalizedClassFilter, 500);

    RestoreState();

    if (!bLeftListOk || !bRightListOk)
    {
        OutError = FString::Printf(
            TEXT("Session target-set compare failed | Left=%s | Right=%s"),
            LeftError.IsEmpty() ? TEXT("ok") : *LeftError,
            RightError.IsEmpty() ? TEXT("ok") : *RightError);
        const_cast<UInspectorWorldSubsystem*>(this)->LastRuntimeSessionTargetSetCompareReport = OutReport;
        return false;
    }

    TMap<FString, TArray<const FRIRuntimeTargetInfo*>> LeftByKey;
    TMap<FString, TArray<const FRIRuntimeTargetInfo*>> RightByKey;
    TSet<FString> AllKeys;

    for (const FRIRuntimeTargetInfo& Target : LeftTargets)
    {
        const FString Key = RI_RemoteBuildTargetSetCompareKey(Target);
        LeftByKey.FindOrAdd(Key).Add(&Target);
        AllKeys.Add(Key);
    }

    for (const FRIRuntimeTargetInfo& Target : RightTargets)
    {
        const FString Key = RI_RemoteBuildTargetSetCompareKey(Target);
        RightByKey.FindOrAdd(Key).Add(&Target);
        AllKeys.Add(Key);
    }

    TArray<FString> SortedKeys = AllKeys.Array();
    SortedKeys.Sort();

    OutReport.GeneratedAtUtc = FDateTime::UtcNow().ToIso8601();
    OutReport.LeftSessionId = NormalizedLeftSessionId;
    OutReport.RightSessionId = NormalizedRightSessionId;
    OutReport.NameFilter = NormalizedNameFilter;
    OutReport.ClassFilter = NormalizedClassFilter;
    OutReport.LeftMatchCount = LeftTargets.Num();
    OutReport.RightMatchCount = RightTargets.Num();

    TArray<FString> DetailLines;
    DetailLines.Add(FString::Printf(
        TEXT("SessionTargetSetCompare | Left=%s Right=%s | NameFilter=%s | ClassFilter=%s"),
        *NormalizedLeftSessionId,
        *NormalizedRightSessionId,
        NormalizedNameFilter.IsEmpty() ? TEXT("-") : *NormalizedNameFilter,
        NormalizedClassFilter.IsEmpty() ? TEXT("-") : *NormalizedClassFilter));

    for (const FString& Key : SortedKeys)
    {
        const TArray<const FRIRuntimeTargetInfo*>* LeftMatches = LeftByKey.Find(Key);
        const TArray<const FRIRuntimeTargetInfo*>* RightMatches = RightByKey.Find(Key);
        const FRIRuntimeTargetInfo* LeftPrimary = (LeftMatches && LeftMatches->Num() > 0) ? (*LeftMatches)[0] : nullptr;
        const FRIRuntimeTargetInfo* RightPrimary = (RightMatches && RightMatches->Num() > 0) ? (*RightMatches)[0] : nullptr;

        FRIRuntimeSessionTargetSetCompareLine Line;
        Line.CompareKey = Key;
        Line.DisplayLabel = LeftPrimary && !LeftPrimary->ActorLabel.IsEmpty()
            ? LeftPrimary->ActorLabel
            : (RightPrimary && !RightPrimary->ActorLabel.IsEmpty() ? RightPrimary->ActorLabel : (LeftPrimary ? LeftPrimary->ActorName : (RightPrimary ? RightPrimary->ActorName : Key)));
        Line.ActorClass = LeftPrimary ? LeftPrimary->ActorClass : (RightPrimary ? RightPrimary->ActorClass : FString());
        Line.bPresentInLeft = LeftPrimary != nullptr;
        Line.bPresentInRight = RightPrimary != nullptr;
        Line.LeftCount = LeftMatches ? LeftMatches->Num() : 0;
        Line.RightCount = RightMatches ? RightMatches->Num() : 0;
        Line.LeftPrimaryPath = LeftPrimary ? LeftPrimary->ActorPath : FString();
        Line.RightPrimaryPath = RightPrimary ? RightPrimary->ActorPath : FString();

        TArray<FString> MismatchReasons;
        if (Line.bPresentInLeft && Line.bPresentInRight)
        {
            ++OutReport.SharedTargetCount;
            if (Line.LeftCount != Line.RightCount)
            {
                MismatchReasons.Add(TEXT("count"));
            }
            if (LeftPrimary && RightPrimary)
            {
                if (LeftPrimary->ActorLabel != RightPrimary->ActorLabel)
                {
                    MismatchReasons.Add(TEXT("label"));
                }
                if (LeftPrimary->bReplicates != RightPrimary->bReplicates)
                {
                    MismatchReasons.Add(TEXT("replicates"));
                }
                if (LeftPrimary->bReplicateMovement != RightPrimary->bReplicateMovement)
                {
                    MismatchReasons.Add(TEXT("replicate-movement"));
                }
                if (LeftPrimary->LocalRoleLabel != RightPrimary->LocalRoleLabel)
                {
                    MismatchReasons.Add(TEXT("local-role"));
                }
                if (LeftPrimary->RemoteRoleLabel != RightPrimary->RemoteRoleLabel)
                {
                    MismatchReasons.Add(TEXT("remote-role"));
                }
            }
        }
        else if (Line.bPresentInLeft)
        {
            ++OutReport.LeftOnlyCount;
            MismatchReasons.Add(TEXT("left-only"));
        }
        else if (Line.bPresentInRight)
        {
            ++OutReport.RightOnlyCount;
            MismatchReasons.Add(TEXT("right-only"));
        }

        Line.bHasMismatch = MismatchReasons.Num() > 0;
        if (Line.bHasMismatch)
        {
            ++OutReport.MismatchCount;
        }
        Line.Message = Line.bHasMismatch ? FString::Join(MismatchReasons, TEXT(",")) : TEXT("match");
        OutReport.Lines.Add(Line);

        DetailLines.Add(FString::Printf(
            TEXT("%s | Left=%d Right=%d | %s"),
            *Line.CompareKey,
            Line.LeftCount,
            Line.RightCount,
            *Line.Message));
    }

    OutReport.LineCount = OutReport.Lines.Num();
    OutReport.Details = FString::Join(DetailLines, TEXT("\n"));
    OutReport.Summary = FString::Printf(
        TEXT("SessionTargetSetCompare | Left=%s Right=%s | NameFilter=%s | ClassFilter=%s | LeftCount=%d RightCount=%d Shared=%d LeftOnly=%d RightOnly=%d Mismatch=%d"),
        *NormalizedLeftSessionId,
        *NormalizedRightSessionId,
        NormalizedNameFilter.IsEmpty() ? TEXT("-") : *NormalizedNameFilter,
        NormalizedClassFilter.IsEmpty() ? TEXT("-") : *NormalizedClassFilter,
        OutReport.LeftMatchCount,
        OutReport.RightMatchCount,
        OutReport.SharedTargetCount,
        OutReport.LeftOnlyCount,
        OutReport.RightOnlyCount,
        OutReport.MismatchCount);

    const bool bPassed = OutReport.LineCount > 0 && OutReport.SharedTargetCount > 0;
    const_cast<UInspectorWorldSubsystem*>(this)->LastRuntimeSessionTargetSetCompareReport = OutReport;
    return bPassed;
#endif
}

void UInspectorWorldSubsystem::SetActiveRemoteSessionTargetSetCompareRequest(const FRIRuntimeSessionTargetSetCompareRequest& InRequest)
{
    ActiveRemoteSessionTargetSetCompareRequest = InRequest;
}

void UInspectorWorldSubsystem::SetRemoteSessionUIContext(const FString& SessionId, const FString& SelectionSummary, const FString& TargetQuery, const FString& WorkflowId)
{
    PreferredRemoteSessionId = SessionId.TrimStartAndEnd();
    LastRemoteSessionSelectionSummary = SelectionSummary.TrimStartAndEnd();
    LastRemoteSessionTargetQuery = TargetQuery.TrimStartAndEnd();
    LastRemoteSessionWorkflowId = WorkflowId.TrimStartAndEnd();
    InvalidateFileManagementSummaryCache();
}

void UInspectorWorldSubsystem::ClearActiveRemoteSessionTargetSetCompareRequest()
{
    ActiveRemoteSessionTargetSetCompareRequest = FRIRuntimeSessionTargetSetCompareRequest();
}

TArray<FRIRuntimeSessionTargetSetCompareMatrixDefinition> UInspectorWorldSubsystem::GetAvailableRuntimeSessionTargetSetCompareMatrices() const
{
    TArray<FRIRuntimeSessionTargetSetCompareMatrixDefinition> Results;
#if RUNTIME_INSPECTOR_ENABLED
    FRIRuntimeSessionTargetSetCompareMatrixDefinition DefaultMatrix;
    DefaultMatrix.MatrixId = RI_RemoteCompareMatrixId_Default;
    DefaultMatrix.DisplayName = TEXT("Mainline Remote Session Compare Matrix");
    DefaultMatrix.Description = TEXT("Runs the curated editor-vs-PIE target-set compare matrix: full inventory plus BP_TestVarsActor-scoped inventory.");
    DefaultMatrix.bRequiresPIE = true;

    FRIRuntimeSessionTargetSetCompareMatrixEntry FullInventoryEntry;
    FullInventoryEntry.EntryId = TEXT("editor_vs_pie_full_inventory");
    FullInventoryEntry.DisplayName = TEXT("Editor vs PIE Full Inventory");
    FullInventoryEntry.Description = TEXT("Compares the full editor and PIE runtime target inventories without filters.");
    FullInventoryEntry.Request.LeftSessionId = TEXT("local_editor_current");
    FullInventoryEntry.Request.RightSessionId = TEXT("local_pie_current");
    DefaultMatrix.Entries.Add(MoveTemp(FullInventoryEntry));

    FRIRuntimeSessionTargetSetCompareMatrixEntry ScopedEntry;
    ScopedEntry.EntryId = TEXT("editor_vs_pie_bp_testvarsactor_scoped");
    ScopedEntry.DisplayName = TEXT("Editor vs PIE BP_TestVarsActor");
    ScopedEntry.Description = TEXT("Compares editor and PIE runtime target inventories with explicit BP_TestVarsActor name/class filters.");
    ScopedEntry.Request.LeftSessionId = TEXT("local_editor_current");
    ScopedEntry.Request.RightSessionId = TEXT("local_pie_current");
    ScopedEntry.Request.NameFilter = TEXT("BP_TestVarsActor");
    ScopedEntry.Request.ClassFilter = TEXT("BP_TestVarsActor");
    DefaultMatrix.Entries.Add(MoveTemp(ScopedEntry));

    Results.Add(MoveTemp(DefaultMatrix));
#endif
    return Results;
}

bool UInspectorWorldSubsystem::RunRuntimeSessionTargetSetCompareMatrixById(FName MatrixId, FRIRuntimeSessionTargetSetCompareMatrixRunResult& OutResult)
{
    OutResult = FRIRuntimeSessionTargetSetCompareMatrixRunResult();

#if !RUNTIME_INSPECTOR_ENABLED
    OutResult.MatrixId = MatrixId;
    OutResult.DisplayName = MatrixId.ToString();
    OutResult.bPassed = false;
    OutResult.bBlocked = true;
    OutResult.Summary = TEXT("RuntimeInspector disabled");
    OutResult.FullReport = OutResult.Summary;
    LastRuntimeSessionTargetSetCompareMatrixRunResult = OutResult;
    return false;
#else
    const TArray<FRIRuntimeSessionTargetSetCompareMatrixDefinition> Definitions = GetAvailableRuntimeSessionTargetSetCompareMatrices();
    const FRIRuntimeSessionTargetSetCompareMatrixDefinition* Definition = Definitions.FindByPredicate([MatrixId](const FRIRuntimeSessionTargetSetCompareMatrixDefinition& Candidate)
    {
        return Candidate.MatrixId == MatrixId;
    });

    if (!Definition)
    {
        OutResult.MatrixId = MatrixId;
        OutResult.DisplayName = MatrixId.ToString();
        OutResult.bPassed = false;
        OutResult.Summary = FString::Printf(TEXT("Unknown remote session compare matrix: %s"), *MatrixId.ToString());
        OutResult.FullReport = OutResult.Summary;
        LastRuntimeSessionTargetSetCompareMatrixRunResult = OutResult;
        return false;
    }

    OutResult.MatrixId = Definition->MatrixId;
    OutResult.DisplayName = Definition->DisplayName;
    if (Definition->bRequiresPIE && !IsSelfTestPIEAvailable())
    {
        OutResult.bPassed = false;
        OutResult.bBlocked = true;
        OutResult.Summary = TEXT("Blocked: PIE with a local player controller is required.");
        OutResult.FullReport = OutResult.Summary;
        LastRuntimeSessionTargetSetCompareMatrixRunResult = OutResult;
        return false;
    }

    TArray<FString> ReportSections;
    for (const FRIRuntimeSessionTargetSetCompareMatrixEntry& Entry : Definition->Entries)
    {
        FRIRuntimeSessionTargetSetCompareMatrixEntryResult EntryResult;
        EntryResult.EntryId = Entry.EntryId;
        EntryResult.DisplayName = Entry.DisplayName;
        EntryResult.Request = RI_RemoteResolveTargetSetCompareRequest(Entry.Request);

        FString CompareError;
        EntryResult.bPassed = CompareRuntimeTargetSetsAcrossSessions(
            EntryResult.Request.LeftSessionId,
            EntryResult.Request.RightSessionId,
            EntryResult.Request.NameFilter,
            EntryResult.Request.ClassFilter,
            EntryResult.Report,
            CompareError);

        const bool bSessionEchoOk = EntryResult.Report.LeftSessionId == EntryResult.Request.LeftSessionId
            && EntryResult.Report.RightSessionId == EntryResult.Request.RightSessionId;
        const bool bFilterEchoOk = EntryResult.Report.NameFilter == EntryResult.Request.NameFilter
            && EntryResult.Report.ClassFilter == EntryResult.Request.ClassFilter;
        const bool bHasLines = EntryResult.Report.LineCount > 0;
        const bool bHasShared = EntryResult.Report.SharedTargetCount > 0;
        const bool bNameFilterOk = EntryResult.Request.NameFilter.IsEmpty()
            || EntryResult.Report.Lines.ContainsByPredicate([&EntryResult](const FRIRuntimeSessionTargetSetCompareLine& Line)
            {
                return Line.CompareKey.Contains(EntryResult.Request.NameFilter, ESearchCase::IgnoreCase)
                    || Line.DisplayLabel.Contains(EntryResult.Request.NameFilter, ESearchCase::IgnoreCase);
            });
        const bool bClassFilterOk = EntryResult.Request.ClassFilter.IsEmpty()
            || EntryResult.Report.Lines.ContainsByPredicate([&EntryResult](const FRIRuntimeSessionTargetSetCompareLine& Line)
            {
                return Line.ActorClass.Contains(EntryResult.Request.ClassFilter, ESearchCase::IgnoreCase)
                    || Line.CompareKey.Contains(EntryResult.Request.ClassFilter, ESearchCase::IgnoreCase);
            });

        EntryResult.bPassed = EntryResult.bPassed
            && bSessionEchoOk
            && bFilterEchoOk
            && bHasLines
            && bHasShared
            && bNameFilterOk
            && bClassFilterOk;
        EntryResult.Summary = FString::Printf(
            TEXT("%s=%s | Left=%s Right=%s | NameFilter=%s | ClassFilter=%s | Lines=%d Shared=%d Mismatch=%d"),
            *EntryResult.EntryId.ToString(),
            EntryResult.bPassed ? TEXT("PASS") : TEXT("FAIL"),
            *EntryResult.Request.LeftSessionId,
            *EntryResult.Request.RightSessionId,
            EntryResult.Request.NameFilter.IsEmpty() ? TEXT("-") : *EntryResult.Request.NameFilter,
            EntryResult.Request.ClassFilter.IsEmpty() ? TEXT("-") : *EntryResult.Request.ClassFilter,
            EntryResult.Report.LineCount,
            EntryResult.Report.SharedTargetCount,
            EntryResult.Report.MismatchCount);
        EntryResult.FullReport = FString::Printf(
            TEXT("%s | Error=%s | Report=%s"),
            *EntryResult.Summary,
            CompareError.IsEmpty() ? TEXT("-") : *CompareError,
            EntryResult.Report.Summary.IsEmpty() ? TEXT("-") : *EntryResult.Report.Summary);

        OutResult.PassedEntryCount += EntryResult.bPassed ? 1 : 0;
        OutResult.FailedEntryCount += EntryResult.bPassed ? 0 : 1;
        ReportSections.Add(EntryResult.FullReport);
        OutResult.EntryResults.Add(MoveTemp(EntryResult));
    }

    OutResult.bPassed = OutResult.FailedEntryCount == 0 && OutResult.EntryResults.Num() > 0;
    OutResult.Summary = FString::Printf(
        TEXT("%s=%s | Passed=%d Failed=%d"),
        *OutResult.MatrixId.ToString(),
        OutResult.bPassed ? TEXT("PASS") : TEXT("FAIL"),
        OutResult.PassedEntryCount,
        OutResult.FailedEntryCount);
    OutResult.FullReport = FString::Join(ReportSections, TEXT("\n"));
    LastRuntimeSessionTargetSetCompareMatrixRunResult = OutResult;
    return OutResult.bPassed;
#endif
}

bool UInspectorWorldSubsystem::RunRemoteRuntimeFoundationSelfTest(FString& OutReport)
{
    OutReport.Reset();

#if !RUNTIME_INSPECTOR_ENABLED
    OutReport = TEXT("RemoteRuntimeFoundationSelfTest=BLOCKED | RuntimeInspector disabled");
    return false;
#else
    if (!IsSelfTestPIEAvailable())
    {
        OutReport = TEXT("RemoteRuntimeFoundationSelfTest=BLOCKED | PIE with local player required");
        return false;
    }

    const FString PreviousConnectedSessionId = ConnectedRuntimeSessionId;
    const FString PreviousRemoteError = LastRemoteRuntimeSessionError;

    const auto RestoreState = [this, PreviousConnectedSessionId, PreviousRemoteError]()
    {
        ConnectedRuntimeSessionId = PreviousConnectedSessionId;
        LastRemoteRuntimeSessionError = PreviousRemoteError;
    };

    const TArray<FRIRuntimeSessionInfo> Sessions = GetAvailableRuntimeSessions();
    const bool bSessionsOk = Sessions.Num() > 0;
    const FRIRuntimeSessionInfo* PreferredSession = Sessions.FindByPredicate([](const FRIRuntimeSessionInfo& Session)
    {
        return Session.SessionId == TEXT("local_pie_current");
    });
    const FString SessionId = bSessionsOk ? (PreferredSession ? PreferredSession->SessionId : Sessions[0].SessionId) : FString();

    FRIRuntimeSessionInfo ConnectedSession;
    FString ConnectError;
    const bool bConnectOk = bSessionsOk && ConnectRemoteRuntimeSession(SessionId, ConnectedSession, ConnectError);

    TArray<FRIRuntimeTargetInfo> Targets;
    FString ListError;
    const bool bListOk = bConnectOk && ListRuntimeTargetsForSession(SessionId, Targets, ListError, FString(), FString(), 50);

    const AActor* SelectedActorPtr = SelectedActor.Get();
    const FString SelectedActorPath = SelectedActorPtr ? SelectedActorPtr->GetPathName() : FString();
    const bool bSelectedTargetOk = SelectedActorPath.IsEmpty()
        || Targets.ContainsByPredicate([&SelectedActorPath](const FRIRuntimeTargetInfo& Target)
        {
            return Target.ActorPath == SelectedActorPath && Target.bSelected;
        });

    const bool bTargetsOk = bListOk && Targets.Num() > 0;
    const bool bConnectedStateOk = bConnectOk
        && ConnectedSession.ConnectionState == ERIRuntimeSessionConnectionState::Connected
        && ConnectedSession.bSessionAvailable;
    const bool bPermissionOk = bConnectOk
        && ConnectedSession.bRuntimeEnabled
        && (!ConnectedSession.bUnlockRequired || ConnectedSession.bUnlocked);

    const bool bPassed = bSessionsOk && bConnectedStateOk && bPermissionOk && bTargetsOk && bSelectedTargetOk;
    OutReport = FString::Printf(
        TEXT("RemoteRuntimeFoundationSelfTest=%s | Sessions=%d Connect=%s SessionId=%s Runtime=%s Unlock=%s Targets=%d Selected=%s"),
        bPassed ? TEXT("PASS") : TEXT("FAIL"),
        Sessions.Num(),
        bConnectOk ? TEXT("ok") : *(ConnectError.IsEmpty() ? TEXT("fail") : ConnectError),
        SessionId.IsEmpty() ? TEXT("None") : *SessionId,
        bConnectOk && ConnectedSession.bRuntimeEnabled ? TEXT("enabled") : TEXT("disabled"),
        bConnectOk ? (ConnectedSession.bUnlockRequired ? (ConnectedSession.bUnlocked ? TEXT("unlocked") : TEXT("locked")) : TEXT("not-required")) : TEXT("unknown"),
        Targets.Num(),
        bSelectedTargetOk ? (SelectedActorPath.IsEmpty() ? TEXT("none") : TEXT("ok")) : TEXT("missing"));

    RestoreState();
    return bPassed;
#endif
}

FString UInspectorWorldSubsystem::RunRemoteRuntimeFoundationSelfTestSimple()
{
#if !RUNTIME_INSPECTOR_ENABLED
    return TEXT("RuntimeInspector disabled");
#else
    FString Report;
    RunRemoteRuntimeFoundationSelfTest(Report);
    return Report;
#endif
}

bool UInspectorWorldSubsystem::RunRemoteSessionCompareSelfTest(FString& OutReport)
{
    OutReport.Reset();

#if !RUNTIME_INSPECTOR_ENABLED
    OutReport = TEXT("RemoteSessionCompareSelfTest=BLOCKED | RuntimeInspector disabled");
    return false;
#else
    if (!IsSelfTestPIEAvailable())
    {
        OutReport = TEXT("RemoteSessionCompareSelfTest=BLOCKED | PIE with local player required");
        return false;
    }

    const FRIRuntimeSessionTargetSetCompareRequest Request = GetActiveRemoteSessionTargetSetCompareRequest();
    const FString LeftSessionId = Request.LeftSessionId.TrimStartAndEnd().IsEmpty() ? TEXT("local_editor_current") : Request.LeftSessionId.TrimStartAndEnd();
    const FString RightSessionId = Request.RightSessionId.TrimStartAndEnd().IsEmpty() ? TEXT("local_pie_current") : Request.RightSessionId.TrimStartAndEnd();

    const TArray<FRIRuntimeSessionInfo> Sessions = GetAvailableRuntimeSessions();
    const bool bHasLeftSession = Sessions.ContainsByPredicate([&LeftSessionId](const FRIRuntimeSessionInfo& Session)
    {
        return Session.SessionId == LeftSessionId;
    });
    const bool bHasRightSession = Sessions.ContainsByPredicate([&RightSessionId](const FRIRuntimeSessionInfo& Session)
    {
        return Session.SessionId == RightSessionId;
    });

    FRIRuntimeSessionTargetCompareReport CompareReport;
    FString CompareError;
    const bool bCompareOk = bHasLeftSession
        && bHasRightSession
        && CompareRuntimeTargetsAcrossSessions(LeftSessionId, RightSessionId, TEXT("BP_TestVarsActor"), CompareReport, CompareError);

    const bool bPassed = bHasLeftSession
        && bHasRightSession
        && bCompareOk
        && CompareReport.bLeftTargetFound
        && CompareReport.bRightTargetFound
        && CompareReport.FieldCount > 0
        && CompareReport.DifferenceCount > 0;

    OutReport = FString::Printf(
        TEXT("RemoteSessionCompareSelfTest=%s | Sessions=%d Left=%s Right=%s Query=%s LeftFound=%s RightFound=%s Fields=%d Diff=%d Error=%s"),
        bPassed ? TEXT("PASS") : TEXT("FAIL"),
        Sessions.Num(),
        bHasLeftSession ? *LeftSessionId : TEXT("missing"),
        bHasRightSession ? *RightSessionId : TEXT("missing"),
        TEXT("BP_TestVarsActor"),
        CompareReport.bLeftTargetFound ? TEXT("yes") : TEXT("no"),
        CompareReport.bRightTargetFound ? TEXT("yes") : TEXT("no"),
        CompareReport.FieldCount,
        CompareReport.DifferenceCount,
        CompareError.IsEmpty() ? TEXT("-") : *CompareError);
    return bPassed;
#endif
}

FString UInspectorWorldSubsystem::RunRemoteSessionCompareSelfTestSimple()
{
#if !RUNTIME_INSPECTOR_ENABLED
    return TEXT("RuntimeInspector disabled");
#else
    FString Report;
    RunRemoteSessionCompareSelfTest(Report);
    return Report;
#endif
}

bool UInspectorWorldSubsystem::RunRemoteSessionTargetSetCompareSelfTest(FString& OutReport)
{
    OutReport.Reset();

#if !RUNTIME_INSPECTOR_ENABLED
    OutReport = TEXT("RemoteSessionTargetSetCompareSelfTest=BLOCKED | RuntimeInspector disabled");
    return false;
#else
    if (!IsSelfTestPIEAvailable())
    {
        OutReport = TEXT("RemoteSessionTargetSetCompareSelfTest=BLOCKED | PIE with local player required");
        return false;
    }

    const FRIRuntimeSessionTargetSetCompareRequest Request = GetActiveRemoteSessionTargetSetCompareRequest();
    const FString LeftSessionId = Request.LeftSessionId.TrimStartAndEnd().IsEmpty() ? TEXT("local_editor_current") : Request.LeftSessionId.TrimStartAndEnd();
    const FString RightSessionId = Request.RightSessionId.TrimStartAndEnd().IsEmpty() ? TEXT("local_pie_current") : Request.RightSessionId.TrimStartAndEnd();
    const FString NameFilter = Request.NameFilter.TrimStartAndEnd();
    const FString ClassFilter = Request.ClassFilter.TrimStartAndEnd();

    const TArray<FRIRuntimeSessionInfo> Sessions = GetAvailableRuntimeSessions();
    const bool bHasLeftSession = Sessions.ContainsByPredicate([&LeftSessionId](const FRIRuntimeSessionInfo& Session)
    {
        return Session.SessionId == LeftSessionId;
    });
    const bool bHasRightSession = Sessions.ContainsByPredicate([&RightSessionId](const FRIRuntimeSessionInfo& Session)
    {
        return Session.SessionId == RightSessionId;
    });

    FRIRuntimeSessionTargetSetCompareReport CompareReport;
    FString CompareError;
    const bool bCompareOk = bHasLeftSession
        && bHasRightSession
        && CompareRuntimeTargetSetsAcrossSessions(LeftSessionId, RightSessionId, NameFilter, ClassFilter, CompareReport, CompareError);

    const bool bFoundReferenceActor = CompareReport.Lines.ContainsByPredicate([](const FRIRuntimeSessionTargetSetCompareLine& Line)
    {
        return Line.CompareKey.Contains(TEXT("BP_TestVarsActor"), ESearchCase::IgnoreCase);
    });
    const bool bFilterEchoOk = CompareReport.NameFilter == NameFilter && CompareReport.ClassFilter == ClassFilter;
    const bool bScopedLineOk = NameFilter.IsEmpty()
        ? true
        : CompareReport.Lines.ContainsByPredicate([&NameFilter](const FRIRuntimeSessionTargetSetCompareLine& Line)
        {
            return Line.CompareKey.Contains(NameFilter, ESearchCase::IgnoreCase)
                || Line.DisplayLabel.Contains(NameFilter, ESearchCase::IgnoreCase);
        });

    const bool bPassed = bHasLeftSession
        && bHasRightSession
        && bCompareOk
        && CompareReport.LineCount > 0
        && CompareReport.SharedTargetCount > 0
        && bFoundReferenceActor
        && bFilterEchoOk
        && bScopedLineOk;

    OutReport = FString::Printf(
        TEXT("RemoteSessionTargetSetCompareSelfTest=%s | Sessions=%d Left=%s Right=%s Lines=%d Shared=%d LeftOnly=%d RightOnly=%d Mismatch=%d NameFilter=%s ClassFilter=%s BP_TestVarsActor=%s Error=%s"),
        bPassed ? TEXT("PASS") : TEXT("FAIL"),
        Sessions.Num(),
        bHasLeftSession ? *LeftSessionId : TEXT("missing"),
        bHasRightSession ? *RightSessionId : TEXT("missing"),
        CompareReport.LineCount,
        CompareReport.SharedTargetCount,
        CompareReport.LeftOnlyCount,
        CompareReport.RightOnlyCount,
        CompareReport.MismatchCount,
        NameFilter.IsEmpty() ? TEXT("-") : *NameFilter,
        ClassFilter.IsEmpty() ? TEXT("-") : *ClassFilter,
        bFoundReferenceActor ? TEXT("yes") : TEXT("no"),
        CompareError.IsEmpty() ? TEXT("-") : *CompareError);
    return bPassed;
#endif
}

FString UInspectorWorldSubsystem::RunRemoteSessionTargetSetCompareSelfTestSimple()
{
#if !RUNTIME_INSPECTOR_ENABLED
    return TEXT("RuntimeInspector disabled");
#else
    FString Report;
    RunRemoteSessionTargetSetCompareSelfTest(Report);
    return Report;
#endif
}

bool UInspectorWorldSubsystem::RunRemoteSessionTargetSetCompareMatrixSelfTest(FString& OutReport)
{
    OutReport.Reset();

#if !RUNTIME_INSPECTOR_ENABLED
    OutReport = TEXT("RemoteSessionTargetSetCompareMatrixSelfTest=BLOCKED | RuntimeInspector disabled");
    return false;
#else
    if (!IsSelfTestPIEAvailable())
    {
        OutReport = TEXT("RemoteSessionTargetSetCompareMatrixSelfTest=BLOCKED | PIE with local player required");
        return false;
    }

    FRIRuntimeSessionTargetSetCompareMatrixRunResult MatrixResult;
    const bool bMatrixOk = RunRuntimeSessionTargetSetCompareMatrixById(RI_RemoteCompareMatrixId_Default, MatrixResult);
    const bool bEntryCountOk = MatrixResult.EntryResults.Num() >= 2;
    const bool bHasScopedEntry = MatrixResult.EntryResults.ContainsByPredicate([](const FRIRuntimeSessionTargetSetCompareMatrixEntryResult& Entry)
    {
        return Entry.EntryId == TEXT("editor_vs_pie_bp_testvarsactor_scoped")
            && Entry.Request.NameFilter == TEXT("BP_TestVarsActor")
            && Entry.Request.ClassFilter == TEXT("BP_TestVarsActor")
            && Entry.bPassed;
    });
    const bool bHasUnscopedEntry = MatrixResult.EntryResults.ContainsByPredicate([](const FRIRuntimeSessionTargetSetCompareMatrixEntryResult& Entry)
    {
        return Entry.EntryId == TEXT("editor_vs_pie_full_inventory")
            && Entry.Request.NameFilter.IsEmpty()
            && Entry.Request.ClassFilter.IsEmpty()
            && Entry.bPassed;
    });

    const bool bPassed = bMatrixOk && bEntryCountOk && bHasScopedEntry && bHasUnscopedEntry && MatrixResult.FailedEntryCount == 0;
    OutReport = FString::Printf(
        TEXT("RemoteSessionTargetSetCompareMatrixSelfTest=%s | Matrix=%s Entries=%d Passed=%d Failed=%d Scoped=%s Unscoped=%s"),
        bPassed ? TEXT("PASS") : TEXT("FAIL"),
        *MatrixResult.MatrixId.ToString(),
        MatrixResult.EntryResults.Num(),
        MatrixResult.PassedEntryCount,
        MatrixResult.FailedEntryCount,
        bHasScopedEntry ? TEXT("yes") : TEXT("no"),
        bHasUnscopedEntry ? TEXT("yes") : TEXT("no"));
    return bPassed;
#endif
}

FString UInspectorWorldSubsystem::RunRemoteSessionTargetSetCompareMatrixSelfTestSimple()
{
#if !RUNTIME_INSPECTOR_ENABLED
    return TEXT("RuntimeInspector disabled");
#else
    FString Report;
    RunRemoteSessionTargetSetCompareMatrixSelfTest(Report);
    return Report;
#endif
}

bool UInspectorWorldSubsystem::RunRemotePackagedFoundationSelfTest(FString& OutReport)
{
    OutReport.Reset();

#if !RUNTIME_INSPECTOR_ENABLED
    OutReport = TEXT("RemotePackagedFoundationSelfTest=BLOCKED | RuntimeInspector disabled");
    return false;
#elif !WITH_EDITOR
    OutReport = TEXT("RemotePackagedFoundationSelfTest=BLOCKED | Editor authority required");
    return false;
#else
    const FString PreviousConnectedSessionId = ConnectedRuntimeSessionId;
    const FString PreviousRemoteError = LastRemoteRuntimeSessionError;

    auto RestoreState = [this, PreviousConnectedSessionId, PreviousRemoteError]()
    {
        ConnectedRuntimeSessionId = PreviousConnectedSessionId;
        LastRemoteRuntimeSessionError = PreviousRemoteError;
    };

    FRIRuntimeSessionInfo PreferredSession;
    FString ResolveError;
    const bool bSessionResolved = RI_RemoteResolvePreferredExternalPackagedSession(this, PreferredSession, ResolveError);

    FRIRuntimeSessionInfo ConnectedSession;
    FString ConnectError;
    const bool bConnectOk = bSessionResolved && ConnectRemoteRuntimeSession(PreferredSession.SessionId, ConnectedSession, ConnectError);

    TArray<FRIRuntimeTargetInfo> Targets;
    FString ListError;
    const bool bListOk = bConnectOk && ListRuntimeTargetsForSession(
        ConnectedSession.SessionId,
        Targets,
        ListError,
        TEXT("BP_TestVarsActor"),
        TEXT("BP_TestVarsActor"),
        32);

    const bool bProtocolOk = bConnectOk && ConnectedSession.ProtocolVersion == RI_RemoteExpectedExternalProtocolVersion;
    const bool bCapabilityOk = bConnectOk
        && RI_RemoteSessionHasCapability(ConnectedSession, TEXT("loopback-only"))
        && RI_RemoteSessionHasCapability(ConnectedSession, TEXT("runtime-safe"))
        && RI_RemoteSessionHasCapability(ConnectedSession, TEXT("patch-pull"));
    const bool bBuildOk = bConnectOk && !ConnectedSession.BuildConfiguration.Equals(TEXT("Shipping"), ESearchCase::IgnoreCase);
    const bool bTargetOk = bListOk && Targets.ContainsByPredicate([](const FRIRuntimeTargetInfo& Target)
    {
        return Target.ActorLabel.Contains(TEXT("BP_TestVarsActor"), ESearchCase::IgnoreCase)
            || Target.ActorName.Contains(TEXT("BP_TestVarsActor"), ESearchCase::IgnoreCase)
            || Target.ActorClass.Contains(TEXT("BP_TestVarsActor"), ESearchCase::IgnoreCase)
            || Target.ActorClassPath.Contains(TEXT("BP_TestVarsActor"), ESearchCase::IgnoreCase);
    });

    const bool bPassed = bSessionResolved && bConnectOk && bProtocolOk && bCapabilityOk && bBuildOk && bTargetOk;
    OutReport = FString::Printf(
        TEXT("RemotePackagedFoundationSelfTest=%s | Session=%s Connect=%s Build=%s Protocol=%d TargetCount=%d Target=%s Capabilities=%s"),
        bPassed ? TEXT("PASS") : TEXT("FAIL"),
        bSessionResolved ? *PreferredSession.SessionId : TEXT("None"),
        bConnectOk ? TEXT("ok") : *(ConnectError.IsEmpty() ? ResolveError : ConnectError),
        bConnectOk ? *ConnectedSession.BuildConfiguration : TEXT("unknown"),
        bConnectOk ? ConnectedSession.ProtocolVersion : -1,
        Targets.Num(),
        bTargetOk ? TEXT("BP_TestVarsActor") : TEXT("missing"),
        bCapabilityOk ? TEXT("ok") : TEXT("missing"));

    RestoreState();
    return bPassed;
#endif
}

FString UInspectorWorldSubsystem::RunRemotePackagedFoundationSelfTestSimple()
{
#if !RUNTIME_INSPECTOR_ENABLED
    return TEXT("RuntimeInspector disabled");
#else
    FString Report;
    RunRemotePackagedFoundationSelfTest(Report);
    return Report;
#endif
}

bool UInspectorWorldSubsystem::RunRemotePackagedPatchPullSelfTest(FString& OutReport)
{
    OutReport.Reset();

#if !RUNTIME_INSPECTOR_ENABLED
    OutReport = TEXT("RemotePackagedPatchPullSelfTest=BLOCKED | RuntimeInspector disabled");
    return false;
#elif !WITH_EDITOR
    OutReport = TEXT("RemotePackagedPatchPullSelfTest=BLOCKED | Editor authority required");
    return false;
#else
    const bool bHadStagedPatch = bHasStagedPatch;
    const FRIPatchBundle PreviousStagedPatch = StagedPatchBundle;
    const FRIImportReport PreviousImportReport = LastImportReport;
    const FRIAuditReport PreviousAuditReport = LastAuditReport;
    const FRIAuditReport PreviousBaselineAuditReport = CachedBaselineAuditReport;
    const FRIAuditReport PreviousCurrentVsPatchAuditReport = CachedCurrentVsPatchAuditReport;
    const FRIAuditReport PreviousPatchVsSourceAuditReport = CachedPatchVsSourceAuditReport;
    const FRIAuditReport PreviousAppliedPatchVsSourceAuditReport = CachedAppliedPatchVsSourceAuditReport;
    const ERIAuditComparisonMode PreviousActiveAuditMode = ActiveFileAuditMode;
    const FString PreviousConnectedSessionId = ConnectedRuntimeSessionId;
    const FString PreviousRemoteError = LastRemoteRuntimeSessionError;
    const FString PreviousRemotePatchPullSummary = LastRemotePatchPullSummary;
    const FString PreviousRemoteSessionSelectionSummary = LastRemoteSessionSelectionSummary;
    const FString PreviousRemoteSessionTargetQuery = LastRemoteSessionTargetQuery;
    const FString PreviousRemoteSessionWorkflowId = LastRemoteSessionWorkflowId;

    FRIRuntimeSessionInfo PreferredSession;
    FString ResolveError;
    const bool bSessionResolved = RI_RemoteResolvePreferredExternalPackagedSession(this, PreferredSession, ResolveError);

    FRIRuntimeSessionInfo ConnectedSession;
    FString ConnectError;
    const bool bConnectOk = bSessionResolved && ConnectRemoteRuntimeSession(PreferredSession.SessionId, ConnectedSession, ConnectError);

    FString ActorQuery;
    FRIRuntimeTargetInfo Target;
    FString TargetError;
    const bool bTargetOk = bConnectOk && RI_RemoteResolvePackagedTarget(this, ConnectedSession.SessionId, ActorQuery, Target, TargetError);

    FString PullSummary;
    FString PullDetails;
    FString AuditSummary;
    FString AuditDetails;
    FString ClearSummary;
    FString ClearDetails;
    FRIPatchBundle PulledBundle;
    FString RestoreError;
    bool bRestoreOk = false;

    auto RestoreState = [&]()
    {
        if (bTargetOk && PulledBundle.Operations.Num() > 0)
        {
            const FRIPatchOperation& Operation = PulledBundle.Operations[0];
            if (!Operation.BaselineValue.IsEmpty())
            {
                RI_RemoteApplyExternalPropertyText(ConnectedSession, ActorQuery, Operation.Field.FieldPath, Operation.BaselineValue, RestoreError);
            }
        }

        ExecuteFileClearStagedAction(ClearSummary, ClearDetails);

        if (bHadStagedPatch)
        {
            StagedPatchBundle = PreviousStagedPatch;
            bHasStagedPatch = PreviousStagedPatch.Operations.Num() > 0;
        }
        else
        {
            StagedPatchBundle = FRIPatchBundle();
            bHasStagedPatch = false;
        }

        LastImportReport = PreviousImportReport;
        LastAuditReport = PreviousAuditReport;
        CachedBaselineAuditReport = PreviousBaselineAuditReport;
        CachedCurrentVsPatchAuditReport = PreviousCurrentVsPatchAuditReport;
        CachedPatchVsSourceAuditReport = PreviousPatchVsSourceAuditReport;
        CachedAppliedPatchVsSourceAuditReport = PreviousAppliedPatchVsSourceAuditReport;
        ActiveFileAuditMode = PreviousActiveAuditMode;
        ConnectedRuntimeSessionId = PreviousConnectedSessionId;
        LastRemoteRuntimeSessionError = PreviousRemoteError;
        LastRemotePatchPullSummary = PreviousRemotePatchPullSummary;
        LastRemoteSessionSelectionSummary = PreviousRemoteSessionSelectionSummary;
        LastRemoteSessionTargetQuery = PreviousRemoteSessionTargetQuery;
        LastRemoteSessionWorkflowId = PreviousRemoteSessionWorkflowId;
    };

    const bool bApplyOk = bTargetOk
        && RI_RemoteApplyExternalPropertyText(ConnectedSession, ActorQuery, TEXT("InitialLifeSpan"), TEXT("0.15"), RestoreError);

    const bool bPullOk = bApplyOk
        && ExecuteFilePullPatchFromRemoteSessionAction(ConnectedSession.SessionId, ActorQuery, PullSummary, PullDetails);
    PulledBundle = bPullOk ? GetStagedPatch() : FRIPatchBundle();

    const bool bBundleOk = PulledBundle.Operations.Num() > 0
        && PulledBundle.Operations.ContainsByPredicate([](const FRIPatchOperation& Operation)
        {
            return Operation.Field.FieldPath == TEXT("InitialLifeSpan");
        });

    const bool bAuditOk = bBundleOk && ExecuteFileBuildPatchVsSourceAuditAction(AuditSummary, AuditDetails);
    const FRIAuditReport AuditReport = LastAuditReport;
    const bool bAuditLinesOk = bAuditOk && AuditReport.Mode == ERIAuditComparisonMode::PatchVsSource && AuditReport.Lines.Num() > 0;

    if (bBundleOk)
    {
        const FRIPatchOperation* RestoreOperation = PulledBundle.Operations.FindByPredicate([](const FRIPatchOperation& Operation)
        {
            return Operation.Field.FieldPath == TEXT("InitialLifeSpan");
        });
        if (RestoreOperation && !RestoreOperation->BaselineValue.IsEmpty())
        {
            bRestoreOk = RI_RemoteApplyExternalPropertyText(
                ConnectedSession,
                ActorQuery,
                RestoreOperation->Field.FieldPath,
                RestoreOperation->BaselineValue,
                RestoreError);
        }
    }

    const bool bClearOk = ExecuteFileClearStagedAction(ClearSummary, ClearDetails);
    const bool bPassed = bSessionResolved
        && bConnectOk
        && bTargetOk
        && bApplyOk
        && bPullOk
        && bBundleOk
        && bAuditLinesOk
        && bRestoreOk
        && bClearOk
        && !HasStagedPatch();

    OutReport = FString::Printf(
        TEXT("RemotePackagedPatchPullSelfTest=%s | Session=%s Target=%s Apply=%s Pull=%s Ops=%d Audit=%s Restore=%s Clear=%s"),
        bPassed ? TEXT("PASS") : TEXT("FAIL"),
        bSessionResolved ? *PreferredSession.SessionId : TEXT("None"),
        bTargetOk ? *ActorQuery : *(TargetError.IsEmpty() ? TEXT("missing") : TargetError),
        bApplyOk ? TEXT("ok") : *RestoreError,
        bPullOk ? TEXT("ok") : *PullSummary,
        PulledBundle.Operations.Num(),
        bAuditLinesOk ? TEXT("ok") : *AuditSummary,
        bRestoreOk ? TEXT("ok") : *(RestoreError.IsEmpty() ? TEXT("restore-failed") : RestoreError),
        bClearOk ? TEXT("ok") : *ClearSummary);

    RestoreState();
    return bPassed;
#endif
}

FString UInspectorWorldSubsystem::RunRemotePackagedPatchPullSelfTestSimple()
{
#if !RUNTIME_INSPECTOR_ENABLED
    return TEXT("RuntimeInspector disabled");
#else
    FString Report;
    RunRemotePackagedPatchPullSelfTest(Report);
    return Report;
#endif
}

bool UInspectorWorldSubsystem::RunRemotePackagedToSourceClosureSelfTest(FString& OutReport)
{
    OutReport.Reset();

#if !RUNTIME_INSPECTOR_ENABLED
    OutReport = TEXT("RemotePackagedToSourceClosureSelfTest=BLOCKED | RuntimeInspector disabled");
    return false;
#elif !WITH_EDITOR
    OutReport = TEXT("RemotePackagedToSourceClosureSelfTest=BLOCKED | Editor authority required");
    return false;
#else
    UBlueprint* Blueprint = nullptr;
    UClass* BlueprintClass = nullptr;
    UObject* SourceObject = nullptr;
    if (!RI_RemoteLoadPackagedSelfTestBlueprint(Blueprint))
    {
        OutReport = TEXT("RemotePackagedToSourceClosureSelfTest=FAIL | Test Blueprint asset unavailable");
        return false;
    }
    BlueprintClass = Cast<UClass>(Blueprint->GeneratedClass);
    SourceObject = BlueprintClass ? BlueprintClass->GetDefaultObject() : nullptr;
    if (!BlueprintClass || !SourceObject)
    {
        OutReport = TEXT("RemotePackagedToSourceClosureSelfTest=FAIL | Test Blueprint asset unavailable");
        return false;
    }

    FString SourceOriginalText;
    if (!InspectorPropertyUtils::GetValueAsText(SourceObject, TEXT("InitialLifeSpan"), SourceOriginalText))
    {
        OutReport = TEXT("RemotePackagedToSourceClosureSelfTest=FAIL | Failed to read Blueprint source default");
        return false;
    }

    const bool bHadStagedPatch = bHasStagedPatch;
    const FRIPatchBundle PreviousStagedPatch = StagedPatchBundle;
    const FRIImportReport PreviousImportReport = LastImportReport;
    const FRIAuditReport PreviousAuditReport = LastAuditReport;
    const FRIAuditReport PreviousBaselineAuditReport = CachedBaselineAuditReport;
    const FRIAuditReport PreviousCurrentVsPatchAuditReport = CachedCurrentVsPatchAuditReport;
    const FRIAuditReport PreviousPatchVsSourceAuditReport = CachedPatchVsSourceAuditReport;
    const FRIAuditReport PreviousAppliedPatchVsSourceAuditReport = CachedAppliedPatchVsSourceAuditReport;
    const ERIAuditComparisonMode PreviousActiveAuditMode = ActiveFileAuditMode;
    const FRIPromoteResult PreviousPromoteResult = LastPromoteResult;
    const FString PreviousConnectedSessionId = ConnectedRuntimeSessionId;
    const FString PreviousRemoteError = LastRemoteRuntimeSessionError;
    const FString PreviousRemotePatchPullSummary = LastRemotePatchPullSummary;
    const FString PreviousRemoteSessionSelectionSummary = LastRemoteSessionSelectionSummary;
    const FString PreviousRemoteSessionTargetQuery = LastRemoteSessionTargetQuery;
    const FString PreviousRemoteSessionWorkflowId = LastRemoteSessionWorkflowId;

    FRIRuntimeSessionInfo PreferredSession;
    FString ResolveError;
    const bool bSessionResolved = RI_RemoteResolvePreferredExternalPackagedSession(this, PreferredSession, ResolveError);

    FRIRuntimeSessionInfo ConnectedSession;
    FString ConnectError;
    const bool bConnectOk = bSessionResolved && ConnectRemoteRuntimeSession(PreferredSession.SessionId, ConnectedSession, ConnectError);

    FString ActorQuery;
    FRIRuntimeTargetInfo Target;
    FString TargetError;
    const bool bTargetOk = bConnectOk && RI_RemoteResolvePackagedTarget(this, ConnectedSession.SessionId, ActorQuery, Target, TargetError);

    FString PullSummary;
    FString PullDetails;
    FString AuditSummary;
    FString AuditDetails;
    FString PreviewSummary;
    FString PreviewDetails;
    FString ApplySummary;
    FString ApplyDetails;
    FString ClearSummary;
    FString ClearDetails;
    FString RestoreError;
    FRIPatchBundle PulledBundle;
    FRIPatchBundle SourceRestoreBundle;
    bool bSourceRestoreOk = false;
    bool bRemoteRestoreOk = false;

    auto RestoreState = [&]()
    {
        if (bSourceRestoreOk)
        {
            // already restored below
        }
        else if (SourceRestoreBundle.Operations.Num() > 0)
        {
            FRIPromoteResult RestorePromoteResult;
            FString RestorePromoteError;
            PromotePatchToSource(SourceRestoreBundle, RestorePromoteResult, RestorePromoteError);
        }

        if (bTargetOk && PulledBundle.Operations.Num() > 0)
        {
            const FRIPatchOperation* RestoreOperation = PulledBundle.Operations.FindByPredicate([](const FRIPatchOperation& Operation)
            {
                return Operation.Field.FieldPath == TEXT("InitialLifeSpan");
            });
            if (RestoreOperation && !RestoreOperation->BaselineValue.IsEmpty())
            {
                RI_RemoteApplyExternalPropertyText(
                    ConnectedSession,
                    ActorQuery,
                    RestoreOperation->Field.FieldPath,
                    RestoreOperation->BaselineValue,
                    RestoreError);
            }
        }

        ExecuteFileClearStagedAction(ClearSummary, ClearDetails);

        if (bHadStagedPatch)
        {
            StagedPatchBundle = PreviousStagedPatch;
            bHasStagedPatch = PreviousStagedPatch.Operations.Num() > 0;
        }
        else
        {
            StagedPatchBundle = FRIPatchBundle();
            bHasStagedPatch = false;
        }

        LastImportReport = PreviousImportReport;
        LastAuditReport = PreviousAuditReport;
        CachedBaselineAuditReport = PreviousBaselineAuditReport;
        CachedCurrentVsPatchAuditReport = PreviousCurrentVsPatchAuditReport;
        CachedPatchVsSourceAuditReport = PreviousPatchVsSourceAuditReport;
        CachedAppliedPatchVsSourceAuditReport = PreviousAppliedPatchVsSourceAuditReport;
        ActiveFileAuditMode = PreviousActiveAuditMode;
        LastPromoteResult = PreviousPromoteResult;
        ConnectedRuntimeSessionId = PreviousConnectedSessionId;
        LastRemoteRuntimeSessionError = PreviousRemoteError;
        LastRemotePatchPullSummary = PreviousRemotePatchPullSummary;
        LastRemoteSessionSelectionSummary = PreviousRemoteSessionSelectionSummary;
        LastRemoteSessionTargetQuery = PreviousRemoteSessionTargetQuery;
        LastRemoteSessionWorkflowId = PreviousRemoteSessionWorkflowId;
    };

    const bool bApplyRemoteOk = bTargetOk
        && RI_RemoteApplyExternalPropertyText(ConnectedSession, ActorQuery, TEXT("InitialLifeSpan"), TEXT("0.15"), RestoreError);

    const bool bPullOk = bApplyRemoteOk
        && ExecuteFilePullPatchFromRemoteSessionAction(ConnectedSession.SessionId, ActorQuery, PullSummary, PullDetails);
    PulledBundle = bPullOk ? GetStagedPatch() : FRIPatchBundle();

    if (PulledBundle.Operations.Num() > 0)
    {
        RI_RemoteInjectBlueprintSourceTag(PulledBundle, Blueprint->GetPathName());
        StagedPatchBundle = PulledBundle;
        bHasStagedPatch = true;
    }

    const FRIPatchOperation* PrimaryOperation = PulledBundle.Operations.FindByPredicate([](const FRIPatchOperation& Operation)
    {
        return Operation.Field.FieldPath == TEXT("InitialLifeSpan");
    });
    const bool bBundleOk = PrimaryOperation != nullptr;

    const bool bAuditOk = bBundleOk && ExecuteFileBuildPatchVsSourceAuditAction(AuditSummary, AuditDetails);
    const bool bPreviewOk = bBundleOk && ExecuteFilePromotePreviewAction(PreviewSummary, PreviewDetails);
    const bool bApplyOk = bBundleOk && ExecuteFilePromoteApplyAction(ApplySummary, ApplyDetails);

    FString AfterPromoteText;
    SourceObject = BlueprintClass->GetDefaultObject();
    const bool bReadAfterPromote = SourceObject && InspectorPropertyUtils::GetValueAsText(SourceObject, TEXT("InitialLifeSpan"), AfterPromoteText);
    const bool bPromoteValueOk = bReadAfterPromote
        && PrimaryOperation
        && FMath::IsNearlyEqual(FCString::Atof(*AfterPromoteText), FCString::Atof(*PrimaryOperation->PatchedValue), 0.001f);

    if (bBundleOk)
    {
        bSourceRestoreOk = RI_RemoteBuildRestoreBundle(PulledBundle, SourceRestoreBundle);
        if (bSourceRestoreOk)
        {
            RI_RemoteInjectBlueprintSourceTag(SourceRestoreBundle, Blueprint->GetPathName());
            FRIPromoteResult RestorePromoteResult;
            FString RestorePromoteError;
            bSourceRestoreOk = PromotePatchToSource(SourceRestoreBundle, RestorePromoteResult, RestorePromoteError);
            if (!bSourceRestoreOk && RestoreError.IsEmpty())
            {
                RestoreError = RestorePromoteError;
            }
        }
    }

    FString AfterRestoreText;
    SourceObject = BlueprintClass->GetDefaultObject();
    const bool bReadAfterRestore = SourceObject && InspectorPropertyUtils::GetValueAsText(SourceObject, TEXT("InitialLifeSpan"), AfterRestoreText);
    const bool bSourceRestoredValueOk = bReadAfterRestore
        && FMath::IsNearlyEqual(FCString::Atof(*AfterRestoreText), FCString::Atof(*SourceOriginalText), 0.001f);

    if (PrimaryOperation && !PrimaryOperation->BaselineValue.IsEmpty())
    {
        bRemoteRestoreOk = RI_RemoteApplyExternalPropertyText(
            ConnectedSession,
            ActorQuery,
            PrimaryOperation->Field.FieldPath,
            PrimaryOperation->BaselineValue,
            RestoreError);
    }

    const bool bClearOk = ExecuteFileClearStagedAction(ClearSummary, ClearDetails);
    const bool bPassed = bSessionResolved
        && bConnectOk
        && bTargetOk
        && bApplyRemoteOk
        && bPullOk
        && bBundleOk
        && bAuditOk
        && bPreviewOk
        && bApplyOk
        && bPromoteValueOk
        && bSourceRestoreOk
        && bSourceRestoredValueOk
        && bRemoteRestoreOk
        && bClearOk;

    OutReport = FString::Printf(
        TEXT("RemotePackagedToSourceClosureSelfTest=%s | Session=%s Target=%s Pull=%s Audit=%s Preview=%s Apply=%s AfterPromote=%s SourceRestore=%s RemoteRestore=%s Clear=%s"),
        bPassed ? TEXT("PASS") : TEXT("FAIL"),
        bSessionResolved ? *PreferredSession.SessionId : TEXT("None"),
        bTargetOk ? *ActorQuery : *(TargetError.IsEmpty() ? TEXT("missing") : TargetError),
        bPullOk ? TEXT("ok") : *PullSummary,
        bAuditOk ? TEXT("ok") : *AuditSummary,
        bPreviewOk ? TEXT("ok") : *PreviewSummary,
        bApplyOk ? TEXT("ok") : *ApplySummary,
        bPromoteValueOk ? *AfterPromoteText : TEXT("mismatch"),
        bSourceRestoredValueOk ? TEXT("ok") : *AfterRestoreText,
        bRemoteRestoreOk ? TEXT("ok") : *(RestoreError.IsEmpty() ? TEXT("restore-failed") : RestoreError),
        bClearOk ? TEXT("ok") : *ClearSummary);

    RestoreState();
    return bPassed;
#endif
}

FString UInspectorWorldSubsystem::RunRemotePackagedToSourceClosureSelfTestSimple()
{
#if !RUNTIME_INSPECTOR_ENABLED
    return TEXT("RuntimeInspector disabled");
#else
    FString Report;
    RunRemotePackagedToSourceClosureSelfTest(Report);
    return Report;
#endif
}
