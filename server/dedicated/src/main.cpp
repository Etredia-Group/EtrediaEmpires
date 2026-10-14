import Network;
import utils;

int main(const int argc, char **argv) {
    Logger::Init(L"/P7.Sink=Console");
    Logger::RegisterThread(L"Main Server");

    ee_network::Lobby lobby;

    lobby.Create(7777);


    Logger::UnregisterThread();
    Logger::Release();

    return 0;
}