#ifndef __I_ENTITY_H_HEARDER_GUARD__
#define __I_ENTITY_H_HEARDER_GUARD__

#include <string>

namespace YChat {
	class IEntity {
	public:
		//explicit IEntity(int) { }
		//explicit IEntity(const std::string&) { }
		virtual std::string toString() const = 0;
		//virtual bool isNull() const = 0;

/*
		template <class Entity>
		static Entity parse(const std::string&, Entity* pent = NULL);
		
		template <class Entity>
		static std::string toString(const Entity& entity);
*/
	};
};

#endif // IEntity.h
