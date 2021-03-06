#ifndef __I_ENTITY_H_HEARDER_GUARD__
#define __I_ENTITY_H_HEARDER_GUARD__

#include <string>

namespace YChat {
	class IEntity {
	public:
		virtual ~IEntity() { }
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
	
	template<typename Entity>
	class __InternalIEntity : public IEntity {
		Entity (*_parse)(const std::string&);
		std::string (*_toString)(const Entity& entity);
	public: 
		__InternalIEntity() {
			_parse = &Entity::parse;
			_toString = &Entity::toString;
		}
	};
};

#endif // IEntity.h
