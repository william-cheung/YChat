#include "FriendRequest.h"

#include "json/json.h"

namespace YChat {
	FriendRequest FriendRequest::parse(const std::string& obj_str) {
		Json::Reader reader;
		Json::Value root;
		if (reader.parse(obj_str, root)) {
			uid_t uid = root.get("sender_uid", 0).asInt();
			std::string name = root.get("sender_name", "").asString();
			time_t time = root.get("time", 0).asInt();
			std::string message = root.get("message", "").asString();
			return FriendRequest(uid, name, time, message);
		}
		return FriendRequest(0, "", (time_t)0, "");
	}

	std::string FriendRequest::toString(const FriendRequest& request) {
		Json::Value root;

		root["sender_uid"] = (int)request.getSenderUid();
		root["sender_name"] = request.getSenderName();
		root["time"] = (int)request.getTime();
		root["message"] = request.getMessage();

		Json::FastWriter writer;
		return writer.write(root);
	}
};
