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
    template <typename T> concept IWorkerCommand = std::is_base_of_v<WorkerCommand, T>;

    template <IWorkerCommand TCommand, std::size_t MaxCmdTypeId, bool IdleDetection = false>
    class Worker
    {
    public:
        void Request(const TCommand &command)
        {
            commands.push(command); //TODO: filled queue case
        }

        void Request(const std::uint8_t type)
        {
            commands.push(TCommand {.type = type}); //TODO: filled queue case
        }

        void ProcessQueue(TCommand &buffer) requires !IdleDetection
        {
            while (this->commands.pop(buffer))
            {
                ProcessCommand(buffer);
            }
        }

        std::uint32_t ProcessQueue(TCommand &buffer) requires IdleDetection
        {
            thread_local std::uint32_t idleCounter = 0;
            if (!this->commands.pop(buffer))
            {
                idleCounter++;
                return idleCounter;
            }
            idleCounter = 0;

            do
            {
                ProcessCommand(buffer);
            } while (this->commands.pop(buffer));

            return 0;
        }

    protected:
        // TODO: replace std::function with more optimized ref
        std::array<std::function<void(TCommand &command)>, MaxCmdTypeId + 1> commandsMapping;
        // TODO: maybe I should use lockfree::queue ? Otherwise make consumer thread explicit for safety
        boost::lockfree::spsc_queue<TCommand, boost::lockfree::capacity<512>> commands;

        Worker() {};

        void ProcessCommand(TCommand &command)
        {
            if (command.type > MaxCmdTypeId || !commandsMapping[command.type])
            {
                LOG(WARNING, "Unhandled command. Type id: %d", command.type);
                command.type = MaxCmdTypeId;
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
            if (isRunning.load())
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
            if (isRunning.load())
                thread.request_stop();
            else LOG(WARNING, "Tried to shutdown stopped thread");
        }

    protected:
        std::stop_token *stoken{};
        std::jthread thread;

        virtual void Threading() = 0;
    };
    template<typename T> concept IThreadedWorker = std::is_base_of_v<ThreadedWorkerBase, T>;

    enum class IdleBackoffMode : std::uint8_t
    {
        DISABLED,
        YIELD
    };

    template <IWorkerCommand TCommand, std::size_t MaxCmdType, IdleBackoffMode IdleBackoffMode = IdleBackoffMode::DISABLED>
    class ThreadedWorker : public Worker<TCommand, MaxCmdType, static_cast<bool>(IdleBackoffMode)>, public ThreadedWorkerBase
    {
    protected:
        const wchar_t *threadName;

        explicit ThreadedWorker(const wchar_t *threadName, std::uint32_t yieldIdleTime = 0)
        {
            this->threadName = threadName;
            this->yieldIdleTime = yieldIdleTime;
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
            while (!stoken->stop_requested())
            {
                if constexpr (IdleBackoffMode == IdleBackoffMode::DISABLED)
                {
                    this->ProcessQueue(commandBuffer);
                }
                else
                {
                    const std::uint32_t idleCounter = this->ProcessQueue(commandBuffer);

                    if (idleCounter > yieldIdleTime)
                        std::this_thread::yield();
                }

                Update();
            }
        }

        virtual void Update() = 0;
        virtual void Release() = 0;

    private:
        std::uint32_t yieldIdleTime;

        void ReleaseStop()
        {
            Release();
            LOG(INFO, "Thread is released");
            Logger::UnregisterThread();
            isRunning.store(false);
        }
    };
}