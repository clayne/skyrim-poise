#include "AVManager.h"
#include "Hooks.h"
#include "Events.h"
#include "Serialization.h"

#include "PoiseAV.h"
#include "PoiseAVHUD.h"

#include "Settings.h"

static void MessageHandler(SKSE::MessagingInterface::Message* message)
{
	switch (message->type) {
	case SKSE::MessagingInterface::kDataLoaded:
		{
			PoiseAV::GetSingleton()->RetrieveFullBodyStaggerFaction();
			auto trueHud = PoiseAVHUD::GetSingleton();
			if (trueHud->g_trueHUD) {
				if (trueHud->g_trueHUD->RequestSpecialResourceBarsControl(SKSE::GetPluginHandle()) == TRUEHUD_API::APIResult::OK) {
					trueHud->g_trueHUD->RegisterSpecialResourceFunctions(SKSE::GetPluginHandle(), PoiseAVHUD::GetCurrentSpecial, PoiseAVHUD::GetMaxSpecial, true);
				}
			}
			break;
		}
	case SKSE::MessagingInterface::kPostLoad:
		{
			auto settings = Settings::GetSingleton();
			settings->LoadSettings();
			if (PoiseAVHUD::GetSingleton()->g_trueHUD && settings->GameSetting.bPoiseUseSpecialBar) {
				PoiseAVHUD::GetSingleton()->g_trueHUD = reinterpret_cast<TRUEHUD_API::IVTrueHUD3*>(TRUEHUD_API::RequestPluginAPI(TRUEHUD_API::InterfaceVersion::V3));
				logger::info("Obtained TrueHUD API");
			} else {
				logger::warn("Failed to obtain TrueHUD API");
			}
			break;
		}
	case SKSE::MessagingInterface::kPreLoadGame:
		auto settings = Settings::GetSingleton();
		settings->LoadSettings();
		break;
	case SKSE::MessagingInterface::kNewGame:
		auto settings = Settings::GetSingleton();
		settings->LoadSettings();
		break;
	default:
		break;
	}
}

void Init()
{
	auto poiseAV = PoiseAV::GetSingleton();
	auto avManager = AVManager::GetSingleton();
	avManager->RegisterActorValue(PoiseAV::g_avName, poiseAV);

	auto messaging = SKSE::GetMessagingInterface();
	messaging->RegisterListener("SKSE", MessageHandler);

	auto serialization = SKSE::GetSerializationInterface();
	serialization->SetUniqueID(Serialization::kUniqueID);
	serialization->SetSaveCallback(Serialization::SaveCallback);
	serialization->SetLoadCallback(Serialization::LoadCallback);
	serialization->SetRevertCallback(Serialization::RevertCallback);

	Hooks::Install();
	Events::Register();
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

	*path /= fmt::format("{}.log"sv, Plugin::NAME);
	auto       sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);
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
	spdlog::set_pattern("[%l] %v"s);
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
	v.PluginName("PluginName");
	v.PluginVersion({ 1, 0, 0, 0 });
	v.UsesAddressLibrary(true);
	return v;
}();

EXTERN_C [[maybe_unused]] __declspec(dllexport) bool SKSEAPI SKSEPlugin_Query(const SKSE::QueryInterface*, SKSE::PluginInfo* pluginInfo)
{
	pluginInfo->name = SKSEPlugin_Version.pluginName;
	pluginInfo->infoVersion = SKSE::PluginInfo::kVersion;
	pluginInfo->version = SKSEPlugin_Version.pluginVersion;
	return true;
}
