#include "sc2api/sc2_api.h"
#include "sc2lib/sc2_lib.h"
#include "sc2utils/sc2_manage_process.h"
#include "sc2utils/sc2_arg_parser.h"

#include "memibot.h"

#include <iostream>

static const std::string bot_name = "AdeptBot";
static const std::string current_version = "v0.5";

#ifdef DEBUG
static const bool VsHuman = false;
static const int stepsize = 2;
static const bool Realtime = false;
static const bool ControlTest = true;
static const std::vector<std::string> map_names( {
	"CatalystLE",
	"AcidPlantLE",
	"DarknessSanctuary",
	"DreamcatcherLE",
	"LostAndFoundLE",
	"Redshift",
	"16BitLE"
	} );
static const std::string ControlMap = "1pTest.SC2Map"; // 1pTest 2pTest

class Human : public sc2::Agent {
public:
	void OnGameStart() final {
		Debug()->DebugTextOut("Human");
		Debug()->SendDebug();
	}
};


int main(int argc, char* argv[])
{
    Coordinator coordinator;
	if (!coordinator.LoadSettings(argc, argv))
	{
		std::cout << "Unable to find or parse settings." << std::endl;
		return 1;
	}

	coordinator.SetStepSize(stepsize); //Control
								 //게임속도 빠르게 speed faster

	coordinator.SetMultithreaded(true);

	if (Realtime || ControlTest) {
		coordinator.SetRealtime(true);
	}
	// Add the custom bot, it will control the players.
	MEMIBot bot(bot_name, current_version);
	Human human_bot;

	if (VsHuman) {
		coordinator.SetParticipants({
			CreateParticipant(sc2::Race::Protoss, &human_bot),
			CreateParticipant(sc2::Race::Protoss, &bot),
			});
	}
	else {
		coordinator.SetParticipants({
			CreateParticipant(Race::Protoss, &bot),
			CreateComputer(Race::Terran, Difficulty::CheatVision),
			});
	}

	size_t num_maps = map_names.size();
	if (!num_maps) {
		std::cout << "Please set map names!" << std::endl;
		return 1;
	}

	std::string map_name;

	// Start the game.
	coordinator.LaunchStarcraft();

	bool do_break = false;
	while (!do_break) {
		int i = GetRandomInteger(0, static_cast<int>(num_maps - 1));
		map_name = map_names.at(i) + ".SC2Map";
		std::cout << map_names.at(i) << std::endl;

		if (ControlTest)
			map_name = ControlMap;

		if (!coordinator.StartGame(map_name)) {
			break;
		}
		while (coordinator.Update() && !do_break) {
            //sc2::SleepFor(50);
			if (sc2::PollKeyPress()) {
				do_break = true;
			}
		}
	}
    bot.Control()->DumpProtoUsage();

    return 0;
}
#else
#include "LadderInterface.h"
int main(int argc, char* argv[])
{
	RunBot(argc, argv, new MEMIBot(bot_name, current_version), sc2::Race::Protoss);

	return 0;
}
#endif
