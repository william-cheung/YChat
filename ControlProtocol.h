#ifndef __LOGIN_PROTOCOL_H__
#define __LOGIN_PROTOCOL_H__

#include "protocol.h"

#include "GNETTypes.h"
#include "ProtocolTypes.h"
#include "Listeners.h"

namespace YChat {
	class ControlProtocol : public GNET::Protocol {
		Octets method;
		Octets content;
		
		Protocol* Clone() const { return new ControlProtocol(*this); }

		static OnProtocolProcessListener* p_process_listener;

	public:

		ControlProtocol() : Protocol(PROTOCOL_CONTROL) { }
		
		void setup(const std::string method, const Octets& content) {
			OctetsStream method_os;  method_os << method;
			this->method = method_os;
			this->content = content;
		}

		std::string getMethod() const { 
			OctetsStream method_os(method);
			std::string method_name;
			method_os >> method_name;
			return method_name;
		}

		const Octets& getContent() const {
			return content;
		}

		static void setOnProtocolProcessListener(OnProtocolProcessListener* plistener) {
			p_process_listener = plistener;
		}

		virtual OctetsStream& marshal(OctetsStream& os) const {
			return os << method << content;
		}
		virtual const OctetsStream& unmarshal(const OctetsStream& os) {
			return os >> method >> content;
		}
		virtual int PriorPolicy() { return 1; }
		virtual void Process(Manager* pmanager, sid_t sid) {
			if (p_process_listener != NULL) {
				p_process_listener->onProtocolProcess(pmanager, sid, this);
			}
		}
	};
};

#endif // LoginProtocol.h

