#pragma once

#include "HardwareStructs.h"
#include "HardwareStructs.h"

#ifdef _WIN32
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00
#endif
#endif
#include "asio.hpp"
#include "thread"

class NetworkMgr {
public:
	// get/reset instance
	static NetworkMgr* getInstance();
	static void resetInstance();

	void InitSocket(network_settings& _network_settings);
	bool CheckSocket();
	void ShutdownSocket();

	// clone/assign protection
	NetworkMgr(NetworkMgr const&) = delete;
	NetworkMgr(NetworkMgr&&) = delete;
	NetworkMgr& operator=(NetworkMgr const&) = delete;
	NetworkMgr& operator=(NetworkMgr&&) = delete;



protected:
	// constructor
	NetworkMgr() = default;
	~NetworkMgr() = default;

private:
	static NetworkMgr* instance;

	std::string ipv4Address;
	int port;

	std::unique_ptr<asio::io_context> networkContext = nullptr;
	std::unique_ptr<asio::ip::tcp::socket> socket = nullptr;
	std::thread networkThread;

	void Listen();
};

