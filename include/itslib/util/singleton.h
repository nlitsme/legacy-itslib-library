#ifndef _ENGINE_UTIL_SINGLETON_H_
#define _ENGINE_UTIL_SINGLETON_H_

#include <memory>
#include <mutex>

// see http://www.boostcookbook.com/Recipe:/1235044
//
// issues solved in this singleton class:
//   - normal c++ statics initialization is not thread safe
//   - assignment and construction in one statement makes no
//     garantee about the order the operations are performed
//   - locking with a static mutex, would turn this in a
//     chicken+egg problem, this mutex is a singleton too
//   - instantiation occurs when needed ( i.e. this is a lazy singleton )
//
// use:
// class yourclass : public Singleton<yourclass> {
// public:
//     yourclass() { /* init */ }
//     void yourmethod() { }
// };
//
// using your singleton:
//
// yourclass::instance().yourmethod();
template<class T>
class Singleton
{
public:
    static T& instance()
    {
        std::call_once(flag, [&]{ t.reset(new T()); });
        return *t;
    }

protected:
    ~Singleton() {}
     Singleton() {}

private:
    Singleton(const T&);
    const T& operator=( const T& );

    static std::unique_ptr<T> t;
    static std::once_flag flag;

};

template<class T> std::unique_ptr<T> Singleton<T>::t;
template<class T> std::once_flag Singleton<T>::flag;

#endif
