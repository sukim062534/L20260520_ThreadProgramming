#pragma once
#include "Packet.h"

class MovePacket : public IPacket
{
public: 
	std::string Type;
	std::string UserID;
	std::string Key;
	int X;
	int Y;

	void Parse(std::string InString) override;

	std::string ToString() override;

	int Length() override;
};

