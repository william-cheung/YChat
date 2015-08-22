#ifndef __MESSAGE_H_HEADER_GUARD__
#define __MESSAGE_H_HEADER_GUARD__

#include <string>
#include <time.h>

#include "User.h"

namespace YChat {
	class Message {
		uid_t 		sender_uid;
		std::string sender_name;
		time_t 		time;
	   	std::string text;
	public:
		Message(std::string username, time_t _time, std::string message) :
			sender_name(username), time(_time), text(message) { } 
		
		Message(const std::string& obj_str) {
			*this = Message::parse(obj_str);
		}
		
		const std::string& getSenderName() const { return sender_name; }
		time_t getTime() const { return time; }
		const std::string& getText() const { return text; }

		std::string toString() const {
			return Message::toString(*this);
		}

		static Message parse(const std::string& obj_str);
		static std::string toString(const Message& message);
	};
};

#endif // Message.h

