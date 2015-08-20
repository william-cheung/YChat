#ifndef __USER_H_HEADER_GUARD__
#define __USER_H_HEADER_GUARD__

#include <string>
#include <vector>

#include "IEntity.h"
#include "Friend.h"

namespace YChat {

	class User : public IEntity {
		uid_t uid;
		std::string user_name;
//		std::vector<Friend> friend_list;
	public:
	
		User(uid_t id, const std::string& name)
			: uid(id), user_name(name) { }	
		/*User(uid_t id, const std::string& name, const std::vector<Friend>& frnds)
			: uid(id), user_name(name), friend_list(frnds) { }	*/
		explicit User(const std::string& user_string) {
			*this = User::parse(user_string);
		}

		bool isNull() const { return uid == 0; }

		uid_t getUid() const { return uid; }
		const std::string& getUserName() const { return user_name; }
//		const std::vector<Friend>& getFriendList() const { return friend_list; }

		std::string toString() const {
			return User::toString(*this);
		}

/*		void addFriend(const Friend& frnd) {
			friend_list.push_back(frnd);
		}
*/		
		static User parse(const std::string& user_string);
		static std::string toString(const User& user);
	};
};

#endif	// User.h

