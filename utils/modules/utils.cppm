export module utils;
export import :allocator;
export import :worker;
export import :buffers;
export import :lockfree;
export import :logger;
export import :coroutines;

export namespace ee_utils {
    struct StringHash {
        using is_transparent = void;

        size_t operator()(const std::string_view sv) const {
            return std::hash<std::string_view>{}(sv);
        }

        size_t operator()(const std::string& s) const {
            return std::hash<std::string>{}(s);
        }
    };

    struct StringEqual {
        using is_transparent = void;

        bool operator()(const std::string_view lhs, const std::string_view rhs) const {
            return lhs == rhs;
        }

        bool operator()(const std::string& lhs, const std::string& rhs) const {
            return lhs == rhs;
        }

        bool operator()(const std::string_view lhs, const std::string& rhs) const {
            return lhs == rhs;
        }

        bool operator()(const std::string& lhs, const std::string_view rhs) const {
            return lhs == rhs;
        }
    };

}
