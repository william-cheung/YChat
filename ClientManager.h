#ifndef __CLIENT_MANAGER_H__
#define __CLIENT_MANAGER_H__

#include <string>
#include <vector>

#include "YChatManager.h"
#include "Listeners.h"
#include "ChatMessage.h"
#include "ControlProtocol.h"

#include "User.h"
#include "Message.h"
#include "FriendRequest.h"

#define DEFAULT_TIMEOUT		5

namespace YChat {
	
	using namespace GNET::Thread;

	class ClientManager : public YChatManager {	
	private:

		std::string identification;

		std::string curr_username;
		sid_t curr_sid;
		uid_t curr_uid;

		struct StateBase {
			Mutex mutex;
			Condition cond;
		};

		struct State : StateBase {
			int status;
		};

		struct StateWithData : StateBase {
			Octets octets;
		};

		State	login_state;
		State	regst_state;

		State 	frnd_add_state;
		State 	frnd_del_state;
		State	frnd_acc_state;

		State	chat_with_state;

		State 	leav_msg_state;
		State 	lmsg_clr_state;

		StateWithData frnd_lst_state;
		StateWithData frnd_req_state;
		StateWithData lmsg_lst_state; 

		OnChatRequestListener*      p_chat_request_listener;
		Mutex message_receive_listener_mutex;
		YChat::OnMessageReceiveListener* 	p_message_receive_listener;

	public:	

		ClientManager(const std::string& id_str) 
			: identification(id_str), curr_sid(0), curr_uid(0),
			p_chat_request_listener(NULL), p_message_receive_listener(NULL) { }

		enum { YCHAT_TIMED_OUT = 1024, YCHAT_NET_ERROR, };

	private:
		void sendControlProtocol(sid_t sid, const std::string& method, const Octets& content) {
			ControlProtocol* control_message = (ControlProtocol*) Protocol::Create(PROTOCOL_CONTROL);
			control_message->setup(method, content);
			Send(sid, control_message);
			control_message->Destroy();
		}

		int onRequest(const std::string& method, const Octets& content, State& state, int timeout) {
			if (curr_sid == 0) return YCHAT_NET_ERROR;
			Mutex::Scoped mutex_scoped(state.mutex);
			sendControlProtocol(curr_sid, method, content);
			if (state.cond.TimedWait(state.mutex, timeout) != ETIMEDOUT) {
				return state.status;
			}
			return YCHAT_TIMED_OUT;
		}

		template <class Entity>
		int onRequest(const std::string& method, StateWithData& state, int timeout, 
				std::vector<Entity>& entities) {
			if (curr_sid == 0) return YCHAT_NET_ERROR;
			Mutex::Scoped mutex_scoped(state.mutex);
			sendControlProtocol(curr_sid, method, Octets());
			if (state.cond.TimedWait(state.mutex, timeout) != ETIMEDOUT) {
				try {
					OctetsStream stream(state.octets);
					int status;
					stream >> status;
					if (status != YCHAT_OK) return status;
			    	int n_entities; stream >> n_entities;
					entities.clear();
					while (n_entities-- > 0) {
						std::string obj_str; stream >> obj_str;
						entities.push_back(Entity(obj_str));
					}
				} catch ( ... ) {
					return YCHAT_CORR_DATA;
				}	
				return YCHAT_OK;
			}
			return YCHAT_TIMED_OUT;
		}

		void onRespond(sid_t sid, int status, State& state) {
			{
				Mutex::Scoped mtx(state.mutex);
				state.status = status;
			}
			state.cond.NotifyOne();	
		}

		void onRespond(sid_t sid, const Octets& octets, StateWithData& state) {
			{
				Mutex::Scoped mtx(state.mutex);
				state.octets = octets;
			}
			state.cond.NotifyOne();
		}

	public:
		
		int login(const std::string& username, const std::string& password) {
			curr_username = username;
			OctetsStream login_stream; login_stream << username << password;
			return onRequest(CPMETHOD_LOGIN, login_stream, login_state, DEFAULT_TIMEOUT);
		}
	
		void onLoginRespond(sid_t sid, int status) {
			onRespond(sid, status, login_state);
		}	

		int regst(const std::string& username, const std::string& password) {
			curr_username = username;	
			OctetsStream regst_stream; regst_stream << username << password;
			return onRequest(CPMETHOD_REGST, regst_stream, regst_state, DEFAULT_TIMEOUT);
		}

		void onRegstRespond(sid_t sid, int status) {
			onRespond(sid, status, regst_state);
		}

		int getFriendList(std::vector<User>& friends) {
			return onRequest(CPMETHOD_FRND_LST, frnd_lst_state, DEFAULT_TIMEOUT, friends);
		}

		void onRecvFriendList(sid_t sid, const Octets& content) {
			onRespond(sid, content, frnd_lst_state);
		}

		int addFriend(const std::string& username) {
			OctetsStream stream; stream << username;
			return onRequest(CPMETHOD_FRND_ADD, stream, frnd_add_state, DEFAULT_TIMEOUT);
		}

		void onAddFriendRespond(sid_t sid, int status) {
			onRespond(sid, status, frnd_add_state);
		}

		int delFriend(const std::string& username) {
			OctetsStream stream; stream << username;
			return onRequest(CPMETHOD_FRND_DEL, stream, frnd_del_state, DEFAULT_TIMEOUT);
		}

		void onDelFriendRespond(sid_t sid, int status) {
			onRespond(sid, status, frnd_del_state);
		}

		int getFrndreqList(std::vector<FriendRequest>& requests) {
			return onRequest(CPMETHOD_FRND_REQ, frnd_req_state, DEFAULT_TIMEOUT, requests);
		}

		void onRecvFrndreqList(sid_t sid, const Octets& content) {
			onRespond(sid, content, frnd_req_state);
		}

		int acceptFrndreq(const std::string username) {
			OctetsStream stream; stream << username;
			return onRequest(CPMETHOD_FRND_ACC, stream, frnd_acc_state, DEFAULT_TIMEOUT);
		}

		void onAccFrndreqRespond(sid_t sid, int status) {
			onRespond(sid, status, frnd_acc_state);
		}

		int getMessageList(std::vector<Message>& messages) {
			return onRequest(CPMETHOD_LMSG_LST, lmsg_lst_state, DEFAULT_TIMEOUT, messages);
		}

		void onRecvMessageList(sid_t sid, const Octets& content) {
			onRespond(sid, content, lmsg_lst_state);
		}

		int leaveMessage(const std::string& username, const std::string& message) {
			OctetsStream stream; stream << username << message;
			return onRequest(CPMETHOD_LEAV_MSG, stream, leav_msg_state, DEFAULT_TIMEOUT);
		}

		void onLeaveMessageRespond(sid_t sid, int status) {
			onRespond(sid, status, leav_msg_state);
		}

		int clearMessages() {
			OctetsStream dummy_stream;
			return onRequest(CPMETHOD_LMSG_CLR, dummy_stream, lmsg_clr_state, DEFAULT_TIMEOUT);
		}

		void onClearMessagesRespond(sid_t sid, int status) {
			onRespond(sid, status, lmsg_clr_state);
		}

		int chatWith(const std::string& username) {
			if (username == curr_username || username == "") {
				return YCHAT_USR_INVAL;
			}

			Mutex::Scoped mutex_scoped(chat_with_state.mutex);
			
			OctetsStream user_stream; user_stream << username;
			sendControlProtocol(curr_sid, CPMETHOD_CHAT_WITH, user_stream);	

			if (chat_with_state.cond.TimedWait(chat_with_state.mutex, 60) != ETIMEDOUT)
				return chat_with_state.status;

			sendControlProtocol(curr_sid, CPMETHOD_CHAT_FAIL, user_stream);
			return YCHAT_REQ_FAILED;
		}


		void onChatWithRespond(sid_t sid, int status) {
			onRespond(sid, status, chat_with_state);
		}

		void setOnChatRequestListener(OnChatRequestListener* plistener) {
			p_chat_request_listener = plistener;
		}

		void onChatRequest(sid_t sid, const std::string& username) {
			if (p_chat_request_listener != NULL) {
				p_chat_request_listener->onChatRequest(username);
			}
		}

		void sendChatRequestAck(const std::string& username, int agreed) {
			OctetsStream content_stream; content_stream << username << agreed;
			sendControlProtocol(curr_sid, CPMETHOD_CHAT_RQST, content_stream);
		}

		void onChatExit(const std::string& username) {
			setOnMessageReceiveListener(NULL);
			OctetsStream content_stream;  content_stream << username;
			sendControlProtocol(curr_sid, CPMETHOD_CHAT_EXIT, content_stream);
		}
		
		void setOnMessageReceiveListener(OnMessageReceiveListener* plistener) {
			Mutex::Scoped mtx(message_receive_listener_mutex);
			p_message_receive_listener = plistener;
		}
	
		int sendMessage(const std::string& username, const std::string& message) {
			if (curr_sid) {
				ChatMessage* p_chat_message = (ChatMessage*) Protocol::Create(PROTOCOL_CHAT_MESSAGE);
				p_chat_message->setup(curr_username, username, message);
				this->Send(curr_sid, p_chat_message);
				p_chat_message->Destroy();
				return 1;
			}			
			return 0;
		}
		
		// virtual methods from class YChatManager
		void onMessageReceive(const sid_t sid, const ChatMessage& chat_message) {
			Mutex::Scoped mtx(message_receive_listener_mutex);
			if (p_message_receive_listener != NULL) {
				p_message_receive_listener->onMessageReceive(
						chat_message.getSrcUserName(), chat_message.getMessage());
			}
		}

		// virtual methods from class GNET::Protocol::Manager
		virtual void OnAddSession(sid_t sid) {
		   	curr_sid = sid;
			//printf("onAddSession\n");	
		}
		virtual void OnDelSession(sid_t) { 
			curr_sid = 0;
			//printf("onDelSession\n");
		}
		virtual session_state_t* GetInitState() const {
			static Protocol::Type _state[] = {
				PROTOCOL_CONTROL, PROTOCOL_CHAT_MESSAGE,
			};
			static session_state_t state("", _state, sizeof(_state)/sizeof(Protocol::Type), 5);
			return &state;
	   	}
		virtual std::string Identification() const { return identification; }


	// --------------------------------------------------------------------------------------------------
	
	public :
		class OnProtocolProcessListener : public YChat::OnProtocolProcessListener {
		public:
			void onProtocolProcess(Manager* pmanager, sid_t sid, Protocol* pprotocol) {
				ClientManager* client_manager = (ClientManager*) pmanager;
				ControlProtocol* control_protocol = (ControlProtocol*) pprotocol;

				std::string method = control_protocol->getMethod();
				OctetsStream content_stream(control_protocol->getContent());
				if (method == CPMETHOD_LOGIN) {
					int login_status; content_stream >> login_status;
					client_manager->onLoginRespond(sid, login_status);
				} else if (method == CPMETHOD_REGST) {
					int regst_status; content_stream >> regst_status;
					client_manager->onRegstRespond(sid, regst_status);
				} else if (method == CPMETHOD_CHAT_WITH) {
					int chat_with_status; content_stream >> chat_with_status;
					client_manager->onChatWithRespond(sid, chat_with_status);
				} else if (method == CPMETHOD_CHAT_RQST) {
					std::string username; content_stream >> username;
					client_manager->onChatRequest(sid, username);
				} else if (method == CPMETHOD_FRND_LST) {
					client_manager->onRecvFriendList(sid, content_stream);
				} else if (method == CPMETHOD_FRND_ADD) {
					int add_frnd_status; content_stream >> add_frnd_status;
					client_manager->onAddFriendRespond(sid, add_frnd_status);
				} else if (method == CPMETHOD_FRND_DEL) {
					int del_frnd_status; content_stream >> del_frnd_status;
					client_manager->onDelFriendRespond(sid, del_frnd_status);
				} else if (method == CPMETHOD_FRND_REQ) {
					client_manager->onRecvFrndreqList(sid, content_stream);
				} else if (method == CPMETHOD_FRND_ACC) {
					int status; content_stream >> status;
					client_manager->onAccFrndreqRespond(sid, status);	
				} else if (method == CPMETHOD_LMSG_LST) {
					client_manager->onRecvMessageList(sid, content_stream);
				} else if (method == CPMETHOD_LMSG_CLR) {
					int status; content_stream >> status;
					client_manager->onClearMessagesRespond(sid, status);
				} else if (method == CPMETHOD_LEAV_MSG) {
					int status; content_stream >> status;
					client_manager->onLeaveMessageRespond(sid, status);
				}
			}	
		};

		class OnMessageReceiveListener : public YChat::OnMessageReceiveListener {
			const std::string& _username;
			std::vector<std::string>& _messages;
			Mutex& _messages_mutex;
		public:
			OnMessageReceiveListener(const std::string& username, std::vector<std::string>& messages, Mutex& messages_mutex) 
				: _username(username), _messages(messages), _messages_mutex(messages_mutex) { }
	
			void onMessageReceive(const std::string& username, const std::string& message) {
				if (_username != username) return;
				Mutex::Scoped mtx(_messages_mutex);
				_messages.push_back(message);
			}	
		};
	};
};

#endif // ClientManager.h
