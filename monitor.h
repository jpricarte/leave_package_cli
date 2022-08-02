//
// All of this code was copied from
// https://stackoverflow.com/questions/12647217/making-a-c-class-a-monitor-in-the-concurrent-sense
// Don't judge me, I already made a lot
//

#ifndef LEAVE_PACKAGE_MONITOR_H
#define LEAVE_PACKAGE_MONITOR_H

#include <iostream>
#include <mutex>

template<class T>
class Monitor
{
public:
    template<typename ...Args>
    monitor(Args&&... args) : m_cl(std::forward<Args>(args)...){}

    struct monitor_helper
    {
        monitor_helper(monitor* mon) : m_mon(mon), m_ul(mon->m_lock) {}
        T* operator->() { return &m_mon->m_cl;}
        monitor* m_mon;
        std::unique_lock<std::mutex> m_ul;
    };

    monitor_helper operator->() { return monitor_helper(this); }
    monitor_helper ManuallyLock() { return monitor_helper(this); }
    T& GetThreadUnsafeAccess() { return m_cl; }

private:
    T           m_cl;
    std::mutex  m_lock;
};

#endif //LEAVE_PACKAGE_MONITOR_H
