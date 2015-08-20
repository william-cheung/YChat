#include <iostream>
//#include <iomanip>
#include <string>
#include <sstream>
#include <vector>
#include <algorithm>

#include <csetjmp>

#include <getopt.h>
#include <ctype.h>

#include "thread.h"

#include "kbhit.h"

#include "ClientManager.h"

using namespace std;
using namespace YChat;

//-------------------------------------------------------------------------------------------------

#define terminate()  	kill(getpid(), SIGINT)

#define display_lf() 	(cout << endl << flush)

#define display_message(message) \ 
	do { cout << (message) << flush; } while (0)

#define display_lf_message(message) \
	do { cout << endl << (message) << flush; } while (0)

#define display_message_lf(message) \
	do { cout << (message) << endl << flush; } while (0)

#define display_message_lf_lf(message) \
	do { cout << (message) << endl << endl << flush; } while (0)

#define display_lf_message_lf(message) \
	do { cout << endl << (message) << endl << flush; } while (0)

#define display_command_prompt() \
	do { cout << endl << "> " << flush; } while (0)


void (*my_signal(int signo, void (*func)(int)))(int) {
	struct sigaction act, oact;
	act.sa_handler = func;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
/*
#ifdef SA_INTERRUPT
	act.sa_flags |= SA_INTERRUPT;
#else
	printf("there is no defined SA_INTERRUPT on your platform," 
			"which may cause undefined behaviour !\n");
#endif
*/
	if (sigaction(signo, &act, &oact) < 0) 
		return SIG_ERR;
	return oact.sa_handler;
}

//-------------------------------------------------------------------------------------------------


void 	run_ui					(int argc, char* argv[]);
void	login					(const string& username);
int	do_login				(const string& username, const string& password);
void 	regst					(void);
int	do_regst				(const string& username, const string& password);
void	step_into_main_ui			(void);
void 	start_command_loop			(void); 
int 	parse_and_exec_cmd			(const string& command); 
bool 	exec					(const string& command, const vector<string>& params); 

void	start_chat_mode				(const string& username);

//-------------------------------------------------------------------------------------------------

static pthread_t main_thread_tid;
static ClientManager manager("Client");

static int chat_request_signal = SIGUSR1;
static std::string chat_request_username;

class OnChatReqListener : public OnChatRequestListener {
public:
	void onChatRequest(const std::string& username) {
		// cout << "On Chat Request Received" << endl;
		chat_request_username = username;
		pthread_kill(main_thread_tid, chat_request_signal);
	}
};

static sigjmp_buf jmp_env_for_getline_exception;
static bool is_getline_interrupted = false;

void chat_request_handler(int sig) {
	string message = string("you received a chat request from ") 
		+ chat_request_username + ", accept? (y/n) : ";
	display_lf_message(message);
	
	string input; getline(cin, input);
	if (input.size() == 0 || input[0] == 'y') {
		manager.sendChatRequestAck(chat_request_username, 1); 
		start_chat_mode(chat_request_username);
	} else {
		manager.sendChatRequestAck(chat_request_username, 0);
	}
	
	is_getline_interrupted = true;
	siglongjmp(jmp_env_for_getline_exception, 1); // setjmp in start_command_loop()
}

int main(int argc, char* argv[]) {	
	Conf::GetInstance("client.conf");
	
	ClientManager::OnProtocolProcessListener listener;
	ControlProtocol::setOnProtocolProcessListener(&listener); 

	OnChatReqListener listener2;
	manager.setOnChatRequestListener(&listener2);
	
	Protocol::Client(&manager);
	Pool::AddTask(PollIO::Task::GetInstance());

	Pool::Run(&(Pool::Policy::s_policy), true);	

	main_thread_tid = pthread_self();
	my_signal(chat_request_signal, SIG_IGN);

	run_ui(argc, argv);
	
	return 0;
}

void print_usage() {
	display_message_lf("usage : YChat <-l|--login> <username>");
	display_message_lf("        YChat <-r|--regst>");
}

void run_ui(int argc, char* argv[]) {
	struct option options[] = {
		{"help",	no_argument, 		NULL, 'h'},
		{"login",	required_argument, 	NULL, 'l'},
		{"regst", 	no_argument, 		NULL, 'r'},
		NULL
	};

	int opt = getopt_long(argc, argv, "hl:r", options, NULL);
	if (opt != -1 && opt != ':' && optind == argc) {
		switch (opt) {
		case 'l':	login(optarg); 	break; 
		case 'r':	regst();		break;
		default: 	print_usage();  
		}
	} else print_usage();

	terminate();
}

#define CHAR_BS			0x08		
#define CHAR_DEL		0x7f

bool is_backspace(int ch) {
	return ch == CHAR_BS || ch == CHAR_DEL;
}

int get_password(string& password) {
	int ret = 0;
	_chmod(1);
	password.clear();
	while (true) {
		if (_kbhit()) {
			int ch = _getch();
			if (isalnum(ch) || is_backspace(ch)) {
				if (is_backspace(ch) && password.size() > 0) { 
					password.erase(password.size()-1);
					display_message("\b \b");
				} else {
					password += (char)ch;
					display_message('*');
				}
			} else if (ch == EOF || ch == '\n') {
				ret = 1; break;
			} else { 
				ret = 0; break;
			}
		}
	}
	_chmod(0);
	display_lf();
	return ret;
}

static const string warnning_invalchar_in_password = "invalid character,"
	"valid characters are a-z, A-Z and 0-9";

void login(const string& username) {
	string password;

	while (true) {
		display_message("Password : ");
		if (!get_password(password)) {
			display_message_lf_lf(warnning_invalchar_in_password);
		} else if (password == "") {
			display_message_lf_lf("the password cannot be blank !");
		} else break;
	}	

	int status = do_login(username, password);
	if (status != 0) terminate(); // not ok

	step_into_main_ui();
}

int do_login(const string& username, const string& password) {
	int status = manager.login(username, password);
	switch (status) {
	case ClientManager::YCHAT_OK: 
		return 0;
	case ClientManager::YCHAT_USR_LOGGED:
		display_message_lf_lf("the account has signed in on another console !");
		return 2;	
	case ClientManager::YCHAT_NET_ERROR:
		display_message_lf_lf("network unavailable !");
		return 2; 
	case ClientManager::YCHAT_USR_INVAL:
		display_message_lf_lf("invalid username !");
		return 1; 
	case ClientManager::YCHAT_PAS_INVAL:
		display_message_lf_lf("invalid password !");
		return 1;
	case ClientManager::YCHAT_TIMED_OUT:
		display_message_lf_lf("the login process timed out !");
		return 1;
	default:
		display_message_lf_lf("unknown error from do_login() !");
		return 2;
	}
}

void regst() {
	while (true) {
		string username, password;
		display_message("User Name: ");
		getline(cin, username);
		if (username == "") {
			display_message_lf("the username cannot be blank !");
			continue;
		}

retry_password:
		display_message("Password : ");
		if (!get_password(password)) {
			display_message_lf_lf(warnning_invalchar_in_password);
			goto retry_password;
		} else if (password == "") {
			display_message_lf("the password cannot be blank !");
			goto retry_password;
		} 

		int status = do_regst(username, password);
		if (status == 0) break;            // ok
		else if (status == 2) terminate(); // fatal errot
		// retry if status equals 1
	}

	step_into_main_ui();
}

int do_regst(const string& username, const string& password) {
	int status = manager.regst(username, password);
	switch (status) {
	case ClientManager::YCHAT_OK:
		return 0;
	case ClientManager::YCHAT_USR_EXIST:
		display_message_lf_lf("the username has been registered !"); 
		return 1;
	case ClientManager::YCHAT_TIMED_OUT:
		display_message_lf_lf("registration timed out !");
		return 2;
	case ClientManager::YCHAT_NET_ERROR:
		display_message_lf_lf("network unavailable !");
		return 2;
	default:
		display_message_lf_lf("unknown error from do_regst() !");
		return 2;
	}
}

void step_into_main_ui() {
	display_lf_message_lf("Welcome to YChat 0.0.1");
	start_command_loop();
}

enum {
	CMD_NORM, CMD_INVAL, CMD_EXIT,
};

void start_command_loop() {
	string line;

	int jmp_ret = sigsetjmp(jmp_env_for_getline_exception, 1);
	if (jmp_ret == 1) goto after_interrupted_getline;
	
	display_command_prompt();	
	my_signal(chat_request_signal, chat_request_handler);

	while (getline(cin, line)) {
		my_signal(chat_request_signal, SIG_IGN);  

	after_interrupted_getline:		
		if (is_getline_interrupted) {
			is_getline_interrupted = false;
		} else {	
			int status = parse_and_exec_cmd(line); 
			if (status == CMD_INVAL) {
				display_message_lf("invalid command, type 'help' for help !");
			} else if (status == CMD_EXIT) break;

		}
	
		display_command_prompt();
		my_signal(chat_request_signal, chat_request_handler);
	}
}

void clean_params(vector<string>& params) {
	params.erase(remove(params.begin(), params.end(), ""), params.end());
}

int parse_and_exec_cmd(const string& command) {
	string cmd_str;
	vector<string> params;
	istringstream iss(command);
	iss >> cmd_str;
	while (iss) {
		string param;
		if (iss >> param)
			params.push_back(param);
	}
	clean_params(params);

	if (cmd_str == "") return CMD_NORM;
	else if (cmd_str == "exit") return CMD_EXIT;
	else if (exec(cmd_str, params)) return CMD_NORM;
	return CMD_INVAL;
}


// ------------------------------------------------------------------------------------------------------

void 	chat_with				(const vector<string>&); 
void	list_all_friends			(const vector<string>&);
void 	add_friend				(const vector<string>&);
void	del_friend				(const vector<string>&);
void	list_all_frndreqs			(const vector<string>&);
void	accept_frndreq				(const vector<string>&);
void 	leave_message				(const vector<string>&);
void	list_all_messages			(const vector<string>&);
void	clear_messages				(const vector<string>&);
void	display_help				(const vector<string>&);	


#define MAX_SNDCMD		8

typedef void (*command_handler)(const vector<string>&);

struct secondary_command {
	string command_name;
	command_handler handler;
	int n_params;
	string description;
};

struct internal_command {
	string command_name;
	int n_sndcmds;
	struct secondary_command sndcmds[MAX_SNDCMD];
};

struct internal_command inter_cmds[] = {
	{"chatwith", 	1, 	{{"", 		chat_with,		1, 	"chatwith USERNAME         \tstart chating with USERNAME"}}},
	{"friend",	5, 	{{"all", 	list_all_friends,	0, 	"friend   all              \tlist all your friends"}, 
				 {"add", 	add_friend,		1, 	"         add USERNAME     \tsend a friend-adding request to USERNAME"},
				 {"del", 	del_friend,		1,	"         del USERNAME     \tdelete USERNAME from your friend list"},
				 {"req", 	list_all_frndreqs,  	0,	"         req              \tlist all friend-adding requests"},
				 {"acc", 	accept_frndreq,     	1,      "         acc UESRNAME     \taccept friend-adding request from USERNAME"}}},
	{"leavmsg",	1, 	{{"",		leave_message,		2,	"leavmsg  USERNAME MESSAGE \tleave USERNAME a message; e.g. leavmsg alice \"hello, alice\""}}},
	{"message",	2, 	{{"all",	list_all_messages,	0,	"message  all              \tlist all messages sent to you using \'leavmsg\' command"},
				 {"clr",	clear_messages,		0,	"         clr              \tclear messages sent to you"}}},
	{"help",	1, 	{{"", 		display_help,		0, 	"help                      \tdisplay this help"}}},
	{"exit",	1, 	{{"",		NULL,			0,      "exit                      \texit YChat"}}},
};

bool exec(const string& cmd, const vector<string>& params) {
	int n = sizeof(inter_cmds) / sizeof(internal_command);
	for (int i = 0; i < n; i++) {
		if (cmd == inter_cmds[i].command_name) 
			for (int j = 0; j < inter_cmds[i].n_sndcmds; j++) {
				struct secondary_command scmd = inter_cmds[i].sndcmds[j];
				if (scmd.command_name == "" && params.size() >= scmd.n_params) {
					if (scmd.handler != NULL) {
						scmd.handler(params); 
						return true;
					}
				} else if (params.size() > 0 && scmd.command_name == params[0]) {
					if (scmd.n_params <= params.size() - 1 && scmd.handler != NULL) {
						vector<string> real_params(params.begin()+1, params.end());
						scmd.handler(real_params);
						return true;
					}
				}	
			}
	}
	return false;
}

void chat_with(const vector<string>& params) {
	const string& username = params[0];
	int status = manager.chatWith(username);
	switch (status) {
	case ClientManager::YCHAT_USR_OFFLINE:
		display_message_lf(string("the user '") + username + "' is offline !");
		break;
	case ClientManager::YCHAT_USR_AVAILABLE:
		start_chat_mode(username);
		break;
	case ClientManager::YCHAT_USR_BUSY:
		display_message_lf(string("the user '") + username + "' is busy now !");
		break;
	case ClientManager::YCHAT_USR_NOTFRIEND:
		display_message_lf(string("the user '") + username + "' is not your friend !");
		break;
	case ClientManager::YCHAT_USR_INVAL:
		display_message_lf("invalid user name or the user does not exist !");
		break;
	case ClientManager::YCHAT_REQ_FAILED:
		display_message_lf("chat request failed ! please try again !");
		break;
	default: 
		display_message_lf("unknown error from chat_with() !");
	} 
}

bool is_exit_in_chat_mode(const string& text) {
	if (text == "\\exit") return true;
	return false;
}

void show_messages_from_peer(const std::string& peer_name, vector<string>& messages, 
		Mutex& messages_mutex) {
	Mutex::Scoped mtx(messages_mutex);
	int n_msgs = messages.size();
	for (int i = 0; i < n_msgs; i++) {
		display_lf_message_lf(string("> chat > [") + peer_name + "] " + messages[i]);
	}
	messages.clear();
}

void start_chat_mode(const string& username) {
	Mutex messages_mutex("start_chat_mode::messages_mutex");
	vector<string> messages;
	ClientManager::OnMessageReceiveListener listener(username, messages, messages_mutex);
	manager.setOnMessageReceiveListener(&listener);
	
	show_messages_from_peer(username, messages, messages_mutex);
	
	string prompt = "> chat > [You] ";
	display_lf_message(prompt);

	string line;
	while (getline(cin, line)) {
		if (is_exit_in_chat_mode(line)) {
			display_message_lf(string("ending the chat with \'") + username +  "\' !");
			break;
		}	

		show_messages_from_peer(username, messages, messages_mutex);

		if (line.size() > 0) {	
			manager.sendMessage(username, line);
		}
		display_lf_message(prompt);
	}

	manager.onChatExit(username);
}	

void display_friends(const vector<User>& friends) {
	int n_friends = friends.size();
	if (n_friends == 0) {
		display_message_lf("you have no friends in your friend list !");
	} else {
		for (int i = 0; i < n_friends; i++) {
			display_message_lf(friends[i].getUserName());
		}
	}
}

void list_all_friends(const vector<string>&) {
	vector<User> friends;
	int status = manager.getFriendList(friends);
	switch (status) {
	case ClientManager::YCHAT_OK : 
		display_friends(friends); break;
	case ClientManager::YCHAT_CORR_DATA :
	case ClientManager::YCHAT_NET_ERROR :
	case ClientManager::YCHAT_TIMED_OUT :
	case ClientManager::YCHAT_SVR_ERROR :
	default:
		display_message_lf("failed to retrieve your friend list from server !");
	}
}

void add_friend(const vector<string>& params) {
	string username = params[0];
	int status = manager.addFriend(username);
	switch (status) {
	case ClientManager::YCHAT_OK :
		display_message_lf("sending friend-adding request succeeded !");
		break;
	case ClientManager::YCHAT_USR_INVAL :
		display_message_lf("invalid username, the user may already your friend or not exist !");
		break;
	case ClientManager::YCHAT_NET_ERROR :
	case ClientManager::YCHAT_TIMED_OUT :
	case ClientManager::YCHAT_SVR_ERROR :
	default:
		display_message_lf("sending friend-adding request failed !") ;
	}
}

void del_friend(const vector<string>& params) {
	string username = params[0];
	int status = manager.delFriend(username);
	switch (status) {
	case ClientManager::YCHAT_OK :
		display_message_lf("friend deletion succeeded !");
		break;
	case ClientManager::YCHAT_USR_INVAL :
		display_message_lf(string("invalid username or you have no friend named \'") 
				+ username + "\' !");
		break;
	case ClientManager::YCHAT_NET_ERROR :
	case ClientManager::YCHAT_TIMED_OUT :
	case ClientManager::YCHAT_SVR_ERROR :
	default:
		display_message_lf("friend deletion failed !");
	}
}

void display_frndreqs(const vector<FriendRequest>& requests) {
	int n_requests = requests.size();
	if (n_requests == 0) {
		display_message_lf("you have no friend-adding requests !");
		return;
	} 

	for (int i = 0; i < n_requests; i++) {
		char formated_request[128];
		time_t time = requests[i].getTime();
		struct tm* ptime = localtime(&time);
		int len = strftime(formated_request, sizeof(formated_request), 
				"%Y-%m-%d %H:%M", ptime);
		snprintf(formated_request+len, sizeof(formated_request)-len, 
				"    %-24s    %s", requests[i].getSenderName().c_str(), requests[i].getMessage().c_str());
		display_message_lf(string(formated_request));
	}
}

void list_all_frndreqs(const vector<string>&) {
	vector<FriendRequest> requests; 
	int status = manager.getFrndreqList(requests);
	switch (status) {
	case ClientManager::YCHAT_OK:
		display_frndreqs(requests);
		break;
	case ClientManager::YCHAT_NET_ERROR :
	case ClientManager::YCHAT_TIMED_OUT :
	case ClientManager::YCHAT_SVR_ERROR :
	default:
		display_message_lf("failed to retrieve friend-adding requests !") ;
	}
}

void accept_frndreq(const vector<string>& params) {
	string username = params[0];
	int status = manager.acceptFrndreq(username);
	switch (status) {
	case ClientManager::YCHAT_OK :
		display_message_lf(string("you have added \'") + 
				username + "\' as your friend !");
		break;
	case ClientManager::YCHAT_USR_INVAL :
		display_message_lf(string("you have no friend-adding requests from \'") 
				+ username + "\' or he/she is already your friend !");
		break;
	case ClientManager::YCHAT_NET_ERROR :
	case ClientManager::YCHAT_TIMED_OUT :
	case ClientManager::YCHAT_SVR_ERROR :
	default:
		display_message_lf(string("failed to accept the friend-adding request from \'")
				+ username + "\' !");
	}
}

bool trim_message(string& message) {
	int size = message.size();
	if (message[0] == '\"' && size > 1 && message[size-1] == '\"') {
		int lim = size - 2;
		for (int i = 0; i < lim; i++) {
			message[i] = message[i+1];
		}
		message.erase(size - 2);
		return true;
	}
	return false;
}

void leave_message(const vector<string>& params) {
	string username = params[0];
	string message;
	int limit = params.size() - 1;
	for (int i = 1; i < limit; i++) message += params[i] + " ";
	message += params[limit];

	if (!trim_message(message)) {
		display_message_lf("leavmsg : message format error !");
		return;
	}
	if (message.size() == 0) {
		display_message_lf("the message must not be blank !");
		return;
	}
	int status = manager.leaveMessage(username, message);
	switch (status) {
	case ClientManager::YCHAT_OK:
		display_message_lf(string("you have successfully left \'") 
				+ username + "\' a message !");
		break;
	case ClientManager::YCHAT_USR_INVAL:
		display_message_lf(string("the user \'") + username + "\' does not exist !");
		break;
	case ClientManager::YCHAT_TIMED_OUT:
	case ClientManager::YCHAT_NET_ERROR:
	case ClientManager::YCHAT_SVR_ERROR:
	default:
		display_message_lf("failed to send the message !");
	}
}

void display_messages(vector<Message>& messages) {
	int n_messages = messages.size();
	if (n_messages == 0) {
		display_message_lf("there are no messages sent to you !");
		return;
	}

	for (int i = 0; i < n_messages; i++) {
		char formated_message[256];
		time_t time = messages[i].getTime();
		struct tm* ptime = localtime(&time);
		int len = strftime(formated_message, sizeof(formated_message), 
				"%Y-%m-%d %H:%M", ptime);
		snprintf(formated_message+len, sizeof(formated_message)-len, 
				"    %-24s    %s", messages[i].getSenderName().c_str(), messages[i].getText().c_str());
		display_message_lf(string(formated_message));
	}
}

void list_all_messages(const vector<string>&) {
	vector<Message> messages;
	int status = manager.getMessageList(messages);
	switch (status) {
	case ClientManager::YCHAT_OK : 
		display_messages(messages); break;
	case ClientManager::YCHAT_CORR_DATA :
	case ClientManager::YCHAT_NET_ERROR :
	case ClientManager::YCHAT_TIMED_OUT :
	case ClientManager::YCHAT_SVR_ERROR :
	default:
		display_message_lf("failed to retrieve your messages from server !");
	}
}

void clear_messages(const vector<string>&) {
	int status = manager.clearMessages();
	switch (status) {
	case ClientManager::YCHAT_OK : 
		display_message_lf("messages have been cleared !"); 
		break;
	case ClientManager::YCHAT_NET_ERROR :
	case ClientManager::YCHAT_TIMED_OUT :
	case ClientManager::YCHAT_SVR_ERROR :
	default:
		display_message_lf("failed to clear messages !");
	}
}

void print_cmd_usage(int index) {
	internal_command& cmd = inter_cmds[index];
	for (int i = 0; i < cmd.n_sndcmds; i++) {
		secondary_command& scmd = cmd.sndcmds[i];
		cout << scmd.description << endl;
	}
}

void print_ui_usage() {
	int n = sizeof(inter_cmds) / sizeof(internal_command);
	cout << endl;
	string dashed_line(94, '-');
	cout << "+" << dashed_line << "+" << endl << endl;
	for (int i = 0; i < n; i++) {
		print_cmd_usage(i);	
		cout << endl;
	}
	cout << "+" << dashed_line << "+" << endl;
}

void display_help(const vector<string>&) {
	print_ui_usage();
}
