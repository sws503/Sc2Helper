#include "memibot.h"
#include <fstream>

int fileread(Race enemyrace, std::string mapname);
void filewrite(Race enemyrace, std::string mapname, GameResult result);

int MEMIBot::ReadStats() {
	enemyrace = Race::Random;
	myid = 0;
	for (const auto& info : Observation()->GetGameInfo().player_info) {
		std::cout << "pid:" << info.player_id << std::endl;
		if (info.race_actual == Race::Random) {
			enemyrace = info.race_requested;
		}
		if (info.race_actual != Race::Random) {
			myid = info.player_id;
		}
	}
	return fileread(enemyrace, Observation()->GetGameInfo().map_name);
}

void MEMIBot::WriteStats() {
	GameResult gameresult = GameResult::Undecided;
	for (const auto& result : Observation()->GetResults()) {
		if (result.player_id == myid) {
			gameresult = result.result;
		}
	}
	filewrite(enemyrace, Observation()->GetGameInfo().map_name, gameresult);
}

using namespace std;

//todo: ·£´ýÁ¾Á· °¨Áö, ¸ÊÃß°¡
int fileread(Race enemyrace, string mapname) { //race´Â 1-P 2-T 3-Z ºôµå ³Ñ¹ö ¸®ÅÏ
	int win, lose;
	string tmp;
	double winratio1, winratio2;
	string inputString;
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
			outFile << "Bel'Shir Vestige LE (Void) LE" << endl;
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
			outFile << "Bel'Shir Vestige LE (Void) LE" << endl;
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
			for (int b = 0; b < 4; b++)
				outFile << 1 << endl;
			outFile << "Bel'Shir Vestige LE (Void) LE" << endl;
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

				if (winratio1 >= winratio2)
				{
					inFile.close();
					return 5;
				}
				else {
					inFile.close();
					return 6;
				}
			}
		}
	}

}

//todo : ·£´ýÁ¾Á· °¨Áö, ¸ÊÃß°¡
void filewrite(Race enemyrace, string mapname, GameResult result) {
	string tmp, map;
	int win, win1, lose, lose1;
	string s_arr[35];
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

		ofstream outFile("Zerg.txt");
		outFile << mapname << endl << win << endl << lose << endl << win1 << endl << lose1 << endl;


		for (int a = 0; a < 35; a++)
		{
			outFile << s_arr[a] << endl;
		}
		outFile.close();
	}
}