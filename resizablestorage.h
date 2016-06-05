#ifndef JWUTIL_RESIZABLESTORAGE_H
#define JWUTIL_RESIZABLESTORAGE_H

#include <assert.h>
#include <initializer_list>

namespace jw_util
{

template <typename DataType>
class ResizableStorage
{
public:
    ResizableStorage()
        : data(0)
        , size(0)
    {}

    ResizableStorage(unsigned int init_size)
        : data(new DataType[init_size])
        , size(init_size)
    {}

    template <typename... UpdatePtrs>
    void resize(unsigned int new_size, UpdatePtrs... ptrs)
    {
        if (new_size <= size) {return;}

        unsigned int new_size_2 = size + (size / 2);
        if (new_size_2 > new_size) {new_size = new_size_2;}

        DataType *new_data = new DataType[new_size];
        for (unsigned int i = 0; i < size; i++)
        {
            new_data[i] = std::move(data[i]);
        }

        update_ptrs(data, new_data, ptrs...);

        delete[] data;

        data = new_data;
        size = new_size;
    }

    DataType *begin() const {return data;}
    DataType *end() const {return data + size;}

private:
    DataType *data;
    unsigned int size;

    template <typename... UpdatePtrs>
    static void update_ptrs(DataType *old_data, DataType *new_data, DataType *&ptr, UpdatePtrs... rest)
    {
        unsigned int offset = ptr - old_data;
        ptr = new_data + offset;
        update_ptrs(old_data, new_data, rest...);
    }

    static void update_ptrs(DataType *old_data, DataType *new_data) {}
};

}

#endif // JWUTIL_RESIZABLESTORAGE_H
