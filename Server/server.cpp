#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include "NetUtil.h"

#include <winsock2.h>
#include <iostream>


#pragma comment(lib, "ws2_32")
#pragma comment(lib, "NetCommon")

using namespace std;

char Buffer[1024] = { 0, };


//blocking, synchrous, multiplexing(polling)
int main()
{
	cout << "server start" << endl;

	WSAData wsaData;

	WSAStartup(MAKEWORD(2, 2), &wsaData);

	SOCKET ListenSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

	SOCKADDR_IN ListenSockAddr;
	memset(&ListenSockAddr, 0, sizeof(ListenSockAddr));
	ListenSockAddr.sin_family = AF_INET;
	ListenSockAddr.sin_addr.s_addr = INADDR_ANY;
	ListenSockAddr.sin_port = htons(35000);

	//already use port 이미 포트 사용중
	::bind(ListenSocket, (SOCKADDR*)&ListenSockAddr, sizeof(ListenSockAddr));

	listen(ListenSocket, SOMAXCONN);



	//blocking, synchronous(TimeOut)
	TIMEVAL TimeOut;
	TimeOut.tv_sec = 0;
	TimeOut.tv_usec = 500000;

	fd_set ReadSockets;
	fd_set CopyReadSockets;

	FD_ZERO(&ReadSockets);
	FD_SET(ListenSocket, &ReadSockets);

	while (true)
	{
		CopyReadSockets = ReadSockets;

		//0.5초씩 blocking
		int ChangeCount = select(0, &CopyReadSockets, 0, 0, &TimeOut);

		if (ChangeCount <= 0)
		{
			//Server Work
			//0.5초한번 서버 작업을 하는거
			continue;
		}

		//몬가 자료 있다.
		for (int i = 0; i < (int)ReadSockets.fd_count; ++i)
		{
			if (FD_ISSET(ReadSockets.fd_array[i], &CopyReadSockets))
			{
				if (ReadSockets.fd_array[i] == ListenSocket)
				{
					//connect process
					SOCKADDR_IN ClientSockAddr;
					memset(&ClientSockAddr, 0, sizeof(ClientSockAddr));
					int ClientSockSockLength = sizeof(ClientSockAddr);

					//blocking, synchronous
					SOCKET ClientSocket = accept(ListenSocket, (SOCKADDR*)&ClientSockAddr, &ClientSockSockLength);

					cout << "connect client " << inet_ntoa(ClientSockAddr.sin_addr) << endl;

					FD_SET(ClientSocket, &ReadSockets);
				}
				else
				{
					//Data Receive

					//header
					unsigned short PacketSize = 0;
					int RecvBytes = recv(ReadSockets.fd_array[i], (char*)&PacketSize, sizeof(PacketSize), MSG_WAITALL);
					if (RecvBytes <= 0)
					{
						cout << "header recv fail " << endl;
						SOCKADDR_IN ClosedSockAddr;
						memset(&ClosedSockAddr, 0, sizeof(ClosedSockAddr));
						int ClosedSockAddrLength = sizeof(ClosedSockAddr);

						SOCKET ClosedSocket = ReadSockets.fd_array[i];
						getpeername(ClosedSocket, (SOCKADDR*)&ClosedSockAddr, &ClosedSockAddrLength);
						cout << "disconnect client " << inet_ntoa(ClosedSockAddr.sin_addr) << endl;
						FD_CLR(ReadSockets.fd_array[i], &ReadSockets);
						closesocket(ClosedSocket);
						continue;
					}

					PacketSize = ntohs(PacketSize);

					memset(Buffer, 0, sizeof(Buffer));
					//data JSON
					RecvBytes = recv(ReadSockets.fd_array[i], Buffer, PacketSize, MSG_WAITALL);
					if (RecvBytes <= 0)
					{
						cout << "data recv fail " << endl;
						SOCKADDR_IN ClosedSockAddr;
						memset(&ClosedSockAddr, 0, sizeof(ClosedSockAddr));
						int ClosedSockAddrLength = sizeof(ClosedSockAddr);

						SOCKET ClosedSocket = ReadSockets.fd_array[i];
						getpeername(ClosedSocket, (SOCKADDR*)&ClosedSockAddr, &ClosedSockAddrLength);
						cout << "disconnect client " << inet_ntoa(ClosedSockAddr.sin_addr) << endl;
						FD_CLR(ReadSockets.fd_array[i], &ReadSockets);
						closesocket(ClosedSocket);
						continue;
					}
					else
					{
						SOCKADDR_IN ClientSockAddr;
						memset(&ClientSockAddr, 0, sizeof(ClientSockAddr));
						int ClientSockAddrLength = sizeof(ClientSockAddr);

						getpeername(ReadSockets.fd_array[i], (SOCKADDR*)&ClientSockAddr, &ClientSockAddrLength);

						rapidjson::Document Doc;
						Doc.Parse(Buffer);
						std::string Type = Doc["Type"].GetString();

						if (Type == "Move")
						{
							cout << "client(" << inet_ntoa(ClientSockAddr.sin_addr) << ") ";
							cout << "키: " << Doc["Key"].GetString();
							cout << " 좌표: (" << Doc["X"].GetInt() << ", " << Doc["Y"].GetInt() << ")" << endl;
						}
						else if (Type == "Chat")
						{
							cout << "client(" << inet_ntoa(ClientSockAddr.sin_addr);
							cout << ")" << Buffer << " send" << endl;
						}

						for (int j = 0; j < (int)ReadSockets.fd_count; ++j)
						{
							//자기꺼는 그냥 찍고 안 받으면 안되요?
							//클라이언트에서는 처리 안함.
							if (ReadSockets.fd_array[j] != ListenSocket)
							{
								PacketSize = (unsigned short)strlen(Buffer);
								PacketSize = htons(PacketSize);

								//header
								SendAll(ReadSockets.fd_array[j], (char*)&PacketSize, 2);

								//Data
								SendAll(ReadSockets.fd_array[j], Buffer, ntohs(PacketSize));

								
							}
						}
					}
				}
			}
		}
	}






	closesocket(ListenSocket);
	WSACleanup();

	return 0;
}