#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include "ChatPacket.h"
#include "MovePacket.h"
#include "NetUtil.h"

#include <winsock2.h>
#include <Windows.h>
#include <iostream>
#include <process.h>
#include <conio.h>

#pragma comment(lib, "ws2_32")
#pragma comment(lib, "NetCommon")

using namespace std;

char RecvBuffer[1024] = { 0, };

bool IsRecvThreadRunning = true;
bool IsSendThreadRunning = true;


CRITICAL_SECTION SendCrypto;

unsigned WINAPI RecvThread(void* Argument)
{
	SOCKET ServerSocket = *(SOCKET*)Argument;

	while (IsRecvThreadRunning)
	{
		unsigned short PacketSize = 0;

		
		int RecvBytes = recv(ServerSocket, (char*)&PacketSize, sizeof(PacketSize), MSG_WAITALL);
		if (RecvBytes <= 0)
		{
			cout << "recv fail " << endl;
			break;
		}

		PacketSize = ntohs(PacketSize);
		if (PacketSize <= 0 || PacketSize >= 1024) continue;

		memset(RecvBuffer, 0, sizeof(RecvBuffer));

		
		RecvBytes = recv(ServerSocket, RecvBuffer, PacketSize, MSG_WAITALL);
		if (RecvBytes <= 0)
		{
			cout << "recv fail " << endl;
			break;
		}

		rapidjson::Document Doc;
		Doc.Parse(RecvBuffer);
		if (Doc.HasParseError() || !Doc.HasMember("Type")) continue;

		std::string Type = Doc["Type"].GetString();

		if (Type == "Move")
		{
			MovePacket Data;
			Data.Parse(RecvBuffer);
			cout << "\n" << Data.UserID << " 이동: (" << Data.X << ", " << Data.Y << ")" << endl;
		}
		else if (Type == "Chat")
		{
			ChatPacket Data;
			Data.Parse(RecvBuffer);
			cout << "\n" << Data.UserID << " : " << Data.Message << " " << Data.Gold << endl;
		}
	}

	return 0;
}

unsigned WINAPI InputThread(void* Argument)
{
	SOCKET ServerSocket = *(SOCKET*)Argument;

	int X = 0;
	int Y = 0;
	char CombinedBuffer[1024] = { 0, };

	while (IsSendThreadRunning)
	{
		
		cin.getline(CombinedBuffer, sizeof(CombinedBuffer));

		std::string InputStr = CombinedBuffer;
		if (InputStr.empty()) continue;

		
		bool IsMoveCommand = true;
		for (char ch : InputStr)
		{
			if (ch != 'w' && ch != 'W' && ch != 'a' && ch != 'A' &&
				ch != 's' && ch != 'S' && ch != 'd' && ch != 'D')
			{
				IsMoveCommand = false;
				break;
			}
		}
		
		if (IsMoveCommand && InputStr.length() == 1)
		{
			bool IsMoved = false;
			for (size_t i = 0; i < InputStr.length(); ++i)
			{
				char Ch = InputStr[i];

				if (Ch == 'w' || Ch == 'W') { Y -= 1; IsMoved = true;}
				else if (Ch == 's' || Ch == 'S') { Y += 1; IsMoved = true;}
				else if (Ch == 'a' || Ch == 'A') { X -= 1; IsMoved = true;}
				else if (Ch == 'd' || Ch == 'D') { X += 1; IsMoved = true; }
				
				
				MovePacket Move;
				Move.Type = "Move";
				Move.UserID = "aaaaaa";
				Move.Key = std::string(1, toupper(Ch));
				Move.X = X;
				Move.Y = Y;

				std::string JSONString = Move.ToString();
				unsigned short PacketSize = (unsigned short)JSONString.length();
				PacketSize = htons(PacketSize);

				
				EnterCriticalSection(&SendCrypto);
				SendAll(ServerSocket, (char*)&PacketSize, 2);
				SendAll(ServerSocket, JSONString.c_str(), ntohs(PacketSize));
				LeaveCriticalSection(&SendCrypto);

			
			}

			
		}
		
		else
		{
			ChatPacket Data;
			Data.Type = "Chat";
			Data.UserID = "aaaaaa";
			Data.Message = InputStr;
			Data.Gold = 1000;

			std::string JSONString = Data.ToString();
			unsigned short PacketSize = (unsigned short)JSONString.length();
			PacketSize = htons(PacketSize);

			EnterCriticalSection(&SendCrypto);
			SendAll(ServerSocket, (char*)&PacketSize, 2);
			SendAll(ServerSocket, JSONString.c_str(), ntohs(PacketSize));
			LeaveCriticalSection(&SendCrypto);
		}
	}

	return 0;
}

int main()
{
	cout << "client start" << endl;

	
	InitializeCriticalSection(&SendCrypto);

	WSAData wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);

	SOCKET ServerSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

	SOCKADDR_IN ServerSockAddr;
	memset(&ServerSockAddr, 0, sizeof(ServerSockAddr));
	ServerSockAddr.sin_family = AF_INET;
	ServerSockAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	ServerSockAddr.sin_port = htons(35000);

	if (connect(ServerSocket, (SOCKADDR*)&ServerSockAddr, sizeof(ServerSockAddr)) == SOCKET_ERROR)
	{
		cout << "서버 연결 실패" << endl;
		DeleteCriticalSection(&SendCrypto);
		WSACleanup();
		return 0;
	}

	cout << "client connect success" << endl;

	
	HANDLE ThreadHandles[2] = { 0, };

	ThreadHandles[0] = (HANDLE)_beginthreadex(0, 0, RecvThread, &ServerSocket, 0, 0);
	ThreadHandles[1] = (HANDLE)_beginthreadex(0, 0, InputThread, &ServerSocket, 0, 0);

	
	WaitForMultipleObjects(2, ThreadHandles, FALSE, INFINITE);

	closesocket(ServerSocket);

	IsSendThreadRunning = false;
	IsRecvThreadRunning = false;

	CloseHandle(ThreadHandles[0]);
	CloseHandle(ThreadHandles[1]);

	
	DeleteCriticalSection(&SendCrypto);
	WSACleanup();

	return 0;
}