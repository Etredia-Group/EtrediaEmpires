module;

#include <steam/isteamnetworkingsockets.h>

export module SteamExtention;
import std;

export namespace ee_network {

    // enum classes needs casting to be provided as int,
    // so use enum with prefixes for Steam API

    enum DisconnectionStatus {
        DISCONNECTION_STATUS_NETWORKING_DESTROY = k_ESteamNetConnectionEnd_AppException_Min,
        DISCONNECTION_STATUS_FREE_DEAD_CONNECTION_LOCALLY,
        DISCONNECTION_STATUS_CLIENT_ACTION,
        DISCONNECTION_STATUS_SERVER_CLOSED,
        DISCONNECTION_STATUS_FAILED_TO_ACCEPT,

        DISCONNECTION_STATUS_NONE = k_ESteamNetConnectionEnd_AppException_Max
    };

    std::string AddressToString(const SteamNetworkingIPAddr &address) {
        std::string addressStr;
        addressStr.resize(SteamNetworkingIPAddr::k_cchMaxString);
        address.ToString(addressStr.data(), addressStr.size(), true);
        return addressStr;
    }

    class SteamNetConfig {
    public:
        using ConfigOptions = std::vector<SteamNetworkingConfigValue_t>;
        ConfigOptions options;

        SteamNetConfig& Set(const ESteamNetworkingConfigValue option, void *ptr) {
            options.push_back(SteamNetworkingConfigValue_t());
            options.back().SetPtr(option, ptr);
            return *this;
        }

        SteamNetConfig& Set(const ESteamNetworkingConfigValue option, const float value) {
            options.push_back(SteamNetworkingConfigValue_t());
            options.back().SetFloat(option, value);
            return *this;
        }

        SteamNetConfig& Set(const ESteamNetworkingConfigValue option, const std::int32_t value) {
            options.push_back(SteamNetworkingConfigValue_t());
            options.back().SetInt32(option, value);
            return *this;
        }

        SteamNetConfig& Set(const ESteamNetworkingConfigValue option, const std::int64_t value) {
            options.push_back(SteamNetworkingConfigValue_t());
            options.back().SetInt64(option, value);
            return *this;
        }

        SteamNetConfig& Set(const ESteamNetworkingConfigValue option, const char *value) {
            options.push_back(SteamNetworkingConfigValue_t());
            options.back().SetString(option, value);
            return *this;
        }
    };

}