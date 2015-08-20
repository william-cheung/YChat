#include "storage.h"
#include "marshal.h"

#include "YChatDB.h"

using 	GNET::Octets;

typedef GNET::Marshal::OctetsStream OctetsStream;

#if defined USE_WDB
using 	WDB::StorageEnv;
using	WDB::DbException;
#elif defined USE_BDB
using 	BDB::StorageEnv;
using 	BDB::DbException;
#elif defined USE_CDB
using 	CDB::StorageEnv;
using 	CDB::DbException;
#else
#error "adjust your defines !"
#endif

typedef StorageEnv::Storage		Storage;
typedef StorageEnv::Transaction		Transaction;

namespace YChat {
	using namespace std;

	static const string user_index_tablename = "uind";	
	static const string curr_max_uid_key = "__curr_max_uid_YChat__"; 

	static const string user_tablename = "user";
	static const User null_user("");

	static const string friend_tablename = "friend";
	static const Friend null_friend("");

	static const string frndreq_tablename = "frndreq";
	static const FriendRequest null_frndreq("");

	static const string message_tablename = "message";
	static const Message null_message("");

	YChatDB* YChatDB::instance = NULL;
	Mutex YChatDB::instance_locker;
	
	YChatDB::YChatDB() {
		StorageEnv::Open();
	}

	YChatDB::~YChatDB() {
		StorageEnv::checkpoint();
		StorageEnv::Close();
	}
	
	bool YChatDB::sync() {
		return StorageEnv::checkpoint();
	}

	static bool sync(Storage* pstorage) {
		return StorageEnv::checkpoint(pstorage);
	}
	
	static inline Storage* get_table(const string& tablename) {
		return StorageEnv::GetStorage(tablename.c_str());
	}

	static inline Storage* get_user_index_table() { 
		return get_table(user_index_tablename);
	}

	static inline Storage* get_user_table() {
		return get_table(user_tablename);
	}

	static inline Storage* get_friend_table() {
		return get_table(friend_tablename);
	}

	static inline Storage* get_frndreq_table() {
		return get_table(frndreq_tablename);
	}

	static inline Storage* get_message_table() {
		return get_table(message_tablename);
	}

	static inline bool is_db_notfound_ex(/*const*/ DbException& ex) {
		return string(ex.what()) == "NOTFOUND";
	}

    static uid_t get_curr_max_uid(Storage* pstorage, Transaction& txn) {
		uid_t uid = 0;
		try {
			OctetsStream os; os << curr_max_uid_key;
			Octets uid_data = pstorage->find(os, txn);
			if (uid_data.size() == 0) return 0;
			os = uid_data;	os >> uid;
		} catch (DbException ex) {
			if (is_db_notfound_ex(ex))  
				return 0;
			else throw;
		}
		return uid;
	}

	static void put_curr_max_uid(Storage* pstorage, Transaction& txn, uid_t uid) {
		OctetsStream key; key << curr_max_uid_key;
		OctetsStream value; value << uid;
		pstorage->insert(key, value, txn, uid);
	}	

	static bool inc_and_get_curr_max_uid(Storage* pstorage, Transaction& txn, uid_t& uid) {
		try {
			uid = get_curr_max_uid(pstorage, txn);
			uid = uid + 1; // can it overflow ?
			put_curr_max_uid(pstorage, txn, uid);
		} catch ( ... ) {
			return false;
		}
		return true;
	}
	
	uid_t YChatDB::addUser(const string& username, const string& password) {
		Storage* uind_store = get_user_index_table();
		Storage* user_store = get_user_table();
		if (uind_store == NULL || user_store == NULL) return 0;
		Transaction txn;
		uid_t uid = 0;
		try {
			if (!inc_and_get_curr_max_uid(uind_store, txn, uid)) {
				txn.abort();
				return 0;
			}
			OctetsStream key, value; 
			key << username, value << password << uid;
			uind_store->insert(key, value, txn);
			
			User user(uid, username);
			OctetsStream key2, value2;
			key2 << uid, value2 << user.toString();
			user_store->insert(key2, value2, txn);
		} catch (...) {
			txn.abort();
			return 0;
		}

		return uid;
	}

	uid_t YChatDB::getUidByName(const string& username) {
		Storage* pstorage = get_user_index_table(); 
		Transaction txn;
		uid_t uid = 0;
		if (pstorage == NULL) return 0;
		try {
			OctetsStream key; key << username;
			OctetsStream value(pstorage->find(key, txn));
			if (value.size() == 0) return 0;
			string pw;
			value >> pw >> uid;
		} catch ( ... /*DbException ex*/) {
			//if (ex.what() == "NOTFOUND")
				return 0;
			//else throw;
		}
		return uid;
	}	

	string YChatDB::getPassword(const string& username) {
		Storage* pstorage = get_user_index_table();
		Transaction txn;
		string password;
		if (pstorage == NULL) return "";
		try {
			OctetsStream key; key << username;
			OctetsStream value(pstorage->find(key, txn));
			if (value.size() == 0) return "";
			uid_t uid;
			value >> password >> uid;
		} catch (...) {
			return "";
		}
		return password;
	}

	User YChatDB::getUserByUid(uid_t uid) {
		Transaction txn;
		Storage* pstorage = get_user_table();
		if (pstorage == NULL) return null_user;
		try {
			OctetsStream key; key << uid;
			Octets value;
			if (pstorage->find(key, value, txn)) {
				OctetsStream stream(value);
				string obj_str;
				stream >> obj_str;
				return User(obj_str);
			} else return null_user;
		} catch (...) {
			return null_user;
		}
	}

	template <class Entity> 
	void read_entities(const Octets& octets, vector<Entity>& entities) {
		OctetsStream stream(octets);
		int n_entities;
		stream >> n_entities;
		entities.clear();
		for (int i = 0; i < n_entities; i++) {
			string obj_str;
			stream >> obj_str;
			entities.push_back(Entity(obj_str));
		}
	}

	template <class Entity>
	void write_entities(Octets& octets, vector<Entity> entities) {
		OctetsStream stream;
		int n_entities = entities.size();
		stream << n_entities;
		for (int i = 0; i < n_entities; i++) 
			stream << entities[i].toString();
		octets = stream;
	}

	template <class Entity>
	bool YChatDB::getEntities(const string& tablename, 
			uid_t uid, vector<Entity>& entities) {
		Transaction txn;
		Storage* pstorage = get_table(tablename);
		if (pstorage == NULL) return false;
		entities.clear();
		try {
			OctetsStream key; key << uid;
			Octets value;
			if (pstorage->find(key, value, txn)) {
				read_entities(value, entities);
			}
		} catch (DbException ex) {	
			if (is_db_notfound_ex(ex)) return true;
			return false;
		} catch (...) {
			return false;
		}
		return true;
	}

	template <class Entity>
	bool YChatDB::getEntities(const string& tablename, 
			const string& username, vector<Entity>& entities) {
		uid_t uid = getUidByName(username);
		if (uid == 0) return false;
		if (getEntities<Entity>(tablename, uid, entities)) 
			return true;
		return false;
	}

	template <class Entity, class Compare>
	bool YChatDB::getSortedEntities(const std::string& tablename, uid_t uid, 
			std::vector<Entity>& entities, Compare compare) {
		if (getEntities(tablename, uid, entities)) {
			sort(entities.begin(), entities.end(), compare);
			return true;
		}
		return false;
	}
		
	template <class Entity>
  	bool YChatDB::setEntities(const string& tablename,
			uid_t uid, const vector<Entity>& entities) {
		Transaction txn;
		Storage* pstorage = get_table(tablename);
		if (pstorage == NULL) return false;
		try {
			OctetsStream key; key << uid;
			Octets value;
			write_entities(value, entities);
			pstorage->insert(key, value, txn);
		} catch (...) {
			return false;
		}
		return true;
	}

	template <class Entity>
	bool YChatDB::clrEntities(const string& tablename, uid_t uid, Entity&) {
		Transaction txn;
		Storage* pstorage = get_table(tablename);
		if (pstorage == NULL) return false;
		try {
			OctetsStream key; key << uid;
			pstorage->del(key, txn);
		} catch (...) {
			txn.abort();
			return false;
		}	
		return true;
	}	

	bool YChatDB::getFriends(uid_t uid, vector<Friend>& friends) {
		return getEntities(friend_tablename, uid, friends);
	}

	Friend YChatDB::getFriend(uid_t uid, uid_t fuid) {
		vector<Friend> friends;
		if (!getFriends(uid, friends)) return null_friend;
		vector<Friend>::iterator iter;
		for (iter = friends.begin(); iter != friends.end(); ++iter) 
			if (iter->getUid() == fuid) break;
		if (iter != friends.end()) return *iter;
		return null_friend;
	}

	bool YChatDB::setFriends(uid_t uid, const vector<Friend>& friends) {
		return setEntities(friend_tablename, uid, friends);
	}

	static bool get_friend_users(const vector<Friend>& friends, vector<User>& users) {
		Transaction txn;
		int n_friends = friends.size();
		users.clear();
		for (int i = 0; i < n_friends; i++) {
			User user = YChatDB::getInstance()->getUserByUid(friends[i].getUid());
			if (user.isNull()) return false;
			users.push_back(user);
		}
		return true;
	}

	bool YChatDB::getFriends(uid_t uid, vector<User>& friends) {
		vector<Friend> friends2;
		if (getFriends(uid, friends2) && get_friend_users(friends2, friends)) 
			return true;
		return false;
	}

	bool YChatDB::setFrndreqs(uid_t uid, const vector<FriendRequest>& requests) {
		return setEntities(frndreq_tablename, uid, requests);
	}

	static struct s_frndreq_greater {
		bool operator () (const FriendRequest& req1, const FriendRequest& req2) {
			return req1.getTime() > req2.getTime();
		}	
	} _frndreq_greater;

	bool YChatDB::getFrndreqs(uid_t uid, vector<FriendRequest>& requests) {
		return getSortedEntities(frndreq_tablename, uid, requests, _frndreq_greater);
	}

	static void remove_frndreq(vector<FriendRequest>& requests, uid_t snd_uid) {
		vector<FriendRequest>::iterator iter = requests.begin(); 
		while (iter != requests.end()) {
			if (iter->getSenderUid() == snd_uid)
			   iter = requests.erase(iter);
			else ++iter;	
		}
	}

	bool YChatDB::delFrndreq(uid_t uid, uid_t req_uid) {
		if (uid == 0 || req_uid == 0) return false;
		Transaction txn;
		vector<FriendRequest> requests;
		if (getFrndreqs(uid, requests)) {
			remove_frndreq(requests, req_uid);
			if (!setFrndreqs(uid, requests)) return false;
			return true;
		}
		return false;
	}

	uid_t YChatDB::hasFrndreq(uid_t uid, const string& req_username) {
		vector<FriendRequest> requests;
		if (getFrndreqs(uid, requests)) {
			int n_requests = requests.size();
			for (int i = 0; i < n_requests; i++) { // linear search
				if (requests[i].getSenderName() == req_username) 
					return requests[i].getSenderUid();
			}
		}
		return 0;
	}

	bool YChatDB::hasFriend(uid_t uid, uid_t fuid) {
		if (uid == 0 || fuid == 0) return false;
		vector<Friend> friends;
		if (getFriends(uid, friends)) {
			int n_friends = friends.size();
			for (int i = 0; i < n_friends; i++) {
				if (friends[i].getUid() == fuid) 
					return true;
			}
		}
		return false;
	}

	uid_t YChatDB::hasFriend(uid_t uid, const string& f_alias) {
		if (uid == 0) return 0;
		vector<Friend> friends;
		if (getFriends(uid, friends)) {
			int n_friends = friends.size();
			for (int i = 0; i < n_friends; i++) {
				if (friends[i].getAlias() == f_alias) 
					return friends[i].getUid();
			}
		}
		return 0;
	}
		
	bool YChatDB::accFriend(uid_t snd_uid, const string& snd_username, 
			uid_t rcv_uid, const string& rcv_username) {
		{
			Transaction txn;
			bool flag = doAddFriend(snd_uid, rcv_uid, rcv_username) 
				&& doAddFriend(rcv_uid, snd_uid, snd_username);
			if (!flag) { txn.abort(); return false; }
		}
		Message sysmsg("YChat", time(NULL), 
				snd_username + " accepted your friend-adding request.");
		return addMessage(rcv_uid, sysmsg);
	}

	bool YChatDB::addFriend(uid_t uid, const std::string username,
			uid_t fuid, const string& f_username) {
		if (uid == 0 || fuid == 0) return false;
		Transaction txn;
		vector<FriendRequest> requests;
		if (getFrndreqs(fuid, requests)) {
			requests.push_back(FriendRequest(uid, username, time(NULL)));
			if (!setFrndreqs(fuid, requests)) return false;
			return true;
		}
		return false;
	}

	bool YChatDB::doAddFriend(uid_t uid, uid_t fuid, const string& f_alias) {
		if (uid == 0 || fuid == 0) return false;
		Transaction txn;
		vector<Friend> friends;
		if (getFriends(uid, friends)) {
			friends.push_back(Friend(fuid, f_alias));
			if (!setFriends(uid, friends)) return false;
			return true;
		}
		return false;
	}

	static void remove_friend(vector<Friend>& friends, uid_t uid) {
		vector<Friend>::iterator iter;
		for (iter = friends.begin(); iter != friends.end(); ++iter) {
			if (iter->getUid() == uid) break;
		}
		if (iter != friends.end()) friends.erase(iter);
	}

	bool YChatDB::delFriend(uid_t uid, uid_t fuid) {
		if (uid == 0 || fuid == 0) return false;
		Transaction txn;
		int flag = doDelFriend(uid, fuid) && doDelFriend(fuid, uid);
		if (!flag) txn.abort(); 
		return flag;
	}

	bool YChatDB::doDelFriend(uid_t uid, uid_t fuid) {
		Transaction txn;
		vector<Friend> friends;
		if (getFriends(uid, friends)) {
			remove_friend(friends, fuid);
			if (!setFriends(uid, friends)) return false;
			return true;
		}
		return false;
	}

	bool YChatDB::setMessages(uid_t uid, const vector<Message>& messages) {
		return setEntities(message_tablename, uid, messages);
	}

	static struct s_message_greater {
		bool operator () (const Message& msg1, const Message& msg2) {
			return msg1.getTime() > msg2.getTime();
		}	
	} _message_greater;


	bool YChatDB::getMessages(uid_t uid, vector<Message>& messages) {
		return getSortedEntities(message_tablename, uid, messages, _message_greater);
	}

	bool YChatDB::clrMessages(uid_t uid) {
		return clrEntities(message_tablename, uid, null_message);
	}

	bool YChatDB::addMessage(uid_t uid, const Message& message) {
		Transaction txn;
		vector<Message> messages;
		if (getMessages(uid, messages)) {
			messages.push_back(message);
			//sort(messages.begin(), messages.end(), _message_greater);
			if (setMessages(uid, messages)) return true;
		}
		return false;
	}

	bool YChatDB::addMessage(uid_t snd_uid, const std::string snd_username, 
			uid_t rcv_uid, const std::string& message) {
		/*Friend frnd = getFriend(rcv_uid, snd_uid);
		string username;
		if (frnd.isNull() || frnd.getAlias() == "") {
			User user = getUserByUid(snd_uid);
			if (user.isNull()) return false;
			username = user.getUserName();
		} else username = frnd.getAlias();*/
		return addMessage(rcv_uid, Message(snd_username, time(NULL), message));
	}
};
