#include <iostream>
#include <WinSock2.h>
#include <memory>
#include <windows.h>

#pragma comment(lib, "Ws2_32.lib")

/*
* 1. Write your own guard to manage handle returned by Win32 API function CreateFile (or open(…), if you use Linux)
*/
class HandleGuard
{
private:
    HANDLE handle_;

public:
    explicit HandleGuard(HANDLE handle) : handle_(handle) {}

    HandleGuard(const HandleGuard&) = delete;
    HandleGuard& operator=(const HandleGuard&) = delete;

    HandleGuard(HandleGuard&& other) noexcept : handle_(other.handle_)
    {
        other.handle_ = INVALID_HANDLE_VALUE;
    }

    HandleGuard& operator=(HandleGuard&& other) noexcept
    {
        if (this != &other)
        {
            if (handle_ != INVALID_HANDLE_VALUE)
            {
                CloseHandle(handle_);
            }
            handle_ = other.handle_;
            other.handle_ = INVALID_HANDLE_VALUE;
        }
        return *this;
    }

    ~HandleGuard()
    {
        if (handle_ != INVALID_HANDLE_VALUE)
        {
            CloseHandle(handle_);
        }
    }

    HANDLE Get() const
    {
        return handle_;
    }
};

HandleGuard CreateFileGuard()
{
    HANDLE fileHandle = CreateFile(TEXT("example.txt"), GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (fileHandle != INVALID_HANDLE_VALUE)
    {
        return HandleGuard(fileHandle);
    }
    return HandleGuard(INVALID_HANDLE_VALUE);
}

/*
*2. Implement custom deleter for std::unique_ptr in three different ways to close a SOCKET.
*/
void CloseSocket(SOCKET* sock)
{
    if (sock && *sock != INVALID_SOCKET)
    {
        closesocket(*sock);
        delete sock;
    }
}

// Using lambda
auto DeleterLambda = [](SOCKET* sock)
    {
        if (sock && *sock != INVALID_SOCKET)
        {
            closesocket(*sock);
            delete sock;
        }
    };

// Using function pointer
auto DeleterFunPtr = CloseSocket;

// Using functor
class DeleterFunctor
{
public:
    void operator()(SOCKET* sock)
    {
        if (sock && *sock != INVALID_SOCKET)
        {
            closesocket(*sock);
            delete sock;
        }
    }
};

void UseSocketDeleters() {
    std::unique_ptr<SOCKET, decltype(DeleterLambda)> sock1(new SOCKET(socket(AF_INET, SOCK_STREAM, 0)), DeleterLambda);

    std::unique_ptr<SOCKET, void(*)(SOCKET*)> sock2(new SOCKET(socket(AF_INET, SOCK_STREAM, 0)), CloseSocket);

    std::unique_ptr<SOCKET, DeleterFunctor> sock3(new SOCKET(socket(AF_INET, SOCK_STREAM, 0)));
}

/*
*3. Implement simplified versions std::shared_ptr and std::weak_ptr. Use next interface in your implementation
*/
template <typename Resource>
class SharedPtr;

template <typename Resource>
class WeakPtr;

template <typename Resource>
class SharedPtr
{
private:
    void Clear()
    {
        if (ref_count_ && --(*ref_count_) == 0)
        {
            delete ptr_;
            delete ref_count_;
        }
    }

    Resource* ptr_;
    long* ref_count_;

    friend class WeakPtr<Resource>;

public:
    SharedPtr() : ptr_(nullptr), ref_count_(nullptr) {}

    explicit SharedPtr(Resource* res) : ptr_(res), ref_count_(new long(1)) {}

    SharedPtr(const SharedPtr<Resource>& rhs) : ptr_(rhs.ptr_), ref_count_(rhs.ref_count_)
    {
        if (ref_count_)
        {
            ++(*ref_count_);
        }
    }

    SharedPtr(const WeakPtr<Resource>& rhs)
    {
        if (!rhs.Expired())
        {
            ptr_ = rhs.ptr_;
            ref_count_ = rhs.ref_count_;
            ++(*ref_count_);
        }
        else
        {
            ptr_ = nullptr;
            ref_count_ = nullptr;
        }
    }

    SharedPtr<Resource>& operator=(const SharedPtr<Resource>& rhs)
    {
        if (this != &rhs)
        {
            Clear();
            ptr_ = rhs.ptr_;
            ref_count_ = rhs.ref_count_;
            if (ref_count_)
            {
                ++(*ref_count_);
            }
        }
        return *this;
    }

    ~SharedPtr()
    {
        Clear();
    }

    void Reset()
    {
        Clear();
        ptr_ = nullptr;
        ref_count_ = nullptr;
    }

    void Reset(Resource* res)
    {
        Clear();
        ptr_ = res;
        ref_count_ = new long(1);
    }

    void Swap(SharedPtr<Resource>& rhs)
    {
        std::swap(ptr_, rhs.ptr_);
        std::swap(ref_count_, rhs.ref_count_);
    }

    Resource* Get() const {
        return ptr_;
    }

    Resource& operator*() const {
        return *ptr_;
    }

    Resource* operator->() const {
        return ptr_;
    }

    long UseCount() const {
        return ref_count_ ? *ref_count_ : 0;
    }
};

template<typename Resource>
class WeakPtr
{
private:
    Resource* ptr_;
    long* ref_count_;

    friend class SharedPtr<Resource>;

public:
    WeakPtr() : ptr_(nullptr), ref_count_(nullptr) {}

    WeakPtr(const WeakPtr<Resource>& rhs) : ptr_(rhs.ptr_), ref_count_(rhs.ref_count_) {}

    WeakPtr(const SharedPtr<Resource>& rhs) : ptr_(rhs.ptr_), ref_count_(rhs.ref_count_) {}

    WeakPtr<Resource>& operator=(const WeakPtr<Resource>& rhs)
    {
        if (this != &rhs)
        {
            ptr_ = rhs.ptr_;
            ref_count_ = rhs.ref_count_;
        }
        return *this;
    }

    WeakPtr<Resource>& operator=(const SharedPtr<Resource>& rhs)
    {
        ptr_ = rhs.ptr_;
        ref_count_ = rhs.ref_count_;
        return *this;
    }

    ~WeakPtr() {}

    void Reset()
    {
        ptr_ = nullptr;
        ref_count_ = nullptr;
    }

    void Swap(WeakPtr<Resource>& rhs)
    {
        std::swap(ptr_, rhs.ptr_);
        std::swap(ref_count_, rhs.ref_count_);
    }

    long UseCount() const {
        return ref_count_ ? *ref_count_ : 0;
    }

    bool Expired() const {
        return UseCount() == 0;
    }

    SharedPtr<Resource> Lock() const {
        return Expired() ? SharedPtr<Resource>() : SharedPtr<Resource>(*this);
    }
};