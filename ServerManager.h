#ifndef __SERVER_MANAGER_H__
#define __SERVER_MANAGER_H__

#include <map>
#include <set>
#include <vector>
#include <algorithm>

#include "protocol.h"

#include "YChatManager.h"
#include "ControlProtocol.h"
#include "Listeners.h"
#include "YChatDB.h"

namespace YChat {

	using namespace GNET::Thread;

	class ServerManager : public YChatManager {

		typedef std::map<std::string, sid_t> 	SessionMap;
		typedef std::map<sid_t, std::string> 	UserNameMap;
		typedef std::map<sid_t, int>		ChatRequestStateMap;
		typedef std::map<sid_t, sid_t>		ChatPairMap;

		std::string identification;

		YChatDB* database;

		Mutex user_tracker_mutex;
		SessionMap 	session_map;
		UserNameMap	username_map;

		Mutex chat_request_sids_mutex;
		std::set<sid_t> chat_request_sids;

		Mutex chat_pairs_mutex;
		ChatPairMap chat_pairs;

	public:

		ServerManager(const std::string& id_str) : identification(id_str), database(YChatDB::getInstance()) { }

		void releaseResources() {
			YChatDB::delInstance();
		} 

		sid_t getSidByUserName(std::string username) {
			Mutex::Scoped mtx(user_tracker_mutex);
			if (session_map.find(username) != session_map.end()) 
				return session_map[username];
			return 0;
		}

		std::string getUserNameBySid(sid_t sid) {
			Mutex::Scoped mtx(user_tracker_mutex);
			if (username_map.find(sid) != username_map.end())
				return username_map[sid];
			return "";
		}

		void sendControlProtocol(sid_t sid, const std::string& method, const Octets& content) {
			ControlProtocol* control_message = (ControlProtocol*) Protocol::Create(PROTOCOL_CONTROL);
			control_message->setup(method, content);
			Send(sid, control_message);
			control_message->Destroy();
		}

		void sendControlProtocol(sid_t sid, const std::string& method, int status) {
			OctetsStream stream; stream << status;
			sendControlProtocol(sid, method, stream);
		}

		void onUserLogin(sid_t sid, 
				const std::string username, const std::string password) {
			printf("login request : %s %s\n", username.c_str(), password.c_str());

			int login_status;
			uid_t uid = database->getUidByName(username);
			if (uid != 0) {
				std::string password_ = database->getPassword(username);
				if (password != password_) login_status = YCHAT_PAS_INVAL;
				else {
					Mutex::Scoped mtx(user_tracker_mutex);
					if (session_map.find(username) != session_map.end()) {
						login_status = YCHAT_USR_LOGGED;
					} else {
						session_map[username] = sid;
						username_map[sid] = username;
						login_status = YCHAT_OK;
					}
				}
			} else login_status = YCHAT_USR_INVAL;
	
			if (login_status == YCHAT_OK) {
				printf("%s login succeeded !\n", username.c_str());
			}

			sendControlProtocol(sid, CPMETHOD_LOGIN, login_status);
		}
		
		void onUserRegst(sid_t sid,
				const std::string username, const std::string password) {
			printf("regst request : %s %s\n", username.c_str(), password.c_str());

			int regst_status;
			uid_t uid = database->getUidByName(username);
			if (uid != 0) regst_status = YCHAT_USR_EXIST;
			else {
				if (database->addUser(username, password)) {
					printf("add user %s succeeded !\n", username.c_str());
					Mutex::Scoped mtx(user_tracker_mutex);
					session_map[username] = sid;
					username_map[sid] = username;
					regst_status = YCHAT_OK;
				} else regst_status = YCHAT_SVR_ERROR;
			}

			printf("%s : uid : %d\n", username.c_str(), database->getUidByName(username));

			sendControlProtocol(sid, CPMETHOD_REGST, regst_status);
		}

		void onGetFriends(sid_t sid) {
			OctetsStream data_stream;
			int get_friends_status = YCHAT_OK;
			std::string username; 
			uid_t uid = 0;
			std::vector<User> friends;
			
			if ((username = getUserNameBySid(sid)) == "" 
					|| (uid = database->getUidByName(username)) == 0 
					|| !database->getFriends(uid, friends))
				get_friends_status = YCHAT_SVR_ERROR; 
			
			if (get_friends_status != YCHAT_OK) {
				data_stream << get_friends_status;
				printf("%s get friends failed !\n", username.c_str());
			} else {
				printf("%s get friends succeeded !\n", username.c_str());
				get_friends_status = YCHAT_OK;
				int n_friends = friends.size();
				data_stream << get_friends_status << n_friends;
				for (int i = 0; i < friends.size(); i++) {
					data_stream << friends[i].toString();
				}
			}
			sendControlProtocol(sid, CPMETHOD_FRND_LST, data_stream);
		}

		void onAddFriend(sid_t sid, const std::string& username) {
			std::string src_username = getUserNameBySid(sid);
			uid_t uid = 0, fuid = 0;
			int status = YCHAT_OK;
			if (src_username == "") status = YCHAT_SVR_ERROR;
			else if ((uid = database->getUidByName(src_username)) == 0) status = YCHAT_SVR_ERROR;
			else if ((fuid = database->getUidByName(username)) == 0) status = YCHAT_USR_INVAL;
			else if (uid == fuid || database->hasFriend(uid, fuid)) status = YCHAT_USR_INVAL;
			else if (!database->addFriend(uid, src_username, fuid, username)) status = YCHAT_SVR_ERROR;
			sendControlProtocol(sid, CPMETHOD_FRND_ADD, status);
		}

		void onDelFriend(sid_t sid, const std::string& username) {
			std::string src_username = getUserNameBySid(sid);
			printf("%s wants to delete his/her friend %s\n", src_username.c_str(), username.c_str());
			
			uid_t uid = 0, fuid = 0;
			int status = YCHAT_OK;
			if (src_username == "") status = YCHAT_SVR_ERROR;
			else if ((uid = database->getUidByName(src_username)) == 0) status = YCHAT_SVR_ERROR;
			else if ((fuid = database->getUidByName(username)) == 0) status = YCHAT_USR_INVAL;
			else if (!database->hasFriend(uid, fuid)) status = YCHAT_USR_INVAL;
			else if (database->delFriend(uid, fuid)) { 
				printf("%s delete friend succeeded\n", src_username.c_str());
				status = YCHAT_OK;
			} else status = YCHAT_SVR_ERROR;
			sendControlProtocol(sid, CPMETHOD_FRND_DEL, status);
		}

		void onGetFrndreqs(sid_t sid) {
			int status = YCHAT_OK;
			std::string username;
			uid_t uid = 0;
			std::vector<FriendRequest> requests;
			if ((username = getUserNameBySid(sid)) == ""
					|| (uid = database->getUidByName(username)) == 0
					|| !database->getFrndreqs(uid, requests))
				status = YCHAT_SVR_ERROR;

			OctetsStream stream;
			stream << status;
		   if (status == YCHAT_OK) {
			   int n_requests = requests.size();
			   stream << n_requests;
			   for (int i = 0; i < n_requests; i++) {
				   stream << requests[i].toString();
			   }
		   } 
		   sendControlProtocol(sid, CPMETHOD_FRND_REQ, stream);
		}

		void onAccFriend(sid_t sid, const std::string& username) {
			std::string snd_username = getUserNameBySid(sid);
			uid_t uid = 0, req_uid = 0;
			int status = YCHAT_OK;
			if (snd_username == "") status = YCHAT_SVR_ERROR;
			else if ((uid = database->getUidByName(snd_username)) == 0)
				status = YCHAT_SVR_ERROR;
			else if ((req_uid = database->hasFrndreq(uid, username)) == 0
					|| database->hasFriend(uid, req_uid)) {
				//printf("aha, catch you ! %s\n", username.c_str());
				status = YCHAT_USR_INVAL;
			}
			else if (!database->accFriend(uid, snd_username, req_uid, username)) 
				status = YCHAT_SVR_ERROR;
			sendControlProtocol(sid, CPMETHOD_FRND_ACC, status);
		}

		void onGetMessages(sid_t sid) {
			int status = YCHAT_OK;
			std::string username; 
			uid_t uid = 0;
			std::vector<Message> messages;
			if ((username = getUserNameBySid(sid)) == "" 
					|| (uid = database->getUidByName(username)) == 0
					|| !database->getMessages(uid, messages)) 
				status = YCHAT_SVR_ERROR;

			OctetsStream stream;
			if (status != YCHAT_OK) {
				printf("%s get message list failed\n", username.c_str());
				stream << status;
			} else {
				printf("%s get message list succeeded\n", username.c_str());
				status = YCHAT_OK;
				int n_messages = messages.size();
				stream << status << n_messages;
				for (int i = 0; i < n_messages; i++) {
					stream << messages[i].toString();
				}
			}
			sendControlProtocol(sid, CPMETHOD_LMSG_LST, stream);
		}

		void onClearMessages(sid_t sid) {
			int status = YCHAT_OK;
			std::string username; 
			uid_t uid = 0;
			if ((username = getUserNameBySid(sid)) == "" 
					|| (uid = database->getUidByName(username)) == 0) {
				status = YCHAT_SVR_ERROR;
				// printf("username : %s, uid : %d\n", username.c_str(), uid);
				printf("server internal error in ServerManager::onClearMessages()\n");
			} else {
				printf("%s wants to clear his/her messages\n", username.c_str());
				if (!database->clrMessages(uid))
					status = YCHAT_SVR_ERROR;
			}
			sendControlProtocol(sid, CPMETHOD_LMSG_CLR, status);
		}

		void onLeaveMessage(sid_t sid, const std::string& username, const std::string& message) {
			int status = YCHAT_OK;
			std::string snd_username, rcv_username(username);
			uid_t snd_uid = 0, rcv_uid = 0;
			if ((snd_username = getUserNameBySid(sid)) == "" 
					|| (snd_uid = database->getUidByName(snd_username)) == 0)
				status = YCHAT_SVR_ERROR;
			else if ((rcv_uid = database->getUidByName(rcv_username)) == 0) 
				status = YCHAT_USR_INVAL;
			else if(!database->addMessage(rcv_uid, Message(snd_username, time(NULL), message))) 
				status = YCHAT_SVR_ERROR;
			sendControlProtocol(sid, CPMETHOD_LEAV_MSG, status);
		}

		bool isRequestingForChat(sid_t sid) {
			Mutex::Scoped mutex_scoped(chat_request_sids_mutex);
			return chat_request_sids.find(sid) != chat_request_sids.end();
		}

		bool isChating(sid_t sid) {
			Mutex::Scoped mutex_scoped(chat_pairs_mutex);
			return chat_pairs.find(sid) != chat_pairs.end();
		}

		void onChatRequest(sid_t sid, const std::string& username) {
			std::string src_username = getUserNameBySid(sid);
			int dst_user_sid = getSidByUserName(username);

			printf("%s wants to chat with %s\n", src_username.c_str(), username.c_str());
		
			int status = YCHAT_USR_AVAILABLE;
			int uid = 0, fuid = 0;
			if ((uid = database->getUidByName(src_username)) == 0) {
				status = YCHAT_SVR_ERROR;
			} else if ((fuid = database->getUidByName(username)) == 0) {
				status = YCHAT_USR_INVAL;
			} else if (!database->hasFriend(uid, fuid)) {
				status = YCHAT_USR_NOTFRIEND;
			} else if (dst_user_sid  == 0) {
				printf("but %s is offline !\n", username.c_str());
				status = YCHAT_USR_OFFLINE;
			} else if (isRequestingForChat(dst_user_sid) || isChating(dst_user_sid)) {
				printf("but %s is busy now !\n", username.c_str());
				status = YCHAT_USR_BUSY;
			} else {
				printf("sending chat request to %s\n", username.c_str());
				{
					Mutex::Scoped mutex_scoped(chat_request_sids_mutex);
					chat_request_sids.insert(sid);
				}
				OctetsStream chat_request_user; chat_request_user << src_username;
				sendControlProtocol(dst_user_sid, CPMETHOD_CHAT_RQST, chat_request_user); 
			}
			if (status != YCHAT_USR_AVAILABLE) 
				sendControlProtocol(sid, CPMETHOD_CHAT_WITH, status);
		}
	
		void onChatRequestAcked(sid_t sid, const std::string& username, int agreed) {
			sid_t request_sid = getSidByUserName(username);
			int chat_with_status;
			if (agreed) {
				sid_t sid1 = request_sid, sid2 = sid;
				Mutex::Scoped mutex_scoped(chat_pairs_mutex);
				if (isRequestingForChat(sid1)) {
					chat_pairs.insert(std::make_pair(sid1, sid2));
				}
				chat_pairs.insert(std::make_pair(sid2, sid1));
				chat_with_status = YCHAT_USR_AVAILABLE;
			} else {
				Mutex::Scoped mutex_scoped(chat_request_sids_mutex);
				chat_request_sids.erase(request_sid);
				chat_with_status = YCHAT_USR_BUSY;
			}
			sendControlProtocol(request_sid, CPMETHOD_CHAT_WITH, chat_with_status);
		}

		void onChatExit(sid_t sid, const std::string& username) {
			std::string username2 = getUserNameBySid(sid);
			printf("%s exits the chat with %s\n", username2.c_str(), username.c_str());
			{
				Mutex::Scoped mutex_scoped(chat_request_sids_mutex);
				chat_request_sids.erase(sid);
			}
			{
				Mutex::Scoped mutex_scoped(chat_pairs_mutex);
				chat_pairs.erase(sid);
			}
		}
		
		void onChatFail(sid_t sid, const std::string& username) {
			std::string username2 = getUserNameBySid(sid);
			printf("the chat request of %s failed !\n", username2.c_str());
			Mutex::Scoped mutex_scoped(chat_request_sids_mutex);
			chat_request_sids.erase(sid);
		}

		void onMessageReceive(const sid_t sid, const ChatMessage& chat_message) {
			std::string dst_username = chat_message.getDstUserName();
			sid_t dst_sid = getSidByUserName(dst_username);

			printf("%s send a message to %s : %s\n",
					chat_message.getSrcUserName().c_str(),
					dst_username.c_str(),
					chat_message.getMessage().c_str());
			
			if (dst_sid != 0) {
				Mutex::Scoped mutex_scoped(chat_pairs_mutex);
				if (chat_pairs.find(dst_sid) != chat_pairs.end() && chat_pairs[dst_sid] == sid)
					Send(dst_sid, &chat_message);
			} 
		};	

		// virtual methods from GNET::Protocol::Manager
		virtual void OnAddSession(sid_t sid) { 
		}

		virtual void OnDelSession(sid_t sid) { 
			Mutex::Scoped mtx(user_tracker_mutex);
			std::string username = username_map[sid];
			username_map.erase(sid);
			session_map.erase(username);	
		}
		virtual session_state_t* GetInitState() const {  
			static Protocol::Type _state[] = {
				PROTOCOL_CONTROL, PROTOCOL_CHAT_MESSAGE,
			};
			static session_state_t state("", _state, sizeof(_state)/sizeof(Protocol::Type), 5);
			return &state;
	   	} 
		virtual std::string Identification() const { return identification; }


	public:
		class OnProtocolProcessListener : public YChat::OnProtocolProcessListener {
		public:
			void onProtocolProcess(Manager* pmanager, sid_t sid, Protocol* pprotocol) {
				ServerManager* server_manager = (ServerManager*)pmanager;
				ControlProtocol* control_protocol = (ControlProtocol*) pprotocol;

				std::string method = control_protocol->getMethod();
				OctetsStream content_stream(control_protocol->getContent());
				if (method == CPMETHOD_LOGIN) {
					std::string username, password;
					content_stream >> username >> password;
					server_manager->onUserLogin(sid, username, password);
				} else if (method == CPMETHOD_REGST) {
					std::string username, password;
					content_stream >> username >> password;
					server_manager->onUserRegst(sid, username, password);	
				} else if (method == CPMETHOD_CHAT_WITH) {
					std::string username; content_stream >> username;
					server_manager->onChatRequest(sid, username);
				} else if (method == CPMETHOD_CHAT_RQST) {
					std::string username;  int agreed;
					content_stream >> username >> agreed;
					server_manager->onChatRequestAcked(sid, username, agreed);
				} else if (method == CPMETHOD_CHAT_EXIT) {
					std::string username;  content_stream >> username;
					server_manager->onChatExit(sid, username);
				} else if (method == CPMETHOD_CHAT_FAIL) {
					std::string username; content_stream >> username;
					server_manager->onChatFail(sid, username);
				} else if (method == CPMETHOD_FRND_LST) {
					server_manager->onGetFriends(sid);
				} else if (method == CPMETHOD_FRND_ADD) {
					std::string username; content_stream >> username;
					server_manager->onAddFriend(sid, username);
				} else if (method == CPMETHOD_FRND_DEL) {
					std::string username; content_stream >> username;
					server_manager->onDelFriend(sid, username);
				} else if (method == CPMETHOD_FRND_REQ) {
					server_manager->onGetFrndreqs(sid);
				} else if (method == CPMETHOD_FRND_ACC) {
					std::string username; content_stream >> username;
					server_manager->onAccFriend(sid, username);
				} else if (method == CPMETHOD_LMSG_LST) {
					server_manager->onGetMessages(sid);
				} else if (method == CPMETHOD_LMSG_CLR) {
					server_manager->onClearMessages(sid);
				} else if (method == CPMETHOD_LEAV_MSG) {
					std::string username, message;
					content_stream >> username >> message;
					server_manager->onLeaveMessage(sid, username, message);
				} 
			}
		};

	};
};
#endif // ServerManager.h
