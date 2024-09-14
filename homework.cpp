#include "YourImplementation.h"

int main() 
{
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) 
    {
        std::cerr << "WSAStartup failed: " << result << std::endl;
        return 1;
    }

    SharedPtr<int> shared(new int(24));

    WeakPtr<int> weak = shared;

    HandleGuard guard = CreateFileGuard();
    if (guard.Get() != INVALID_HANDLE_VALUE) 
    {
        std::cout << "File handle is valid.\n";
    }
    else {
        std::cout << "Failed to open file.\n";
    }

    UseSocketDeleters();

    WSACleanup();

    return 0;
}
