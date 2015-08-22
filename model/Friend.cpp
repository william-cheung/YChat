#include "Friend.h"

#include "json/json.h"

namespace YChat {
	Friend Friend::parse(const std::string& friend_string) {
		Json::Reader reader;
		Json::Value root;
		if (reader.parse(friend_string, root)) {
			uid_t uid = root.get("uid", 0).asInt();
			std::string alias = root.get("alias", "").asString();
			return Friend(uid, alias);
		}
		return Friend(0);
	}

	std::string Friend::toString(const Friend& frnd) {
		Json::Value root;
		root["uid"] = frnd.getUid();
		root["alias"] = frnd.getAlias();
		Json::FastWriter writer;
		//writer.omitEndingLineFeed();
		return writer.write(root);
	}
};
