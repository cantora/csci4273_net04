#ifndef UO_PAIR_H
#define UO_PAIR_H

#include <utility>
#include <algorithm>

namespace net04 {

template<class T>
class uo_pair : public std::pair<T,T> {
	
	public:
		uo_pair(T x, T y) : std::pair<T,T>(x,y) {}
		
		bool contains(const T &x) const;	
		bool operator== (const uo_pair<T> &other) const;
		bool operator< (const uo_pair<T> &other) const;
		bool operator!= (const uo_pair<T> &other) const;
		bool operator> (const uo_pair<T> &other) const;
}; /* uo_pair */

template<class T>
bool uo_pair<T>::contains(const T &x) const {
	return (this->first == x) || (this->second == x);
}

template<class T>
bool uo_pair<T>::operator== (const uo_pair<T> &other) const {
	return (this->first == other.first && this->second == other.second) || (this->first == other.second && this->second == other.first);
}

template<class T>
bool uo_pair<T>::operator< (const uo_pair<T> &other) const {
	T my_small, my_big, o_small, o_big;
	
	if(*this == other) {
		return false;
	}
	else {
		my_small = min(this->first, this->second);
		my_big = max(this->first, this->second);
		o_small = min(other.first, other.second);
		o_big = max(other.first, other.second);

		if(my_small < o_small) {
			return true;
		}
		else if(my_small > o_small) {
			return false;
		}
		else { // my_small == o_small
			return my_big < o_big;
		}
	}
}

template<class T>
bool uo_pair<T>::operator!= (const uo_pair<T> &other) const {
	return !(*this == other);
}

template<class T>
bool uo_pair<T>::operator> (const uo_pair<T> &other) const {
	return !(*this < other);
}

}; /* net04 */
#endif /* UO_PAIR_H */