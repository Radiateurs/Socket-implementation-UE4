// Fill out your copyright notice in the Description page of Project Settings.

#include "SocketImplem.h"
#include "MySocketImplem.h"
#include "Runtime/Networking/Public/Interfaces/IPv4/IPv4Endpoint.h"

// Sets default values
AMySocketImplem::AMySocketImplem()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	NewMessage = false;
	BufferList.Init(FString(), 0);
}

// Called when the game starts or when spawned
void AMySocketImplem::BeginPlay()
{
	Super::BeginPlay();
}

// Called when the game ends
void	AMySocketImplem::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	int32	i = 0;

	while (i < SocketList.Num())
	{
		if (SocketList[i].ConnectSocket)
		{
			SocketList[i].ConnectSocket->Close();
			ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(SocketList[i].ConnectSocket);
		}
		i++;
	}
	if (ListenerSocket)
	{
		ListenerSocket->Close();
		ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(ListenerSocket);
	}
	return ;
}

// Called every frame
void AMySocketImplem::Tick( float DeltaTime )
{
	Super::Tick( DeltaTime );
}

// Called to bind functionality to input
void AMySocketImplem::SetupPlayerInputComponent(class UInputComponent* inputComponent)
{
	Super::SetupPlayerInputComponent(inputComponent);

}

bool	AMySocketImplem::StartTCPReceiver(const FString &SocketName, const FString &TheIp, uint32 ThePort)
{
	struct FTimerHandle	TimeMaster;

	ListenerSocket = CreateTCPConnectionListener(SocketName, TheIp, ThePort);
	if (!ListenerSocket)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("StartTCPReceiver>> Listen socket could not be created! ~> %s %d"), *TheIp, ThePort));
		return false;
	}
	GetWorldTimerManager().SetTimer(TimeMaster, this, &AMySocketImplem::TCPConnectionListener, 0.01, true);
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, "Socket Created");
	return true;
}

void	AMySocketImplem::TCPConnectionListener()
{
	struct FTimerHandle	TimeMaster;
	FSocketContent	tmp;

	if (!ListenerSocket)
		return;
	TSharedRef<FInternetAddr> TheRemoteAddress = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
	bool Pending;

	if (ListenerSocket->HasPendingConnection(Pending) && Pending)
	{
		tmp.ConnectSocket = ListenerSocket->Accept(*TheRemoteAddress, TEXT("RamaTCP Received Socket Connection"));
		if (tmp.ConnectSocket != NULL)
		{
			tmp.Address = FIPv4Endpoint(TheRemoteAddress);
			GetWorldTimerManager().SetTimer(TimeMaster, this, &AMySocketImplem::TCPSocketListener, 0.01, true);
			SocketList.Add(FSocketContent(tmp));
		}
	}
}

void		AMySocketImplem::TCPSocketListener()
{
	TArray<uint8> ReceivedData;
	int32 Read = 0;
	int32 i = 0;

	while (i < SocketList.Num())
	{
		uint32 Size;
		bool received = false;
		while (SocketList[i].ConnectSocket && SocketList[i].ConnectSocket->HasPendingData(Size))
		{
			Read = 0;
			ReceivedData.Init(FMath::Min(Size, 65507u), Size);
			SocketList[i].ConnectSocket->Recv(ReceivedData.GetData(), ReceivedData.Num(), Read);
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("Received data from %i! Nb of data readed : %d"), i, Read));
			SocketList[i].Buffer += StringFromBinaryArray(ReceivedData);
			NewMessage = true;
			received = true;
		}
		if (received)
		{
			BufferList.Add(FString::FromInt(i).Append(FString(". ").Append(SocketList[i].Buffer)));
			received = false;
		}
		i++;
	}
}

FSocket*	AMySocketImplem::CreateTCPConnectionListener(const FString &SocketName, const FString &TheIP, uint32 ThePort, uint32 BufferSize)
{
	uint8 IP4Nums[4];
	if (!FormatIP4ToNumber(TheIP, IP4Nums))
	{
		return false;
	}
	FIPv4Endpoint Endpoint(FIPv4Address(IP4Nums[0], IP4Nums[1], IP4Nums[2], IP4Nums[3]), ThePort);
	FSocket* ListenSocket = FTcpSocketBuilder(*SocketName)
		.AsReusable()
		.BoundToEndpoint(Endpoint)
		.Listening(8);

	int32 NewSize = 0;
	ListenSocket->SetReceiveBufferSize(BufferSize, NewSize);

	//Done!
	return ListenSocket;
}

bool AMySocketImplem::FormatIP4ToNumber(const FString& TheIP, uint8(&Out)[4])
{
	//IP Formatting
	TheIP.Replace(TEXT(" "), TEXT(""));

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	//						   IP 4 Parts

	//String Parts
	TArray<FString> Parts;
	TheIP.ParseIntoArray(Parts, TEXT("."), true);
	if (Parts.Num() != 4)
		return false;

	//String to Number Parts
	for (int32 i = 0; i < 4; ++i)
	{
		Out[i] = FCString::Atoi(*Parts[i]);
	}

	return true;
}

FString AMySocketImplem::StringFromBinaryArray(const TArray<uint8>& BinaryArray)
{
	//Create a string from a byte array!
	const std::string cstr(reinterpret_cast<const char*>(BinaryArray.GetData()), BinaryArray.Num());

	//FString can take in the c_str() of a std::string
	return FString(cstr.c_str());
}

bool AMySocketImplem::SendStringToClient(int32 ClientID, const FString MessageToSend)
{
	TArray<uint8> ConvertedData;
	int32 i = 0;

	while (i < MessageToSend.Len())
	{
		ConvertedData.Add((uint8)MessageToSend[i]);
		i++;
	}
	if (SocketList[ClientID].ConnectSocket)
	{
		SocketList[ClientID].ConnectSocket->Send(ConvertedData.GetData(), ConvertedData.Num(), i);
		return true;
	}
	return false;
}

int32 AMySocketImplem::GetClientIDFromLastMessage(const FString LastMessage)
{
	int32	i = 0;
	FString	tmp = LastMessage;

	while (i < LastMessage.Len())
	{
		if (LastMessage[i] == '.' && LastMessage[i + 1] == ' ')
			tmp[i] = '\0';
		i++;
	}
	return FCString::Atoi(*tmp);
}

bool AMySocketImplem::AskPause(const FString LastMessage)
{
	int32	i = 0;
	int32	j = 0;
	FString tmp;

	while (i < LastMessage.Len() && (LastMessage[i] != '.' && LastMessage[i + 1] != ' '))
		i++;
	i += 2;
	tmp = LastMessage;
	while (j + i < LastMessage.Len())
	{
		tmp[j] = LastMessage[j + i];
		j++;
	}
	tmp[j] = '\0';
	if (tmp == "PAUSE\n")
		return true;
	return false;
}

bool AMySocketImplem::CreateSocketConnection(const FString & SocketName, const FString & TheIP, int32 ThePort)
{
	if (StartTCPReceiver(SocketName, TheIP, ThePort))
		return true;
	return false;
}

FString AMySocketImplem::GetCommandWithoutID(const FString& FullMessage)
{
	FString tmp;
	int32	i = 0;
	int32	j = 0;

	while (i < FullMessage.Len() && (FullMessage[i] != '.' && FullMessage[i + 1] != ' '))
		i++;
	i += 2;
	tmp = FullMessage;
	while (j + i < FullMessage.Len())
	{
		tmp[j] = FullMessage[j + i];
		j++;
	}
	tmp[j] = '\0';
	return (FString(tmp));
}

FCommand AMySocketImplem::ParseReceivedMessage(const FString& Message)
{
	FCommand MessageType = UNKNOWN;
	TArray<FString>	CommandList = { "PAUSE\n", "UNPAUSE\n", "MOVE\n", "SAY ", "QUIT\n" };
	FString cmd = GetCommandWithoutID(Message);
	int32	i = 0;

	while (i < CommandList.Num())
	{
		int32	j = 0;

		while (j < CommandList[i].Len() && j < cmd.Len() && CommandList[i][j] == cmd[j])
		{
			j++;
		}
		if (j > 0 && (CommandList[i][j - 1] == '\n' || CommandList[i][j - 1] == ' '))
			return ((FCommand)i);
		i++;
	}
	return (UNKNOWN);
}
