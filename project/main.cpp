#include "sc2api/sc2_api.h"
#include "sc2lib/sc2_lib.h"
#include "sc2utils/sc2_manage_process.h"
#include "sc2utils/sc2_arg_parser.h"

#include "memibot.h"

#include <iostream>

static const std::string bot_name = "MEMIBot";
static const std::string current_version = "v1.3";

#ifdef DEBUG
static const bool VsHuman = false;
static const int stepsize = 10;
static const bool realtime = true;

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

	//coordinator.SetStepSize(10); //Control
								 //게임속도 빠르게 speed faster
	coordinator.SetMultithreaded(true);
	coordinator.SetRealtime(realtime);

	// Add the custom bot, it will control the players.
	MEMIBot bot(bot_name, current_version);
	Human human_bot;

	if (VsHuman) {
		coordinator.SetParticipants({
			CreateParticipant(sc2::Race::Terran, &human_bot),
			CreateParticipant(sc2::Race::Protoss, &bot),
			});
	}
	else {
		coordinator.SetParticipants({
			CreateParticipant(Race::Protoss, &bot),
			CreateComputer(Race::Terran, Difficulty::HardVeryHard),
			});
	}

	// Start the game.
	coordinator.LaunchStarcraft();

	bool do_break = false;
	while (!do_break) {
		if (!coordinator.StartGame("AcidPlantLE.SC2Map")) {
			break;
		}
		while (coordinator.Update() && !do_break) {
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
