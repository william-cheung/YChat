#ifndef __YCHAT_MANAGER__
#define __YCHAT_MANAGER__

#include <map>

#include "protocol.h"

#include "ChatMessage.h"

namespace YChat {
	class YChatManager : public GNET::Protocol::Manager {
	public:	

		enum { 
			YCHAT_OK, 
			YCHAT_SVR_ERROR, 
			YCHAT_CORR_DATA, 
			
			// login
			YCHAT_USR_INVAL, 
			YCHAT_PAS_INVAL, 
			YCHAT_USR_LOGGED,

			// regst
			YCHAT_USR_EXIST,

			// chat
			YCHAT_USR_OFFLINE,
			YCHAT_USR_AVAILABLE,
			YCHAT_USR_BUSY,
			YCHAT_USR_NOTFRIEND,
			YCHAT_REQ_FAILED,

			// friend
			YCHAT_REQ_ACCEPT,
			YCHAT_REQ_REFUSE,
						
	   	}; // common status (0 ~ 1023)
	
		virtual void onMessageReceive(sid_t std, const ChatMessage& chat_message) = 0;
	};
};
#endif // YChatManager.h
