# RAII-guard

1. Own guard to manage handle returned by Win32 API function CreateFile.
2. Custom deleter for std::unique_ptr in three different ways to close a SOCKET.
3. Simplified versions std::shared_ptr and std::weak_ptr. 
