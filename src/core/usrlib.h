
/**
  ******************************************************************************
  * @file           : usrlib.h
  * @author         : ruixuezhao
  * @brief          : None
  * @attention      : None
  * @date           : 25-5-19
  ******************************************************************************
  */
#ifndef USRLIB_H
#define USRLIB_H

#include <cstdint>

//------------------------------------------------------------------------------
//
//  DESCRIPTON: user namespace for some useful types and functions
//
//
namespace usr
{
    // 定义的 TCbuf 类是一个线程安全的环形缓冲区实现，
    // 主要用于进程间通信数据缓冲。
    // 它提供批量读写、单字节安全存取、缓冲区状态查询等功能，
    // 通过 volatile 计数器和指针循环管理实现高效内存复用，
    // 最大支持256字节容量，被 TChannel 类引用作为内核级通信通道的底层存储容器。
    class TCbuf
    {
    public:
        /// @brief 构造函数，初始化环形缓冲区
        /// @param Address 缓冲区起始地址（需预先分配内存）
        /// @param Size 缓冲区总容量（最大256字节）
        TCbuf(uint8_t* const Address, const uint8_t Size);
        
        /// @brief 批量写入字节数据
        /// @param data 源数据指针
        /// @param Count 要写入的字节数
        /// @return 写入成功返回true，缓冲区空间不足返回false
        bool write(const uint8_t* data, const uint8_t Count);

        /// @brief 批量读取字节数据（自动更新缓冲区状态）
        /// @param data 目标缓冲区指针
        /// @param Count 请求读取的字节数（实际读取数不超过可用数据量）
        void read(uint8_t* const data, const uint8_t Count);

        /// @brief 获取当前存储的字节数
        uint8_t get_count() const { return count; }

        /// @brief 获取剩余可用空间
        uint8_t get_free_size() const { return size - count; }

        /// @brief 随机访问缓冲区内容（不修改缓冲区状态）
        /// @param index 相对偏移量（0=最早写入的字节）
        /// @return 指定位置的字节值
        /// @warning 索引超出实际数据范围时将返回无效值
        uint8_t get_byte(const uint8_t index) const;

        /// @brief 清空缓冲区（重置读写指针和计数器）
        void clear() { count = 0; last = first; }

        /// @brief 安全写入单个字节（线程安全版本）
        /// @return 写入成功返回true，缓冲区满返回false
        bool put(const uint8_t item);

        /// @brief 安全读取单个字节（线程安全版本）
        /// @return 读取的字节值（缓冲区空时返回最后写入的值）
        uint8_t get();

    private:
       //------------------------------------------------------------------------------
       //
       //  DESCRIPTON: For internal purposes
       //
        void push(const uint8_t item); ///< Use this function with care - it doesn't perform free size check
        uint8_t pop();                 ///< Use this function with care - it doesn't perform count check
       //------------------------------------------------------------------------------

    private:
        uint8_t* buf;     ///< 缓冲区内存指针
        uint8_t  size;    ///< 缓冲区总容量
        volatile uint8_t count;  ///< 当前数据量（volatile保证多线程可见性）
        uint8_t  first;   ///< 读指针（下一个要读取的位置）
        uint8_t  last;    ///< 写指针（下一个要写入的位置）
    };
    //------------------------------------------------------------------------------



    //-----------------------------------------------------------------------
    /// @brief 通用环形缓冲区模板（支持任意数据类型）
    /// @tparam T 存储的元素类型
    /// @tparam Size 缓冲区容量（最大65535元素）
    /// @tparam S 索引类型（默认uint8_t，容量>255时应使用uint16_t）
    /// @note 支持前后端双向操作，适用于嵌入式系统的数据缓冲
    template<typename T, uint16_t Size, typename S = uint8_t>
    class ring_buffer
    {
    public:
        ring_buffer() : Count(0), First(0), Last(0) { }

        //----------------------------------------------------------------
        /// @name 数据操作接口
        /// @{
        
        /// @brief 批量写入数据到缓冲区后端
        /// @param data 源数据指针
        /// @param cnt 要写入的元素数量
        /// @return 写入成功返回true，空间不足返回false
        bool write(const T* data, const S cnt);

        /// @brief 从缓冲区前端批量读取数据
        /// @param data 目标缓冲区指针
        /// @param cnt 请求读取的元素数量（实际读取数不超过可用数据量）
        void read(T* const data, const S cnt);

        /// @brief 安全压入元素到缓冲区后端
        /// @return 操作成功返回true，缓冲区满返回false
        bool push_back(const T item);

        /// @brief 安全压入元素到缓冲区前端
        /// @return 操作成功返回true，缓冲区满返回false
        bool push_front(const T item);

        /// @brief 从缓冲区前端弹出元素
        /// @return 弹出的元素（缓冲区空时返回无效值）
        T pop_front();

        /// @brief 从缓冲区后端弹出元素
        /// @return 弹出的元素（缓冲区空时返回无效值） 
        T pop_back();

        bool push(const T item) { return push_back(item); }
        T pop() { return pop_front(); }

        //----------------------------------------------------------------
        /// @name 状态访问接口
        /// @{
        
        /// @brief 获取当前元素数量
        S get_count() const { return Count; }
        
        /// @brief 获取剩余可用空间
        S get_free_size() const { return Size - Count; }
        
        /// @brief 随机访问运算符
        /// @param index 相对偏移量（0=最早进入的元素）
        /// @warning 索引越界时将返回无效引用
        T& operator[](const S index);
        
        /// @brief 清空缓冲区（重置指针和计数器）
        void flush() { Count = 0; Last = First; }
        /// @}

    private:
        //--------------------------------------------------------------
        // 内部实现方法（无安全检查）
        void push_item(const T item);      ///< 后端快速压入
        void push_item_front(const T item); ///< 前端快速压入
        T pop_item();       ///< 前端快速弹出
        T pop_item_back();  ///< 后端快速弹出

    private:
        S  Count;    ///< 当前元素计数
        S  First;    ///< 读指针（前端位置）
        S  Last;     ///< 写指针（后端位置）
        T  Buf[Size];///< 数据存储数组
    };
    //------------------------------------------------------------------
} 
//---------------------------------------------------------------------------


//------------------------------------------------------------------------------
//
//    The ring buffer function-member definitions
//
//
//
template<typename T, uint16_t Size, typename S>
bool usr::ring_buffer<T, Size, S>::write(const T* data, const S cnt)
{
    if( cnt > (Size - Count) )
        return false;

    for(S i = 0; i < cnt; i++)
        push_item(*(data++));

    return true;
}
//------------------------------------------------------------------------------
template<typename T, uint16_t Size, typename S>
void usr::ring_buffer<T, Size, S>::read(T* data, const S cnt)
{
    S nItems = cnt <= Count ? cnt : Count;

    for(S i = 0; i < nItems; i++)
        data[i] = pop_item();
}
//------------------------------------------------------------------------------
template<typename T, uint16_t Size, typename S>
T& usr::ring_buffer<T, Size, S>::operator[](const S index)
{
    S x = First + index;

    if(x < Size)
        return Buf[x];
    else
        return Buf[x - Size];
}

//------------------------------------------------------------------------------
template<typename T, uint16_t Size, typename S>
bool usr::ring_buffer<T, Size, S>::push_back(const T item)
{
    if(Count == Size)
        return false;

    push_item(item);
    return true;
}
//------------------------------------------------------------------------------
template<typename T, uint16_t Size, typename S>
bool usr::ring_buffer<T, Size, S>::push_front(const T item)
{
    if(Count == Size)
        return false;

    push_item_front(item);
    return true;
}
//------------------------------------------------------------------------------
template<typename T, uint16_t Size, typename S>
T usr::ring_buffer<T, Size, S>::pop_front()
{
    if(Count)
        return pop_item();
    else
        return Buf[First];
}
//------------------------------------------------------------------------------
template<typename T, uint16_t Size, typename S>
T usr::ring_buffer<T, Size, S>::pop_back()
{
    if(Count)
        return pop_item_back();
    else
        return Buf[First];
}
//------------------------------------------------------------------------------
template<typename T, uint16_t Size, typename S>
void usr::ring_buffer<T, Size, S>::push_item(const T item)
{
    Buf[Last] = item;
    Last++;
    Count++;

    if(Last == Size)
        Last = 0;
}
//------------------------------------------------------------------------------
template<typename T, uint16_t Size, typename S>
void usr::ring_buffer<T, Size, S>::push_item_front(const T item)
{
    if(First == 0)
        First = Size - 1;
    else
        --First;
    Buf[First] = item;
    Count++;
}
//------------------------------------------------------------------------------
template<typename T, uint16_t Size, typename S>
T usr::ring_buffer<T, Size, S>::pop_item()
{
    T item = Buf[First];

    Count--;
    First++;
    if(First == Size)
        First = 0;

    return item;
}
//------------------------------------------------------------------------------
template<typename T, uint16_t Size, typename S>
T usr::ring_buffer<T, Size, S>::pop_item_back()
{

    if(Last == 0)
        Last = Size - 1;
    else
        --Last;

    Count--;
    return Buf[Last];;
}
//------------------------------------------------------------------------------


#endif // USRLIB_H
