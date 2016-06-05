#ifndef JWUTIL_WORKQUEUE_H
#define JWUTIL_WORKQUEUE_H

#include "workqueuebase.h"

namespace jw_util
{

template <unsigned int num_threads, typename... ArgTypes>
class WorkQueue : public WorkQueueBase<WorkQueue<num_threads, ArgTypes...>, num_threads, ArgTypes...>
{
    typedef WorkQueueBase<WorkQueue<num_threads, ArgTypes...>, num_threads, ArgTypes...> BaseType;
    friend BaseType;

public:
    using BaseType::WorkQueueBase;

private:
    void wait(std::unique_lock<std::mutex> &lock)
    {
        BaseType::conditional_variable.wait(lock);
    }
};

}

#endif // JWUTIL_WORKQUEUE_H
