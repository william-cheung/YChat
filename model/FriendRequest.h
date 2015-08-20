#ifndef __FRIEND_REQUEST_H__
#define __FRIEND_REQUEST_H__

#include <string>
#include <time.h>

#include "User.h"

namespace YChat {
	class FriendRequest {
		uid_t 		sender_uid;
		std::string sender_name;
		time_t 		time;
	   	std::string message;
	public:
		FriendRequest(uid_t uid, std::string username, time_t _time) :
			sender_uid(uid), sender_name(username), time(_time) { } 

		FriendRequest(uid_t uid, std::string username, time_t _time, std::string _message) :
			sender_uid(uid), sender_name(username), time(_time), message(_message) { } 
		
		FriendRequest(const std::string& obj_str) {
			*this = FriendRequest::parse(obj_str);
		}
		
		uid_t getSenderUid() const { return sender_uid; }
		const std::string& getSenderName() const { return sender_name; }
		time_t getTime() const { return time; }
		const std::string& getMessage() const { return message; }

		std::string toString() const {
			return FriendRequest::toString(*this);
		}

		static FriendRequest parse(const std::string& obj_str);
		static std::string toString(const FriendRequest& request);
	};
};

#endif // Message.h

