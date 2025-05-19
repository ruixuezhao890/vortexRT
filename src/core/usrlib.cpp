
/**
  ******************************************************************************
  * @file           : usrlib.cpp
  * @author         : ruixuezhao
  * @brief          : None
  * @attention      : None
  * @date           : 25-5-19
  ******************************************************************************
  */
#include <usrlib.h>

using namespace usr;

//------------------------------------------------------------------------------
//
///   Circular buffer function-member description
//
//
//
TCbuf::TCbuf(uint8_t* const Address, const uint8_t Size) :
        buf(Address),
        size(Size),
        count(0),
        first(0),
        last(0)
{
}
//------------------------------------------------------------------------------
bool TCbuf::write(const uint8_t* data, const uint8_t Count)
{
    if( Count > (size - count) )
        return false;

    for(uint_fast8_t i = 0; i < Count; i++)
        push(*(data++));

    return true;
}
//------------------------------------------------------------------------------
void TCbuf::read(uint8_t* data, const uint8_t Count)
{
    uint_fast8_t N = Count <= count ? Count : count;

    for(uint_fast8_t i = 0; i < N; i++)
        data[i] = pop();
}
//------------------------------------------------------------------------------
uint8_t TCbuf::get_byte(const uint8_t index) const
{
    uint8_t x = first + index;

    if(x < size)
        return buf[x];
    else
        return buf[x - size];
}

//------------------------------------------------------------------------------
bool TCbuf::put(const uint8_t item)
{
    if(count == size)
        return false;

    push(item);
    return true;
}
//------------------------------------------------------------------------------
uint8_t TCbuf::get()
{
    if(count)
        return pop();
    else
        return 0;
}
//------------------------------------------------------------------------------
//
/// \note
/// For internal purposes.
/// Use this function with care - it doesn't perform free size check.
//
void TCbuf::push(const uint8_t item)
{
    buf[last] = item;
    last++;
    count++;

    if(last == size)
        last = 0;
}
//------------------------------------------------------------------------------
//
/// \note
/// For internal purposes.
/// Use this function with care - it doesn't perform free size check.
//
uint8_t TCbuf::pop()
{
    uint8_t item = buf[first];

    count--;
    first++;
    if(first == size)
        first = 0;

    return item;
}
//------------------------------------------------------------------------------
