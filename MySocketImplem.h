// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/Pawn.h"
#include <iostream>
#include <string>
#include "Runtime/Sockets/Public/Sockets.h"
#include "Runtime/Sockets/Public/SocketSubsystem.h"
#include "Runtime/Networking/Public/Networking.h"
#include "MySocketImplem.generated.h"

typedef struct	Fs_SocketContent
{
	FSocket*	ConnectSocket;
	FIPv4Endpoint	Address;
	FString		Buffer;
	bool		NewMessage;
}				FSocketContent;

UENUM()
enum FCommand
{
	PAUSE = 0,
	UNPAUSE = 1,
	MOVE = 2,
	SAY = 3,
	QUIT = 4,
	UNKNOWN
};

UCLASS()
class SOCKETIMPLEM_API AMySocketImplem : public APawn
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Container")
	TArray<FString> BufferList;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Notifyer")
	bool	NewMessage;
	TArray<FSocketContent>	SocketList;
	FSocket*	ListenerSocket;
	FSocket*	ConnectSocket;
	FIPv4Endpoint	RemoteAddress;

	bool	FormatIP4ToNumber(const FString &TheIp, uint8(&Out)[4]);
	FSocket*	CreateTCPConnectionListener(const FString &SocketName, const FString &TheIp, uint32 ThePort, uint32 BufferSize = 2*1024*1024);
	bool	StartTCPReceiver(const FString &SocketName, const FString &TheIp, uint32 ThePort);
	void	TCPConnectionListener();
	FString	StringFromBinaryArray(const TArray<uint8> &BinArray);
	FString	GetCommandWithoutID(const FString &FullMessage);

	// Sets default values for this pawn's properties
	AMySocketImplem();

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	// Called when the game ends
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;

	// Called every frame
	virtual void Tick( float DeltaSeconds ) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* InputComponent) override;

	UFUNCTION(BlueprintCallable, Category = "Online")
		bool	SendStringToClient(int32 ClientID, const FString MessageToSend);
	UFUNCTION(BlueprintCallable, Category = "Online")
		int32	GetClientIDFromLastMessage(const FString LastMessage);
	UFUNCTION(BlueprintCallable, Category = "Parser")
		bool	AskPause(const FString LastMessage);
	UFUNCTION(BlueprintCallable, Category = "Init")
		bool	CreateSocketConnection(const FString &SocketName, const FString &TheIP, int32 ThePort);
	UFUNCTION(BlueprintCallable, Category = "Parser")
		FCommand	ParseReceivedMessage(const FString &Message);
	UFUNCTION(BlueprintCallable, Category = "Online")
		void	TCPSocketListener();

};
