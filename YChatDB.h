#ifndef __YCHAT_DB_H__
#define __YCHAT_DB_H__

#include <string>
#include <vector>

#include "mutex.h"

#include "User.h"
#include "Friend.h"
#include "Message.h"
#include "FriendRequest.h"

namespace YChat {
	using namespace GNET::Thread;
	class YChatDB {
		static Mutex instance_locker;
		static YChatDB* instance;
		YChatDB();
		~YChatDB();
	public:
		static YChatDB* getInstance() {
			if (instance == NULL) {
				Mutex::Scoped mutex_scoped(instance_locker);
				if (instance == NULL) 
					instance = new YChatDB;
			}
			return instance;
		}

		static void delInstance() {
			Mutex::Scoped mutex_scoped(instance_locker);
			if (instance == NULL) return;
			delete instance;
			instance = NULL;
		}


	private:

		template <class Entity>
		bool	setEntities			(const std::string& tablename, uid_t uid, const std::vector<Entity>& entities);

		template <class Entity>
		bool	getEntities			(const std::string& tablename, uid_t uid, std::vector<Entity>& entities);

		template <class Entity>	
		bool	getEntities			(const std::string& tablename, const std::string& username, std::vector<Entity>& entities);

		template <class Entity, class Compare>
		bool	getSortedEntities	(const std::string& tablename, uid_t uid, std::vector<Entity>& entities, Compare compare);
		
		template <class Entity>
		bool	clrEntities			(const std::string& tablename, uid_t uid, Entity&);

	public:

		bool 		sync			(void);	

		uid_t		addUser			(const std::string& username, const std::string& password);	
		uid_t 		getUidByName	(const std::string& username);
		std::string getPassword		(const std::string& username);
		User		getUserByUid	(uid_t sid);
		bool		setFriends		(uid_t uid, const std::vector<Friend>& friends);
		bool		getFriends		(uid_t uid, std::vector<Friend>& friends);
		bool		getFriends		(uid_t uid, std::vector<User>& friends);
		Friend		getFriend		(uid_t uid, uid_t fuid);
		bool		addFriend		(uid_t uid, const std::string username, uid_t fuid, const std::string& f_username);	
		bool		doAddFriend		(uid_t uid, uid_t fuid, const std::string& f_alias);	
		bool		hasFriend		(uid_t uid, uid_t fuid);
		uid_t		hasFriend		(uid_t uid, const std::string& f_alias);
		bool		delFriend		(uid_t uid, uid_t fuid);
		bool		doDelFriend		(uid_t uid, uid_t fuid);
		bool		setFrndreqs		(uid_t uid, const std::vector<FriendRequest>& requests);
		bool		getFrndreqs		(uid_t uid, std::vector<FriendRequest>& requests);
		bool		getFrndreqs		(const std::string& username, std::vector<FriendRequest>& requests);
		uid_t		hasFrndreq		(uid_t uid, const std::string& r_username);
		bool		delFrndreq		(uid_t uid, uid_t req_uid);
		bool		accFriend		(uid_t snd_uid, const std::string& snd_username, uid_t rcv_uid, const std::string& rcv_username);
		bool		setMessages		(uid_t uid, const std::vector<Message>& messages);
		bool		getMessages		(uid_t uid, std::vector<Message>& messages);
		bool		clrMessages		(uid_t uid);
		bool		addMessage		(uid_t rcv_uid, const Message& message);
		bool		addMessage		(uid_t snd_uid, const std::string snd_username, uid_t rcv_uid, const std::string& message);
	};
};

#endif // YChatDB.h
