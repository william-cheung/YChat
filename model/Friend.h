#ifndef __FRIEND_H_HEADER_GUARD__
#define __FRIEND_H_HEADER_GUARD__

#include <string>

namespace YChat {
	
	typedef int uid_t;
	
	class Friend {
		uid_t uid;
		std::string alias;
	public:
		explicit Friend(uid_t id) : uid(id) { }
		Friend(uid_t id, const std::string& als) : uid(id), alias(als) { } 
		Friend(const std::string& friend_string) {
			*this = Friend::parse(friend_string);
		}

		bool isNull() const { return uid == 0; }
		
		uid_t getUid() const { return uid; }
		const std::string& getAlias() const { return alias; }

		std::string toString() const {
			return Friend::toString(*this);
		}

		static Friend parse(const std::string& friend_string);
		static std::string toString(const Friend& frnd); 
	};
};

#endif    // Friend.h

