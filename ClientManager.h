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

		struct State {
			Mutex mutex;		
			Condition cond;
			Octets octets;
		};

		State	login_state;
		State	regst_state;

		State	frnd_lst_state;  
		State 	frnd_add_state;
		State 	frnd_del_state;
		State	frnd_req_state;
		State	frnd_acc_state;

		State	chat_with_state;

		State 	leav_msg_state;
		State	lmsg_lst_state;
		State 	lmsg_clr_state;

		OnChatRequestListener*      p_chat_request_listener;
		Mutex message_receive_listener_mutex;
		YChat::OnMessageReceiveListener* 	p_message_receive_listener;

	public:	

		ClientManager(const std::string& id_str) 
			: identification(id_str), curr_sid(0), curr_uid(0),
			p_chat_request_listener(NULL), p_message_receive_listener(NULL) { }

		enum { YCHAT_TIMED_OUT = 1024, YCHAT_NET_ERROR, };  // private status codes start from 1024  

	private:  // functions in this section are utility functions
		void sendControlProtocol(sid_t sid, const std::string& method, const Octets& content) {
			ControlProtocol* control_message = (ControlProtocol*) Protocol::Create(PROTOCOL_CONTROL);
			control_message->setup(method, content);
			Send(sid, control_message);
			control_message->Destroy();
		}

        int extractStatus(const Octets& octets) const {
			OctetsStream stream(octets);
			int status; stream >> status;
			return status;
		}

		int onRequest(const std::string& method, const Octets& content, State& state, int timeout) {
			if (curr_sid == 0) return YCHAT_NET_ERROR;
			Mutex::Scoped mutex_scoped(state.mutex);
			sendControlProtocol(curr_sid, method, content);
			if (state.cond.TimedWait(state.mutex, timeout) != ETIMEDOUT) {
				return extractStatus(state.octets);
			}
			return YCHAT_TIMED_OUT;
		}

		template <class Entity>
		int onRequest(const std::string& method, State& state, int timeout, 
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

		void onRespond(sid_t sid, const Octets& data, State& state) {
			{
				Mutex::Scoped mtx(state.mutex);
				state.octets = data;
			}
			state.cond.NotifyOne();
		}

	public:
		
		int login(const std::string& username, const std::string& password) {
			curr_username = username;
			OctetsStream login_stream; login_stream << username << password;
			return onRequest(CPMETHOD_LOGIN, login_stream, login_state, DEFAULT_TIMEOUT);
		}
	
		void onLoginRespond(sid_t sid, const Octets& data) {
			onRespond(sid, data, login_state);
		}	

		int regst(const std::string& username, const std::string& password) {
			curr_username = username;	
			OctetsStream regst_stream; regst_stream << username << password;
			return onRequest(CPMETHOD_REGST, regst_stream, regst_state, DEFAULT_TIMEOUT);
		}

		void onRegstRespond(sid_t sid, const Octets& data) {
			onRespond(sid, data, regst_state);
		}

		int getFriendList(std::vector<User>& friends) {
			return onRequest(CPMETHOD_FRND_LST, frnd_lst_state, DEFAULT_TIMEOUT, friends);
		}

		void onRcvFriendList(sid_t sid, const Octets& data) {
			onRespond(sid, data, frnd_lst_state);
		}

		int addFriend(const std::string& username) {
			OctetsStream stream; stream << username;
			return onRequest(CPMETHOD_FRND_ADD, stream, frnd_add_state, DEFAULT_TIMEOUT);
		}

		void onAddFriendRespond(sid_t sid, const Octets& data) {
			onRespond(sid, data, frnd_add_state);
		}

		int delFriend(const std::string& username) {
			OctetsStream stream; stream << username;
			return onRequest(CPMETHOD_FRND_DEL, stream, frnd_del_state, DEFAULT_TIMEOUT);
		}

		void onDelFriendRespond(sid_t sid, const Octets& data) {
			onRespond(sid, data, frnd_del_state);
		}

		int getFrndreqList(std::vector<FriendRequest>& requests) {
			return onRequest(CPMETHOD_FRND_REQ, frnd_req_state, DEFAULT_TIMEOUT, requests);
		}

		void onRcvFrndreqList(sid_t sid, const Octets& data) {
			onRespond(sid, data, frnd_req_state);
		}

		int acceptFrndreq(const std::string username) {
			OctetsStream stream; stream << username;
			return onRequest(CPMETHOD_FRND_ACC, stream, frnd_acc_state, DEFAULT_TIMEOUT);
		}

		void onAccFrndreqRespond(sid_t sid, const Octets& data) {
			onRespond(sid, data, frnd_acc_state);
		}

		int getMessageList(std::vector<Message>& messages) {
			return onRequest(CPMETHOD_LMSG_LST, lmsg_lst_state, DEFAULT_TIMEOUT, messages);
		}

		void onRcvMessageList(sid_t sid, const Octets& data) {
			onRespond(sid, data, lmsg_lst_state);
		}

		int leaveMessage(const std::string& username, const std::string& message) {
			OctetsStream stream; stream << username << message;
			return onRequest(CPMETHOD_LEAV_MSG, stream, leav_msg_state, DEFAULT_TIMEOUT);
		}

		void onLeaveMessageRespond(sid_t sid, const Octets& data) {
			onRespond(sid, data, leav_msg_state);
		}

		int clearMessages() {
			OctetsStream dummy_stream;
			return onRequest(CPMETHOD_LMSG_CLR, dummy_stream, lmsg_clr_state, DEFAULT_TIMEOUT);
		}

		void onClearMessagesRespond(sid_t sid, const Octets& data) {
			onRespond(sid, data, lmsg_clr_state);
		}

		int chatWith(const std::string& username) {
			if (username == curr_username || username == "") {
				return YCHAT_USR_INVAL;
			}

			Mutex::Scoped mutex_scoped(chat_with_state.mutex);
			
			OctetsStream user_stream; user_stream << username;
			sendControlProtocol(curr_sid, CPMETHOD_CHAT_WITH, user_stream);	

			if (chat_with_state.cond.TimedWait(chat_with_state.mutex, 60) != ETIMEDOUT)
				return extractStatus(chat_with_state.octets);

			sendControlProtocol(curr_sid, CPMETHOD_CHAT_FAIL, user_stream);
			return YCHAT_REQ_FAILED;
		}


		void onChatWithRespond(sid_t sid, const Octets& data) {
			onRespond(sid, data, chat_with_state);
		}

		void setOnChatRequestListener(OnChatRequestListener* plistener) {
			p_chat_request_listener = plistener;
		}

		void onChatRequest(sid_t sid, const Octets& data) {
			OctetsStream stream(data);
			std::string username; stream >> username;
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


	// -----------------------------------------------------------------------------------------------
	public :

		class OnProtocolProcessListener : public YChat::OnProtocolProcessListener {
			
			typedef void (ClientManager::*response_handler_t) (sid_t, const Octets&);

			struct s_response_handler {
				method_t method;
				response_handler_t handler;
			};
				
		public:
			void onProtocolProcess(Manager* pmanager, sid_t sid, Protocol* pprotocol) {
				typedef ClientManager CM;
				/** the list below is aligned using SPACE **/
				static struct s_response_handler response_handlers[] = {
					{CPMETHOD_LOGIN,        &CM::onLoginRespond          },
					{CPMETHOD_REGST,        &CM::onRegstRespond          },
					{CPMETHOD_FRND_LST,     &CM::onRcvFriendList         },
					{CPMETHOD_FRND_ADD,     &CM::onAddFriendRespond      },
					{CPMETHOD_FRND_DEL,     &CM::onDelFriendRespond      },
					{CPMETHOD_FRND_REQ,     &CM::onRcvFrndreqList        },
					{CPMETHOD_FRND_ACC,     &CM::onAccFrndreqRespond     },
					{CPMETHOD_LMSG_LST,     &CM::onRcvMessageList        },
					{CPMETHOD_LEAV_MSG,     &CM::onLeaveMessageRespond   },
					{CPMETHOD_LMSG_CLR,     &CM::onClearMessagesRespond  },
					{CPMETHOD_CHAT_WITH,    &CM::onChatWithRespond       },
					{CPMETHOD_CHAT_RQST,    &CM::onChatRequest           },    // a request handler
				};
				
				ClientManager* client_manager = (ClientManager*) pmanager;
				ControlProtocol* control_protocol = (ControlProtocol*) pprotocol;

				method_t method = control_protocol->getMethod();
				//OctetsStream content_stream(control_protocol->getContent());
				int n_handlers = sizeof(response_handlers) / sizeof(s_response_handler);
				for (int i = 0; i < n_handlers; i++) {  // linear search
					if (method == response_handlers[i].method) {
						const Octets& content = control_protocol->getContent();
						(client_manager->*response_handlers[i].handler)(sid, content);
						break;
					}
				}
			}	
		};

		class OnMessageReceiveListener : public YChat::OnMessageReceiveListener {
			const std::string& _username;
			std::vector<std::string>& _messages;
			Mutex& _messages_mutex;
		public:
			OnMessageReceiveListener(
				const std::string& username, 
				std::vector<std::string>& messages, Mutex& messages_mutex) 
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
