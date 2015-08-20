#include "ChatMessage.h"
#include "YChatManager.h"

namespace YChat {
	static ChatMessage __chat_message_stub;

	void ChatMessage::Process(Manager* pmanager, sid_t sid) {
		YChatManager* pmanager2 = (YChatManager*)pmanager;
		pmanager2->onMessageReceive(sid, *this);
	}
};
