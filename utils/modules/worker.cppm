export module utils:worker;
    import :lockfree;
    import :logger;
import std;

export namespace ee_utils
{
    struct WorkerCommand
    {
        std::uint8_t type = 0;
    };
    template<typename T> concept IWorkerCommand = std::is_base_of_v<WorkerCommand, T>;

    template <IWorkerCommand TCommand, std::size_t maxCmdType>
    class Worker
    {
    public:
        void Request(const TCommand &command)
        {
            commands.push(command);
        }

        void Request(const std::uint8_t type)
        {
            commands.push(TCommand {.type = type});
        }

        void ProcessQueue(TCommand &buffer, std::vector<TCommand> &incompletesBuffer)
        {
            while (this->commands.pop(buffer))
            {
                ProcessCommand(buffer);
                if (buffer.type != maxCmdType)
                    incompletesBuffer.push_back(buffer);
            }
            for (TCommand &command : incompletesBuffer)
            {
                this->commands.push(command);
            }
            incompletesBuffer.clear();
        }

    protected:
        std::array<std::function<void(TCommand &command)>, maxCmdType + 1> commandsMapping;
        // TODO: maybe I should use lockfree::queue ?
        boost::lockfree::spsc_queue<TCommand, boost::lockfree::capacity<512>> commands;

        Worker() {};

        void ProcessCommand(TCommand &command)
        {
            if (command.type > maxCmdType || !commandsMapping[command.type])
            {
                LOG(WARNING, "Unhandled command. Type id: %d", command.type);
                command.type = maxCmdType;
                return;
            }

            commandsMapping[command.type](command);
        }
    };

    class ThreadedWorkerBase {
    public:
        std::atomic<bool> isRunning;

        virtual ~ThreadedWorkerBase() {}

        void Start()
        {
            if (isRunning)
            {
                LOG(CRITICAL, "Thread can be started only once before stop");
                throw std::logic_error("Thread can be started only once before stop");
            }

            isRunning.store(true);
            thread = std::jthread([this](std::stop_token stop) {
                this->stoken = &stop;
                Threading();
            });
        }

        void Shutdown()
        {
            if (isRunning)
                thread.request_stop();
            else LOG(WARNING, "Tried to shutdown stopped thread");
        }

    protected:
        std::stop_token *stoken{};
        std::jthread thread;

        virtual void Threading() = 0;
    };
    template<typename T> concept IThreadedWorker = std::is_base_of_v<ThreadedWorkerBase, T>;

    template <IWorkerCommand TCommand, std::size_t maxCmdType>
    class ThreadedWorker : public Worker<TCommand, maxCmdType>, public ThreadedWorkerBase
    {
    protected:
        const wchar_t *threadName;

        explicit ThreadedWorker<TCommand, maxCmdType>(const wchar_t *threadName)
        {
            this->threadName = threadName;
        }

        void Threading()
        {
            Logger::RegisterThread(threadName);
            LOG(INFO, "Thread started");

            Loop();

            ReleaseStop();
        }

        void Loop()
        {
            TCommand commandBuffer{};
            // ReSharper disable once CppTooWideScope
            std::vector<TCommand> incompleteCommands;
            while (!stoken->stop_requested())
            {
                this->ProcessQueue(commandBuffer, incompleteCommands);

                Update();
            }
        }

        virtual void Update() = 0;
        virtual void Release() = 0;

    private:
        void ReleaseStop()
        {
            Release();
            LOG(INFO, "Thread is released");
            Logger::UnregisterThread();
            isRunning.store(false);
        }
    };
}