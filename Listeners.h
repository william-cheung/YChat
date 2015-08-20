#ifndef __LISTENERS__H__
#define __LISTENERS__H__

#include <string>

#include "protocol.h"

#include "GNETTypes.h"

namespace YChat {
	class OnProtocolProcessListener {
	public:
		virtual void onProtocolProcess(
			Manager* pmanager,
			sid_t sid,
			Protocol* pprotocol) = 0;
	};

	class OnChatRequestListener {
	public:
		virtual void onChatRequest(const std::string& user_name) = 0;
	};

	class OnMessageReceiveListener {
	public:
		virtual void onMessageReceive(const std::string& user_name, const std::string& message) = 0;
	};
};

#endif // Listeners.h


