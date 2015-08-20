#ifndef __GNET_TYPES_H__
#define __GNET_TYPES_H__

namespace YChat {
	typedef GNET::Octets   			Octets;
	typedef GNET::Marshal::OctetsStream 	OctetsStream;
	typedef GNET::Protocol 			Protocol;
	typedef GNET::Protocol::Manager		Manager;
	typedef GNET::Conf			Conf;
	typedef GNET::PollIO			PollIO;
	typedef GNET::Thread::Pool		ThreadPool;

	typedef GNET::Protocol::Manager::Session::ID 	sid_t;
	typedef GNET::Protocol::Manager::Session::State session_state_t;
};

#endif 
