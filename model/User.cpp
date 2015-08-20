#include "User.h"

#include "json/json.h"

namespace YChat {
	User User::parse(const std::string& user_string) {
		Json::Reader reader;
		Json::Value root;
		if (reader.parse(user_string, root)) {
			uid_t uid = root.get("uid", 0).asInt();
			std::string user_name = root.get("username", "").asString();
			/*int nfriends = root["friends"].size();
			std::vector<Friend> friends;
			for (int i = 0; i < nfriends; i++) {
				friends.push_back(Friend(root["friends"][i].asString()));
			}
			return User(uid, user_name, friends);*/
			return User(uid, user_name);
		}
		return User(0, "");
	}

	std::string User::toString(const User& user) {
		Json::Value root;
		
		root["uid"] = user.getUid();
		root["username"] = user.getUserName();
/*
		std::vector<Friend> friends = user.getFriendList();
		Json::Value arr_obj;
		for (int i = 0; i < friends.size(); i++) {
			arr_obj.append(friends[i].toString());
		}
		root["friends"] = arr_obj;
*/
		Json::FastWriter writer;
		// writer.omitEndingLineFeed();
		return writer.write(root);
	}
};
