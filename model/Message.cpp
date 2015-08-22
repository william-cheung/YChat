#include "Message.h"

#include "json/json.h"

namespace YChat {
	Message Message::parse(const std::string& obj_str) {
		Json::Reader reader;
		Json::Value root;
		if (reader.parse(obj_str, root)) {
			std::string name = root.get("sender_name", "").asString();
			time_t time = root.get("time", 0).asInt();
			std::string text = root.get("text", "").asString();
			return Message(name, time, text);
		}
		return Message("", (time_t)0, "");
	}

	std::string Message::toString(const Message& message) {
		Json::Value root;

		root["sender_name"] = message.getSenderName();
		root["time"] = (int)message.getTime();
		root["text"] = message.getText();

		Json::FastWriter writer;
		return writer.write(root);
	}
};
