#ifndef __CHAT_MESSAGE__
#define __CHAT_MESSAGE__

#include "protocol.h"

#include "GNETTypes.h"
#include "ProtocolTypes.h"

namespace YChat {
	class ChatMessage : public GNET::Protocol {

		typedef std::string 				user_info_t;

		Protocol* Clone() const { return new ChatMessage(*this); }
		
		Octets time_stamp;
		Octets from_user, to_user;
		Octets message;
			
	public:
		ChatMessage() : Protocol(PROTOCOL_CHAT_MESSAGE) { }
		ChatMessage(const ChatMessage& rhs) : Protocol(rhs) { }

		void setup(const std::string& src_user_name, const std::string& dst_user_name, 
				const std::string& message) {
			user_info_t src_user(src_user_name), dst_user(dst_user_name);
			OctetsStream os;
			os << src_user, from_user = os, os.clear();
			os << dst_user, to_user = os, os.clear();
			os << message, this->message = os, os.clear();
		}
		
		
		std::string getSrcUserName() const {
			user_info_t user;
			OctetsStream os(from_user);
			os >> user;
			return user;
		}

		std::string getDstUserName() const {
			user_info_t user;
			OctetsStream os(to_user);
			os >> user;
			return user;
		}	

		std::string getMessage() const {
			std::string msg_str;
			OctetsStream os(message);
			os >> msg_str;
			return msg_str;
		}	

		// virtual methods originally from class Marshal
		virtual OctetsStream& marshal(OctetsStream& os) const {
			os << time_stamp;
			os << from_user << to_user;
			os << message;
			return os;
		}	
		virtual const OctetsStream& unmarshal(const OctetsStream& os) {
			os >> time_stamp;
			os >> from_user >> to_user;
			os >> message;
			return os;
		}

		// virtual methods from class Protocol
		virtual int PriorPolicy() const { return 1; }
		virtual void Process(Manager*, sid_t);
	};
};

#endif // ChatMessage.h
