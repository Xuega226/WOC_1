#ifndef SMARTPTR_H
#define SMARTPTR_H

#include<atomic>

struct normalDeleter
{
    template<typename T>
    void operator()(T* ptr) const {delete ptr;};
};
struct arrayDeleter
{
    template<typename T>
    void operator()(T* ptr) const {delete[] ptr;};
};

template<typename T,typename Deleter>
class uniquePtr
{
public:
    explicit uniquePtr(T* ptr = nullptr,Deleter deleter = Deleter())
    :m_ptr(ptr),m_deleter(deleter){};
    ~uniquePtr()
    {
        if(m_ptr)
        {
            m_deleter(m_ptr);
        }
    }
    uniquePtr(const uniquePtr&) = delete;//禁用拷贝构造,实现独享资源所有权
    uniquePtr& operator=(const uniquePtr&) = delete;//禁用拷贝赋值
    uniquePtr(uniquePtr&& other) noexcept//noexcept保证函数不会抛出异常
    :m_ptr(other.m_ptr),m_deleter(std::move(other.m_deleter))
    {
        other.m_ptr = nullptr;
    }
    uniquePtr& operator=(uniquePtr&& other) noexcept
    {
        if(this != &other)//自我赋值检查
        {
            if(m_ptr)
            {
                m_deleter(m_ptr);
            }
            m_ptr = other.m_ptr;
            m_deleter = std::move(other.m_deleter);
            other.m_ptr = nullptr;
        }
        return *this;//实现链式赋值操作
    };
    T& operator*() const {return *m_ptr;};
    T* operator->() const {return m_ptr;};

    T* get() const {return m_ptr;};
    void reset(T* ptr = nullptr)
    {
        if(m_ptr != ptr)
        {
            m_deleter(m_ptr);
        }
        m_ptr = ptr;
    }


private:
    T* m_ptr;
    Deleter m_deleter;
};

template<typename T,typename Deleter>
class uniquePtr<T[],Deleter>
{
    public:
    explicit uniquePtr(T* ptr = nullptr,Deleter deleter = Deleter())
    :m_ptr(ptr),m_deleter(deleter){};
    ~uniquePtr()
    {
        if(m_ptr)
        {
            m_deleter(m_ptr);
        }
    }
    uniquePtr(const uniquePtr&) = delete;
    uniquePtr& operator=(const uniquePtr&) = delete;
    uniquePtr(uniquePtr&& other) noexcept
    :m_ptr(other.m_ptr),m_deleter(std::move(other.m_deleter))
    {
        other.m_ptr = nullptr;
    }
    uniquePtr& operator=(uniquePtr&& other) noexcept
    {
        if(this != &other)
        {
            if(m_ptr)
            {
                m_deleter(m_ptr);
            }
            m_ptr = other.m_ptr;
            m_deleter = std::move(other.m_deleter);
            other.m_ptr = nullptr;
        }
        return *this;
    };
    T& operator*() const {return *m_ptr;};
    T* operator->() const {return m_ptr;};
    T& operator[](std::size_t index) const {return m_ptr[index];};

    T* get() const {return m_ptr;};
    void reset(T* ptr = nullptr)
    {
        if(m_ptr != ptr)
        {
            m_deleter(m_ptr);
        }
        m_ptr = ptr;
    }

private:
    T* m_ptr;
    Deleter m_deleter;
};

struct contrlBlockBase
{
    std::atomic<long> sharedCount;//使用原子类型保证线程安全的引用计数
    std::atomic<long> weakCount;
    contrlBlockBase():sharedCount(1),weakCount(0){};
    virtual ~contrlBlockBase() = default;
    virtual void destroy() noexcept = 0;
    virtual void delwake() noexcept = 0;
};

template<typename T,typename Deleter = normalDeleter>
struct contrlBlock : contrlBlockBase
{
    T* ptr;
    Deleter deleter;
    contrlBlock(T* ptr,Deleter deleter = Deleter()):ptr(ptr),deleter(deleter){};
    void destroy() noexcept override
    {
        if(ptr)
        {
            deleter(ptr);//因为sharedCount为0，所以资源已经被释放了，这时需要调用deleter来销毁资源
        }
    }
    void delwake() noexcept override
    {
        delete this;//因为weakcount也为0，所以控制块本身也要销毁
    }
};

template<typename T>
class sharedPtr
{
private:
    T* m_ptr;
    contrlBlockBase* m_ctrlBlock;
public:
    explicit sharedPtr(T* ptr = nullptr)
    :m_ptr(ptr),m_ctrlBlock(ptr ? new contrlBlock<T>(ptr) : nullptr){};

    template<typename Deleter>
    sharedPtr(T* ptr,Deleter deleter)
    :m_ptr(ptr),m_ctrlBlock(ptr ? new contrlBlock<T,Deleter>(ptr,std::move(deleter)) : nullptr){};

    sharedPtr(const sharedPtr& other) noexcept
    :m_ptr(other.m_ptr),m_ctrlBlock(other.m_ctrlBlock)
    {
        addSharedCount();
    }

    sharedPtr& operator=(const sharedPtr& other) noexcept
    {
        if(this != &other)
        {
            reset();
            m_ptr = other.m_ptr;
            m_ctrlBlock = other.m_ctrlBlock;
            addSharedCount();
        }
        return *this;
    }

    sharedPtr(sharedPtr&& other) noexcept
    :m_ptr(other.m_ptr),m_ctrlBlock(other.m_ctrlBlock)
    {
        other.m_ptr = nullptr;
        other.m_ctrlBlock = nullptr;
    }

    sharedPtr& operator=(sharedPtr&& other) noexcept
    {
        if(this != &other)
        {
            reset();
            m_ptr = other.m_ptr;
            m_ctrlBlock = other.m_ctrlBlock;
            other.m_ptr = nullptr;
            other.m_ctrlBlock = nullptr;
        }
        return *this;
    }

    ~sharedPtr()
    {
        subSharedCount();
    }

    T& operator*() const {return *m_ptr;};
    T* operator->() const {return m_ptr;};
    T* get() const {return m_ptr;};
    long useCount() const {return m_ctrlBlock ? m_ctrlBlock->sharedCount.load(std::memory_order_acquire) : 0;};

    void reset() noexcept
    {
        subSharedCount();
        m_ptr = nullptr;
        m_ctrlBlock = nullptr;
    }

    private:
    void addSharedCount() noexcept
    {
        if(m_ctrlBlock)
        {
            m_ctrlBlock->sharedCount.fetch_add(1,std::memory_order_relaxed);
        }
    }

    void subSharedCount() noexcept
    {
        if(m_ctrlBlock && m_ctrlBlock->sharedCount.fetch_sub(1,std::memory_order_release) == 1)
        {
            std::atomic_thread_fence(std::memory_order_acquire);
            m_ctrlBlock->destroy();//如果引用计数为0，则调用deleter销毁资源
            if(m_ctrlBlock->weakCount.load(std::memory_order_acquire) == 0)
            {
                m_ctrlBlock->delwake();//如果弱引用计数也为0，则销毁控制块
            }
        }
    }
};

template<typename T>
class weakPtr
{
    private:
    T* m_ptr;
    contrlBlockBase* m_ctrlBlock;
    public:
    weakPtr():m_ptr(nullptr),m_ctrlBlock(nullptr){};
    weakPtr(const sharedPtr<T>& shared)
    :m_ptr(shared.m_ptr),m_ctrlBlock(shared.m_ctrlBlock)
    {
        addWeakCount();
    }

    weakPtr(const weakPtr& other) noexcept
    :m_ptr(other.m_ctrlBlock ? other.m_ptr : nullptr),m_ctrlBlock(other.m_ctrlBlock)//如果other的控制块不为空，则m_ptr指向资源，否则为nullptr
    {
        addWeakCount();
    }

    weakPtr& operator=(const weakPtr& other) noexcept
    {
        if(this != &other)
        {
            reset();
            m_ptr = other.m_ctrlBlock ? other.m_ptr : nullptr;
            m_ctrlBlock = other.m_ctrlBlock;
            addWeakCount();
        }
        return *this;
    }

    void reset() noexcept
    {
        if(m_ctrlBlock)
        {
            m_ctrlBlock->weakCount.fetch_sub(1,std::memory_order_release);
            m_ptr = nullptr;
            m_ctrlBlock = nullptr;
        }
    }
    
    long useCount() const {return m_ctrlBlock ? m_ctrlBlock->sharedCount.load(std::memory_order_acquire) : 0;};
    bool expired() const {return useCount() == 0;};
    sharedPtr<T> lock() const noexcept
    {
        if(expired())
        {
            return sharedPtr<T>();//如果资源已经被销毁，则返回一个空的sharedPtr
        }
        else
        {
            return sharedPtr<T>(*this);//否则返回一个sharedPtr，引用计数会增加
        }
    }

    private:
    void addWeakCount() noexcept
    {
        if(m_ctrlBlock)
        {
            m_ctrlBlock->weakCount.fetch_add(1,std::memory_order_relaxed);
        }
    }
};

template<typename T, typename... Args>
uniquePtr<T,normalDeleter> make_unique(Args&&... args) {
    return uniquePtr<T,normalDeleter>(new T(std::forward<Args>(args)...));
}

template<typename T, typename... Args>
sharedPtr<T> make_shared(Args&&... args) {
    return sharedPtr<T>(new T(std::forward<Args>(args)...));
}


#endif // SMARTPTR_H