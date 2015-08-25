/**
 *  @author William Cheung
 *  @date   08/24/2015   	
 */

#ifndef __LRU_CACHE_H_HEADER_GUARD__
#define __LRU_CACHE_H_HEADER_GUARD__

#include <list>
#include <map>

#ifdef _REENTRANT_    // for the mutithreading case
#	include "mutex.h" // this source need pthread library
	using GNET::Thread::Mutex;
#	define LRU_CACHE_MUTEX_DECL() Mutex __lru_cache_mutex__
#	define LRU_CACHE_MUTEX_INIT() __lru_cache_mutex__(true)   // recursive mutex
#	define LRU_CACHE_LOCK_GUARD() \
		Mutex::Scoped __lru_cache_lock_guard__(__lru_cache_mutex__)	
#else 
#	define LRU_CACHE_MUTEX_DECL() 
#	define LRU_CACHE_MUTEX_INIT()
#	define LRU_CACHE_LOCK_GUARD()
#endif


namespace YChat {

	/* LRU cache template: 
	 * for using this, type V _MUST_ have its default ctor, copy ctor and 
	 * copy assignment exported if it is a class type 
	 * _DONOT_ use 'const' specifier to declare LRUCache objects */

	template <typename K, typename V, V Deflt = V()> 
	class LRUCache {
		
		typedef std::list<std::pair<K, V> > LT;
		typedef typename LT::iterator I;
		LT _list;

		typedef std::map<K, I> MT;
		typedef typename MT::iterator MI;
		MT _mapping;

		const size_t _capacity;

		LRU_CACHE_MUTEX_DECL();

	private:
		I get_list_tail() {
			I end = _list.end();
			return --end;		
		}
		void move_to_tail(I& iter) {
			if (iter != get_list_tail()) {
				I nitr = ++iter;
				_list.splice(--iter, _list, nitr, _list.end());
			}
		}
	public:
		// is it a good idea to define default capacity ?
		explicit LRUCache(size_t capacity = 1024) 
			: _capacity(capacity), LRU_CACHE_MUTEX_INIT() { }

		const V& get(const K& key) {
			static V v_deflt(Deflt);
			LRU_CACHE_LOCK_GUARD();
			if (_mapping.find(key) != _mapping.end()) {
				I& iter = _mapping[key];
				move_to_tail(iter);	
				return iter->second;
			}
			return v_deflt;
		} 

		void set(const K& key, const V& value) {
			LRU_CACHE_LOCK_GUARD();
			MI iter = _mapping.find(key);
			if (iter == _mapping.end()) {
				_list.push_back(std::make_pair(key, value));
				_mapping[key] = get_list_tail();
				if (size() > _capacity) {
					_mapping.erase(_list.front().first);
					_list.pop_front();
				}
			} else {
				iter->second->second = value;
				move_to_tail(iter->second);
			}
		}

		size_t size() { 
			LRU_CACHE_LOCK_GUARD();
			return _mapping.size(); 
		}
		
		size_t capacity() { return _capacity; }

		void clear() {
			LRU_CACHE_LOCK_GUARD();
			_list.clear();
			_mapping.clear();
		}
	};
};

#endif // LRUCache.h
