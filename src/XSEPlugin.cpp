
RE::FormID lastWorld = 0;
bool reset = false;

struct Hooks
{
	struct Sky_UpdateAurora_HasValue_CurrentWeather
	{
		static bool thunk(RE::BSFixedString* a_currentWeather)
		{
			// Wait for the game to unload aurora by its own accord
			if (!RE::Sky::GetSingleton()->auroraRoot)
				reset = false;

			// Get the current "world"
			RE::FormID currentWorld;
			static const auto player = RE::PlayerCharacter::GetSingleton();
			if (auto worldspace = player->GetWorldspace())
				currentWorld = worldspace->formID;
			else if (auto cell = player->GetParentCell())
				currentWorld = cell->formID;
			else
				return func(a_currentWeather);

			// If the world changed then keep reporting within Sky::UpdateAurora that no auroras exist in current weathers
			if (!lastWorld) {
				lastWorld = currentWorld;
			} else if (lastWorld != currentWorld) {
				lastWorld = currentWorld;
				reset = true;
			}

			return reset ? false : func(a_currentWeather);
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};

	struct Sky_UpdateAurora_HasValue_OutgoingWeather
	{
		static bool thunk(RE::BSFixedString* a_outgoingWeather)
		{
			return reset ? false : func(a_outgoingWeather);
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};

	static void Install()
	{
		logger::info("Installing hooks");
		stl::write_thunk_call<Sky_UpdateAurora_HasValue_CurrentWeather>(REL::VariantID(25689, 26236, 0x3C3A50).address() + REL::Relocate(0x2D, 0x2E, 0x3E));
		stl::write_thunk_call<Sky_UpdateAurora_HasValue_OutgoingWeather>(REL::VariantID(25689, 26236, 0x3C3A50).address() + REL::Relocate(0x48, 0x49, 0x59));
	}
};

void Init()
{
	Hooks::Install();
}

void InitializeLog()
{
#ifndef NDEBUG
	auto sink = std::make_shared<spdlog::sinks::msvc_sink_mt>();
#else
	auto path = logger::log_directory();
	if (!path) {
		util::report_and_fail("Failed to find standard logging directory"sv);
	}

	*path /= fmt::format("log"sv, Plugin::NAME);
	auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);
#endif

#ifndef NDEBUG
	const auto level = spdlog::level::trace;
#else
	const auto level = spdlog::level::trace;
#endif

	auto log = std::make_shared<spdlog::logger>("global log"s, std::move(sink));
	log->set_level(level);
	log->flush_on(level);

	spdlog::set_default_logger(std::move(log));
	spdlog::set_pattern("%v"s);
}

EXTERN_C [[maybe_unused]] __declspec(dllexport) bool SKSEAPI SKSEPlugin_Load(const SKSE::LoadInterface* a_skse)
{
#ifndef NDEBUG
	while (!IsDebuggerPresent()) {};
#endif

	InitializeLog();

	logger::info("Loaded plugin");

	SKSE::Init(a_skse);

	Init();

	return true;
}

EXTERN_C [[maybe_unused]] __declspec(dllexport) constinit auto SKSEPlugin_Version = []() noexcept {
	SKSE::PluginVersionData v;
	v.PluginName(Plugin::NAME.data());
	v.PluginVersion(Plugin::VERSION);
	v.UsesAddressLibrary(true);
	v.HasNoStructUse();
	return v;
}();

EXTERN_C [[maybe_unused]] __declspec(dllexport) bool SKSEAPI SKSEPlugin_Query(const SKSE::QueryInterface*, SKSE::PluginInfo* pluginInfo)
{
	pluginInfo->name = SKSEPlugin_Version.pluginName;
	pluginInfo->infoVersion = SKSE::PluginInfo::kVersion;
	pluginInfo->version = SKSEPlugin_Version.pluginVersion;
	return true;
}
