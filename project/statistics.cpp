#include "memibot.h"
#include <fstream>
#include <sstream>
#include "json/json.h"

//int fileread(Race enemyrace, std::string mapname);
//void filewrite(Race enemyrace, std::string mapname, GameResult result);
//void write(const ObservationInterface* observation,Race enemyrace, std::string map_name);

bool MEMIBot::SetOpponentID(const std::string& id) {
	opponentID = id;
	return true;
}

Race MEMIBot::GetEnemyRace() {
	enemyrace = Race::Random;
	uint32_t playerID = Observation()->GetPlayerID();
	for (const auto& info : Observation()->GetGameInfo().player_info) {
		if (info.player_id != playerID) {
			enemyrace = info.race_requested;
		}
	}
	return enemyrace;
}

int MEMIBot::ReadStats(std::vector<int>& branches) {
	std::string map = Observation()->GetGameInfo().map_name;
	std::string filename = "stats/stats_" + version + ".json";
	std::string oppID = opponentID;
	std::string map3 = map.substr(0, 3);

	int maxbranch;
	if (!branches.empty()) {
		maxbranch = branches[GetRandomInteger(0, (int)branches.size() - 1)];
	}
	else {
		std::cout << "fatal error at readstats" << std::endl;
		exit(1);
	}

	Json::Value root;

	std::ifstream istatfile(filename, std::ifstream::binary);
	if (istatfile.is_open()) {
		Json::CharReaderBuilder rbuilder;

		std::string errs;

		bool parsingSuccessful = Json::parseFromStream(rbuilder, istatfile, &root, &errs);
		if (!parsingSuccessful)
		{
			// report to the user the failure and their locations in the document.
			std::cout << "Failed to parse stats, use random\n"
				<< errs;
		}
		else {
			float maxwr = 0.0f;
			for (int b : branches) {
				int prevwin = root["IDW"][oppID][map3].get(b, 1).asInt();
				int prevall = root["IDA"][oppID][map3].get(b, 1).asInt();
				float curwr = prevwin / (float)prevall;
				if (maxwr < curwr) {
					maxwr = curwr;
					maxbranch = b;
				}
			}
		}
		istatfile.close();
	}
	else {
		std::cout << "Failed to open file, use random\n";
	}
#ifdef debug
	maxbranch = branches[GetRandomInteger(0, (int)branches.size() - 1)];
	std::cout << "Debug mode! Use Random\n";
#endif

	return maxbranch;
}

bool MEMIBot::WriteStats() {
	std::string map = Observation()->GetGameInfo().map_name;
	std::string filename = "stats/stats_" + version + ".json";
	std::string oppID = opponentID;
	std::string map3 = map.substr(0, 3);

	uint32_t playerID = Observation()->GetPlayerID();
	GameResult gameresult = GameResult::Undecided;
	for (const auto& result : Observation()->GetResults()) {
		if (result.player_id == playerID) {
			gameresult = result.result;
		}
	}

	//int racewin = 1;
	//int raceall = 1;
	int prevwin = 1;
	int prevall = 1;

	Json::Value root;
	std::ifstream istatfile(filename, std::ifstream::binary);
	if (istatfile.is_open()) {
		Json::CharReaderBuilder rbuilder;

		std::string errs;

		bool parsingSuccessful = Json::parseFromStream(rbuilder, istatfile, &root, &errs);
		if (!parsingSuccessful)
		{
			// report to the user the failure and their locations in the document.
			std::cout << "Failed to parse stats, use default\n"
				<< errs;
		}
		else {
			prevwin = root["IDW"][oppID][map3].get(branch, 1).asInt();
			prevall = root["IDA"][oppID][map3].get(branch, 1).asInt();
		}
		istatfile.close();
	}
	else {
		std::cout << "Failed to open file, use default\n";
	}
	// statfile >> root;

	root["IDW"][oppID][map3][branch] = prevwin + (gameresult == Win);
	root["IDA"][oppID][map3][branch] = prevall++;

	Json::StreamWriterBuilder wbuilder;

	std::string outputstring = Json::writeString(wbuilder, root);
	std::ofstream ostatfile(filename);
	if (ostatfile.is_open()) {
		ostatfile << outputstring << std::endl;
		ostatfile.close();
		return true;
	}
	else {
		std::cout << "file writing failed!" << std::endl;
		return false;
	}
}
/*
int MEMIBot::ReadStats() {
	enemyrace = Race::Random;
	uint32_t playerID = Observation()->GetPlayerID();
	for (const auto& info : Observation()->GetGameInfo().player_info) {
		if (info.player_id != playerID) {
			enemyrace = info.race_requested;
		}
	}
	std::cout << "EnemyRace" <<enemyrace<< std::endl;
	std::cout << "³» ÇÃ·¹ÀÌ¾î ¾ÆÀÌµð : "<< playerID << std::endl;

	return fileread(enemyrace, Observation()->GetGameInfo().map_name);
}
*/
/*
void MEMIBot::WriteStats() {
	/*
	auto playerID = Observation()->GetPlayerID();
	GameResult gameresult = GameResult::Undecided;
	for (const auto& result : Observation()->GetResults()) {
		if (result.player_id == playerID) {
			gameresult = result.result;
		}
	}
	filewrite(enemyrace, Observation()->GetGameInfo().map_name, gameresult);
	* /

	const ObservationInterface* observation = Observation();
	
	write(observation, enemyrace, Observation()->GetGameInfo().map_name);
}
*/

/*
using namespace std;

//todo: ·£´ýÁ¾Á· °¨Áö, ¸ÊÃß°¡
int fileread(Race enemyrace, std::string mapname) { //race´Â 1-P 2-T 3-Z ºôµå ³Ñ¹ö ¸®ÅÏ
	int win, lose;
	std::string tmp;
	double winratio1, winratio2, winratio3;
	std::string inputString;

	if (enemyrace == Race::Protoss)
	{
		ifstream knFile("Protoss.txt");
		if (knFile.fail()) {
			knFile.close();
			cout << "ÆÄÀÏÀÌ¾ø¾î¿ä.P" << endl;
			ofstream outFile("Protoss.txt");
			
			outFile << "Backwater LE" << endl;
			for (int b = 0; b < 4; b++)
				outFile << 1 << endl;
			outFile << "Bel'Shir Vestige LE (Void)" << endl;
			for (int b = 0; b < 4; b++)
				outFile << 1 << endl;
			outFile << "Blackpink LE" << endl;
			for (int b = 0; b < 4; b++)
				outFile << 1 << endl;
			outFile << "Lost and Found LE" << endl;
			for (int b = 0; b < 4; b++)
				outFile << 1 << endl;
			outFile << "Neon Violet Square LE" << endl;
			for (int b = 0; b < 4; b++)
				outFile << 1 << endl;
			outFile << "Newkirk Precinct TE (Void)" << endl;
			for (int b = 0; b < 4; b++)
				outFile << 1 << endl;
			outFile << "Proxima Station LE" << endl;
			for (int b = 0; b < 4; b++)
				outFile << 1 << endl;
			outFile << "Interloper LE" << endl;
			for (int b = 0; b < 4; b++)
				outFile << 1 << endl;
			outFile.close();
			cout << "ÆÄÀÏ»ý¼º¿Ï·á" << endl;

		}
		ifstream in("P_Result.txt");
		if (in.fail()) {
			in.close();
		}
		else
		{
			string ttt,tp;
			GameResult sult;
			getline(in, ttt);
			if (ttt == "Win")
				sult = GameResult::Win;
			else
				sult = GameResult::Loss;
			getline(in, tp);
			in.close();
			filewrite(Race::Protoss, tp, sult);

		}
		ifstream inFile("Protoss.txt");
		while (1) {
			getline(inFile, inputString);
			if (inputString == mapname)
			{
				getline(inFile, tmp);
				win = std::stoi(tmp);
				getline(inFile, tmp);
				lose = std::stoi(tmp);
				winratio1 = (float)win / (float)lose;//1ºôµå ½Â·ü


				getline(inFile, tmp);
				win = std::stoi(tmp);
				getline(inFile, tmp);
				lose = std::stoi(tmp);
				winratio2 = (float)win / (float)lose;//2ºôµå ½Â·ü

				if (winratio1 >= winratio2)
				{
					inFile.close();
					return 1;
				}
				else
				{
					inFile.close();
					return 2;
				}
			}
		}

	}
	else if (enemyrace == Race::Terran)
	{
		ifstream knFile("Terran.txt");
		if (knFile.fail()) {
			cout << "ÆÄÀÏÀÌ¾ø¾î¿ä. T" << endl;
			knFile.close();
			ofstream outFile("Terran.txt");

			outFile << "Backwater LE" << endl;
			for (int b = 0; b < 4; b++)
				outFile << 1 << endl;
			outFile << "Bel'Shir Vestige LE (Void)" << endl;
			for (int b = 0; b < 4; b++)
				outFile << 1 << endl;
			outFile << "Blackpink LE" << endl;
			for (int b = 0; b < 4; b++)
				outFile << 1 << endl;
			outFile << "Lost and Found LE" << endl;
			for (int b = 0; b < 4; b++)
				outFile << 1 << endl;
			outFile << "Neon Violet Square LE" << endl;
			for (int b = 0; b < 4; b++)
				outFile << 1 << endl;
			outFile << "Newkirk Precinct TE (Void)" << endl;
			for (int b = 0; b < 4; b++)
				outFile << 1 << endl;
			outFile << "Proxima Station LE" << endl;
			for (int b = 0; b < 4; b++)
				outFile << 1 << endl;
			outFile << "Interloper LE" << endl;
			for (int b = 0; b < 4; b++)
				outFile << 1 << endl;
			outFile.close();
			cout << "ÆÄÀÏ»ý¼º¿Ï·á" << endl;
		}
		ifstream in("T_Result.txt");
		if (in.fail()) {
			in.close();
		}
		else
		{
			string ttt;
			GameResult sult;
			getline(in, ttt);
			if (ttt == "Win")
				sult = GameResult::Win;
			else
				sult = GameResult::Loss;



			filewrite(Race::Terran, mapname, sult);
		}

		ifstream inFile("Terran.txt");

		while (1) {
			getline(inFile, inputString);
			if (inputString == mapname)
			{
				getline(inFile, tmp);
				win = std::stoi(tmp);
				getline(inFile, tmp);
				lose = std::stoi(tmp);
				winratio1 = (float)win / (float)lose;//1ºôµå ½Â·ü

				getline(inFile, tmp);
				win = std::stoi(tmp);
				getline(inFile, tmp);
				lose = std::stoi(tmp);
				winratio2 = (float)win / (float)lose;//2ºôµå ½Â·ü

				if (winratio1 >= winratio2)
				{
					inFile.close();
					return 3;
				}
				else
				{
					inFile.close();
					return 4;
				}
			}
		}
	}
	else
	{
		ifstream knFile("Zerg.txt");
		if (knFile.fail()) {
			cout << "ÆÄÀÏÀÌ¾ø¾î¿ä. Z" << endl;
			knFile.close();
			ofstream outFile("Zerg.txt");

			outFile << "Backwater LE" << endl;
			for (int b = 0; b < 6; b++)
				outFile << 1 << endl;
			outFile << "Bel'Shir Vestige LE (Void)" << endl;
			for (int b = 0; b < 6; b++)
				outFile << 1 << endl;
			outFile << "Blackpink LE" << endl;
			for (int b = 0; b < 6; b++)
				outFile << 1 << endl;
			outFile << "Lost and Found LE" << endl;
			for (int b = 0; b < 6; b++)
				outFile << 1 << endl;
			outFile << "Neon Violet Square LE" << endl;
			for (int b = 0; b < 6; b++)
				outFile << 1 << endl;
			outFile << "Newkirk Precinct TE (Void)" << endl;
			for (int b = 0; b < 6; b++)
				outFile << 1 << endl;
			outFile << "Proxima Station LE" << endl;
			for (int b = 0; b < 6; b++)
				outFile << 1 << endl;
			outFile << "Interloper LE" << endl;
			for (int b = 0; b < 6; b++)
				outFile << 1 << endl;
			outFile.close();
			cout << "ÆÄÀÏ»ý¼º¿Ï·á" << endl;
		}
		ifstream in("Z_Result.txt");
		if (in.fail()) {
			in.close();
		}
		else
		{
			string ttt;
			GameResult sult;
			getline(in, ttt);
			if (ttt == "Win")
				sult = GameResult::Win;
			else
				sult = GameResult::Loss;
				
		

			filewrite(Race::Zerg, mapname, sult);
		}

		ifstream inFile("Zerg.txt");
		while (1) {
			getline(inFile, inputString);
			if (inputString == mapname)
			{
				getline(inFile, tmp);
				win = std::stoi(tmp);
				getline(inFile, tmp);
				lose = std::stoi(tmp);
				winratio1 = (float)win / (float)lose;//1ºôµå ½Â·ü

				getline(inFile, tmp);
				win = std::stoi(tmp);
				getline(inFile, tmp);
				lose = std::stoi(tmp);
				winratio2 = (float)win / (float)lose;//2ºôµå ½Â·ü

				getline(inFile, tmp);
				win = std::stoi(tmp);
				getline(inFile, tmp);
				lose = std::stoi(tmp);
				winratio3 = (float)win / (float)lose;//3ºôµå ½Â·ü

				if (winratio1 >= winratio2&&winratio1>=winratio3)
				{
					inFile.close();
					return 5;
				}
				else if(winratio2>=winratio1&&winratio2>=winratio3){
					inFile.close();
					return 6;
				}
				else {
					inFile.close();
					return 7;
				}
			}
		}
	}

}

//todo : ·£´ýÁ¾Á· °¨Áö, ¸ÊÃß°¡
void filewrite(Race enemyrace, std::string mapname, GameResult result) {
	std::string mapname_front3 = mapname.substr(0, 3);


	std::cout << "write part";
	string tmp, map;
	int win, win1, lose, lose1,zwin,zlose;
	string s_arr[35];
	string z_arr[49];
	int count = 0, i = 0;
	if (enemyrace == Race::Protoss)
	{
		ifstream inFile("Protoss.txt");

		while (i<40) {
			getline(inFile, tmp);
			if (mapname == tmp)
			{
				if (result == Win) {
					getline(inFile, tmp);
					win = std::stoi(tmp);
					getline(inFile, tmp);
					lose = std::stoi(tmp);
					getline(inFile, tmp);
					win1 = std::stoi(tmp);
					getline(inFile, tmp);
					lose1 = std::stoi(tmp);
					if (((float)win / (float)lose) >= ((float)win1 / (float)lose1))
					{
						win++;
					}
					else
						win1++;
				}
				else if (result == Loss) {
					getline(inFile, tmp);
					win = std::stoi(tmp);
					getline(inFile, tmp);
					lose = std::stoi(tmp);
					getline(inFile, tmp);
					win1 = std::stoi(tmp);
					getline(inFile, tmp);
					lose1 = std::stoi(tmp);
					if (((float)win / (float)lose) >= ((float)win1 / (float)lose1))
					{
						lose++;
					}
					else
						lose1++;

				}

				i = i + 5;

			}
			else {
				s_arr[count] = tmp;
				count++;
				getline(inFile, s_arr[count]);
				count++;
				getline(inFile, s_arr[count]);
				count++;
				getline(inFile, s_arr[count]);
				count++;
				getline(inFile, s_arr[count]);
				count++;
				//inFile.close();

				i = i + 5;
			}

		}
		inFile.close();

		ofstream outFile("Protoss.txt");
		outFile << mapname << endl << win << endl << lose << endl << win1 << endl << lose1 << endl;


		for (int a = 0; a < 35; a++)
		{
			outFile << s_arr[a] << endl;
		}
		outFile.close();

	}
	else if (enemyrace == Race::Terran)
	{
		ifstream inFile("Terran.txt");

		while (i<40) {
			getline(inFile, tmp);
			if (mapname == tmp)
			{
				if (result == Win) {
					getline(inFile, tmp);
					win = std::stoi(tmp);
					getline(inFile, tmp);
					lose = std::stoi(tmp);
					getline(inFile, tmp);
					win1 = std::stoi(tmp);
					getline(inFile, tmp);
					lose1 = std::stoi(tmp);
					if (((float)win / (float)lose) >= ((float)win1 / (float)lose1))
					{
						win++;
					}
					else
						win1++;
				}
				else if (result == Loss) {
					getline(inFile, tmp);
					win = std::stoi(tmp);
					getline(inFile, tmp);
					lose = std::stoi(tmp);
					getline(inFile, tmp);
					win1 = std::stoi(tmp);
					getline(inFile, tmp);
					lose1 = std::stoi(tmp);
					if (((float)win / (float)lose) >= ((float)win1 / (float)lose1))
					{
						lose++;
					}
					else
						lose1++;

				}

				i = i + 5;

			}
			else {
				s_arr[count] = tmp;
				count++;
				getline(inFile, s_arr[count]);
				count++;
				getline(inFile, s_arr[count]);
				count++;
				getline(inFile, s_arr[count]);
				count++;
				getline(inFile, s_arr[count]);
				count++;
				//inFile.close();

				i = i + 5;
			}

		}
		inFile.close();

		ofstream outFile("Terran.txt");
		outFile << mapname << endl << win << endl << lose << endl << win1 << endl << lose1 << endl;


		for (int a = 0; a < 35; a++)
		{
			outFile << s_arr[a] << endl;
		}
		outFile.close();

	}
	else if (enemyrace == Race::Zerg)
	{
		ifstream inFile("Zerg.txt");

		while (i<56) {
			getline(inFile, tmp);
			if (mapname == tmp)
			{
				if (result == Win) {
					getline(inFile, tmp);
					win = std::stoi(tmp);
					getline(inFile, tmp);
					lose = std::stoi(tmp);
					getline(inFile, tmp);
					win1 = std::stoi(tmp);
					getline(inFile, tmp);
					lose1 = std::stoi(tmp);
					getline(inFile, tmp);
					zwin = std::stoi(tmp);
					getline(inFile, tmp);
					zlose = std::stoi(tmp);

					if (((float)win / (float)lose) >= ((float)win1 / (float)lose1) && ((float)win / (float)lose) >= ((float)zwin / (float)zlose))
					{
						win++;
					}
					else if (((float)win1 / (float)lose1) >= ((float)win / (float)lose) && ((float)win1 / (float)lose1) >= ((float)zwin / (float)zlose))
						win1++;
					else
						zwin++;
				}
				else if (result == Loss) {
					getline(inFile, tmp);
					win = std::stoi(tmp);
					getline(inFile, tmp);
					lose = std::stoi(tmp);
					getline(inFile, tmp);
					win1 = std::stoi(tmp);
					getline(inFile, tmp);
					lose1 = std::stoi(tmp);
					getline(inFile, tmp);
					zwin = std::stoi(tmp);
					getline(inFile, tmp);
					zlose = std::stoi(tmp);
					if (((float)win / (float)lose) >= ((float)win1 / (float)lose1) && ((float)win / (float)lose) >= ((float)zwin / (float)zlose))
					{
						lose++;
					}
					else if (((float)win / (float)lose) >= ((float)win1 / (float)lose1) && ((float)win / (float)lose) >= ((float)zwin / (float)zlose))
						lose1++;
					else
						zlose++;

				}

				i = i + 7;

			}
			else {
				z_arr[count] = tmp;
				count++;
				getline(inFile, z_arr[count]);
				count++;
				getline(inFile, z_arr[count]);
				count++;
				getline(inFile, z_arr[count]);
				count++;
				getline(inFile, z_arr[count]);
				count++;
				getline(inFile, z_arr[count]);
				count++;
				getline(inFile, z_arr[count]);
				count++;
				//inFile.close();

				i = i + 7;
			}

		}
		inFile.close();

		ofstream outFile("Zerg.txt");
		outFile << mapname << endl << win << endl << lose << endl << win1 << endl << lose1 << endl<<zwin<<endl<<zlose<<endl;


		for (int a = 0; a < 49; a++)
		{
			outFile << z_arr[a] << endl;
		}
		outFile.close();
	}
}

void write(const ObservationInterface* observation, Race enemyrace, std::string map_name)
{
	//12ÃÊ¿¡ 260 ÃÊ´ç 21.6
	if (observation->GetGameLoop() % 130 == 0 && observation->GetGameLoop() > 2600)
	{
		size_t Mystructure_num = observation->GetUnits(Unit::Alliance::Self, IsStructure(observation)).size();
		size_t Enemystructure_num = observation->GetUnits(Unit::Alliance::Enemy, IsStructure(observation)).size();
		std::string outfile_name;
		switch (enemyrace) {
		case Protoss:
			outfile_name = "P_Result.txt";
			break;
		case Terran:
			outfile_name = "T_Result.txt";
			break;
		case Zerg:
			outfile_name = "Z_Result.txt";
			break;
		case Random:
		default:
			outfile_name = "R_Result.txt";
			break;
		}
		std::ofstream outFile(outfile_name);
		if (Mystructure_num <= 5)
			outFile << "Loss" << std::endl;
		else if (Enemystructure_num <= 5)
			outFile << "Win" << std::endl;
		else if (Mystructure_num > Enemystructure_num)
			outFile << "Win" << std::endl;
		else
			outFile << "Loss" << std::endl;
		outFile << map_name << std::endl;
		outFile.close();
	}
}
*/
/*
int json()
{
	Json::Value root;   // starts as "null"; will contain the root value after parsing
	//std::cin >> root;
	std::ifstream config_doc("config_doc.json", std::ifstream::binary);
	config_doc >> root;

	// Get the value of the member of root named 'my-encoding', return 'UTF-32' if there is no
	// such member.
	std::string my_encoding = root.get("my-encoding", "UTF-32").asString();

	// Get the value of the member of root named 'my-plug-ins'; return a 'null' value if
	// there is no such member.
	const Json::Value my_plugins = root["my-plug-ins"];
	for (int index = 0; index < my_plugins.size(); ++index)  // Iterates over the sequence elements.
		yourlib::loadPlugIn(my_plugins[index].asString());

	yourlib::setIndentLength(root["my-indent"].get("length", 3).asInt());
	yourlib::setIndentUseSpace(root["my-indent"].get("use_space", true).asBool());

	// ...
	// At application shutdown to make the new configuration document:
	// Since Json::Value has implicit constructor for all value types, it is not
	// necessary to explicitly construct the Json::Value object:
	root["encoding"] = yourlib::getCurrentEncoding();
	root["indent"]["length"] = yourlib::getCurrentIndentLength();
	root["indent"]["use_space"] = yourlib::getCurrentIndentUseSpace();

	// Make a new JSON document with the new configuration. Preserve original comments.
	std::cout << root << "\n";
}

int json2(std::string map, std::string opponentID, GameResult result, int branch)
{
	std::string filename = "stats/stats_" + version + ".json";
	std::string oppID = opponentID;
	std::string map3 = map.substr(0, 3);

	//int racewin = 1;
	//int raceall = 1;
	int prevwin = 1;
	int prevall = 1;

	Json::Value root;
	std::ifstream istatfile(filename, std::ifstream::binary);
	if (istatfile.is_open()) {
		Json::CharReaderBuilder rbuilder;

		std::string errs;

		bool parsingSuccessful = Json::parseFromStream(rbuilder, istatfile, &root, &errs);
		if (!parsingSuccessful)
		{
			// report to the user the failure and their locations in the document.
			std::cout << "Failed to parse stats, use default\n"
				<< errs;
		}
		else {
			prevwin = root["IDW"][oppID][map3].get(branch, 1).asInt();
			prevall = root["IDA"][oppID][map3].get(branch, 1).asInt();
		}
		istatfile.close();
	}
	else {
		std::cout << "Failed to open file, use default\n";
	}
	// statfile >> root;

	root["IDW"][oppID][map3][branch] = prevwin + (result == Win);
	root["IDA"][oppID][map3][branch] = prevall++;

	Json::StreamWriterBuilder wbuilder;

	std::string outputstring = Json::writeString(wbuilder, root);
	std::ofstream ostatfile(filename);
	if (ostatfile.is_open()) {
		ostatfile << outputstring << std::endl;
		ostatfile.close();
		return true;
	}
	else {
		std::cout << "file writing failed!" << std::endl;
		return false;
	}
}
*/
