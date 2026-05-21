#include "pch.h"
#include "MovePacket.h"

void MovePacket::Parse(std::string InString)
{
	JSONDocument.Parse(InString.c_str());

	UserID = JSONDocument["UserID"].GetString();
	Type = JSONDocument["Type"].GetString();
	X = JSONDocument["X"].GetInt();
	X = JSONDocument["X"].GetInt();
	Y = JSONDocument["Y"].GetInt();
}

std::string MovePacket::ToString()
{
	JSONDocument.SetObject();
	JSONDocument.AddMember("Type", std::string("Move"), JSONDocument.GetAllocator());
	JSONDocument.AddMember("UserID", UserID, JSONDocument.GetAllocator());
	JSONDocument.AddMember("Key", Key, JSONDocument.GetAllocator());
	JSONDocument.AddMember("X", X, JSONDocument.GetAllocator());
	JSONDocument.AddMember("Y", Y, JSONDocument.GetAllocator());

	rapidjson::StringBuffer Buffer;
	rapidjson::Writer<rapidjson::StringBuffer> Writer(Buffer);
	JSONDocument.Accept(Writer);

	return Buffer.GetString();
}
int MovePacket::Length()
{
	return (int)ToString().length();
}
