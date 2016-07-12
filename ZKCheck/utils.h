/*

 */

#include <algorithm>

#ifndef utilsH
#define utilsH

/*! Helper class to aid in the initializing global const data */
template < typename Cont >
class make_container {
public:
    typedef make_container< Cont > my_type;
    typedef typename Cont::value_type T;

    my_type& operator<< (const T& val) {
        data_.insert(data_.end(), val);
        return *this;
    }
    my_type& operator<< (const Cont& other_container) {
        for (typename Cont::const_iterator it=other_container.begin(); it!=other_container.end(); ++it) {
            data_.insert(data_.end(), *it);
        }
        return *this;
    }
    my_type& operator<< (T&& val) {
        data_.insert(data_.end(), val);
        return *this;
    }
    my_type& operator<< (const char* val) {
        data_.insert(data_.end(), val);
        return *this;
    }
    operator Cont() const {
        return data_;
    }
private:
    Cont data_;
};

#endif
