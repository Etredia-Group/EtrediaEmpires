import Network;
import utils;

int main(const int argc, char **argv) {
    Logger::Init(L"/P7.Sink=Console");
    Logger::RegisterThread(L"Main Server");

    ee_network::LobbyServer lobby;

    lobby.Create(7777);

    while (true) {
        lobby.Tick();
    }


    Logger::UnregisterThread();
    Logger::Release();

    return 0;
}