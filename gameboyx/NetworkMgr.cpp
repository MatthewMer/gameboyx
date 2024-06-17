#include "NetworkMgr.h"
#include "general_config.h"
#include "helper_functions.h"
#include "logger.h"

namespace Backend {
	namespace Network {
		static bool check_ipv4_address(const std::string& _ip) {
			auto ip = Helpers::split_string(_ip, ".");

			if (ip.size() != 4) { return false; }

			for (const auto& n : ip) {
				std::string::const_iterator it = n.begin();
				while (it != n.end() && std::isdigit(*it)) ++it;
				if (n.empty() || it != n.end()) { return false; }
			}

			return true;
		}

		static bool check_port(const int& _port) {
			return _port >= PORT_MIN && _port <= PORT_MAX;
		}

		NetworkMgr* NetworkMgr::instance = nullptr;

		NetworkMgr* NetworkMgr::getInstance() {
			if (instance == nullptr) {
				instance = new NetworkMgr();
			}

			return instance;
		}

		void NetworkMgr::resetInstance() {
			if (instance != nullptr) {
				delete instance;
				instance = nullptr;
			}
		}

		void NetworkMgr::InitSocket(network_settings& _network_settings) {
			if (CheckSocket()) {
				ShutdownSocket();
			}

			std::string ipv4_address_ = _network_settings.ipv4_address;
			int port_ = _network_settings.port;

			if (check_ipv4_address(ipv4_address_) && check_port(port_)) {
				ipv4Address = ipv4_address_;
				port = port_;

				std::string result = "unhandled exception";

				try {
					networkContext = std::make_unique<asio::io_context>();

					asio::ip::tcp::resolver::results_type endpoints = asio::ip::tcp::resolver(*networkContext).resolve(ipv4Address, std::to_string(port));

					socket = std::make_unique<asio::ip::tcp::socket>(*networkContext);

					asio::async_connect(*socket, endpoints,
						[this](std::error_code _error_code, asio::ip::tcp::endpoint _endpoint) {
							if (!_error_code) { Listen(); }
						});

					networkThread = std::thread([this]() { networkContext->run(); });

				} catch (std::exception& ex) {
					result = std::string(ex.what());
				}

				if (socket->is_open()) {
					LOG_INFO("[network] socket opened: ", ipv4Address, ":", port);
				} else {
					LOG_ERROR("[network] open socket: ", ipv4Address, ":", port, " : ", result);
				}
			} else {
				LOG_ERROR("[network] no valid IP/Port: ", _network_settings.ipv4_address, ":", _network_settings.port);
			}
		}

		bool NetworkMgr::CheckSocket() {
			if (socket != nullptr) {
				return socket->is_open();
			} else {
				return false;
			}
		}

		void NetworkMgr::ShutdownSocket() {
			if (socket->is_open()) {
				asio::post(*networkContext, [this]() { socket->close(); });
			}

			networkContext->stop();
			if (networkThread.joinable()) { networkThread.join(); }

			socket.release();
			networkContext.release();
		}

		void NetworkMgr::Listen() {

		}
	}
}