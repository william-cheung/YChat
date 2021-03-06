#ifndef __PROTOCOL_TYPES_H__
#define __PROTOCOL_TYPES_H__

#include <string>

typedef std::string method_t;

#define PROTOCOL_CONTROL  			1

#define CPMETHOD_LOGIN    			"LOGIN"
#define CPMETHOD_REGST    			"REGST"

#define CPMETHOD_CHAT_WITH			"CHAT_WITH"
#define CPMETHOD_CHAT_RQST			"CHAT_RQST"
#define CPMETHOD_CHAT_EXIT			"CHAT_EXIT"
#define CPMETHOD_CHAT_FAIL			"CHAT_FAIL"

#define CPMETHOD_FRND_LST 			"FRND_LST"
#define CPMETHOD_FRND_ADD 			"FRND_ADD"
#define CPMETHOD_FRND_DEL 			"FRND_DEL"
#define CPMETHOD_FRND_REQ 			"FRND_REQ"
#define CPMETHOD_FRND_ACC 			"FRND_ACC"

#define CPMETHOD_LEAV_MSG 			"LEAV_MSG"

#define CPMETHOD_LMSG_LST 			"LMSG_LST"
#define CPMETHOD_LMSG_CLR 			"LMSG_CLR"

#define PROTOCOL_CHAT_MESSAGE		2

#endif
