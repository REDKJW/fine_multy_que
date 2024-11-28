#pragma once

#include <string>
#include <ctime>
#include <fstream>
#include <curl/curl.h>
#include "json.hpp"
#include <iostream>
#include <chrono>
#include <boost/locale.hpp>
#include <map>



using json = nlohmann::json;

//���ڵ� ��ȯ ��Ʈ
std::string toStr(char* chr)
{
	if (!chr)return std::string();
	if (strlen(chr) >= 3 && strcmp(chr + strlen(chr) - 3, "%00") == 0)
	{
		chr[strlen(chr) - 3] = '\0';
	}
	return std::string(chr, strlen(chr));
}

std::string ConvertToUTF8(const std::string& str)
{
	// ���� ���ڿ�(ANSI) �� Wide Char ��ȯ
	int wide_size = MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, nullptr, 0);
	std::wstring wide_str(wide_size, 0);
	MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, &wide_str[0], wide_size);

	// Wide Char �� UTF-8 ��ȯ
	int utf8_size = WideCharToMultiByte(CP_UTF8, 0, wide_str.c_str(), -1, nullptr, 0, nullptr, nullptr);
	std::string utf8_str(utf8_size, 0);
	WideCharToMultiByte(CP_UTF8, 0, wide_str.c_str(), -1, &utf8_str[0], utf8_size, nullptr, nullptr);

	return utf8_str;
}

// Function to convert from ENKR (EUC-KR) to UTF-8
std::string enkr_to_utf8(const std::string& euc_kr_string)
{
	try
	{
		return boost::locale::conv::to_utf<char>(euc_kr_string, "EUC-KR");
	}
	catch (const boost::locale::conv::conversion_error& e)
	{
		std::cerr << "ENKR to UTF-8 Conversion error: " << e.what() << std::endl;
		return "";
	}
}

// Function to convert from UTF-8 to ENKR (EUC-KR)
std::string utf8_to_enkr(const std::string& utf8_string)
{
	try
	{
		return boost::locale::conv::from_utf<char>(utf8_string, "EUC-KR");
	}
	catch (const boost::locale::conv::conversion_error& e)
	{
		std::cerr << "UTF-8 to ENKR Conversion error: " << e.what() << std::endl;
		return "";
	}
}

std::wstring s2ws(const std::string& s)
{
	int len;
	int slength = (int)s.length() + 1;
	len = MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, 0, 0);
	wchar_t* buf = new wchar_t[len];
	MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, buf, len);
	std::wstring r(buf);
	delete[] buf;
	return r;
}
//���ڵ� ���ڵ� ��Ʈ ��




struct Player
{
	std::string puuid;//�ش� �÷��̾��� puuid

	std::string match1;//�� ��ġ�� id����
	std::string match2;//�� ��ġ�� id����
	std::string match3;//�� ��ġ�� id����

	std::vector<std::string> before; //�������ӿ��� ���� ���̾��� puuid��
	std::vector<std::string> bebefore;//�� �� ���ӿ��� ���� ���̾��� puuid��
};

struct API
{
private:
	std::chrono::steady_clock::time_point lastEnterTime;//������ �Է� �ð�
	bool init = false;//�ʱ�ȭ ����


	std::string key = "RGAPI-32c02044-97cd-4fe9-a7cb-a9741d3595b2";
	time_t expiration = NULL; //api key�� ���� �Ⱓ

	std::string puuid = "";//puuid
	std::string my_name = "�����ߵ�������";//�Էµ� �̸�
	std::string tag = "KR1";//�Էµ� �±�
	std::map<std::string, std::string> id2name; //id�� �̸����� ��ȯ�� map

	//���ڵ� ���
	char* encoded_name = nullptr;//���ڵ��� �̸�
	char* encoded_tag = nullptr;//���ڵ��� �±�


	std::vector<Player> ally; //�Ʊ��� ���� id�� ������ ����



	//curl ���� ������
	CURL* curl = NULL;//curl �ڵ�
	struct curl_slist* headers;//��� ����Ʈ
	std::string response;//���� ������ ����

	static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp)//json ���� ó�� �Լ�
	{
		size_t realsize = size * nmemb;
		API* api = static_cast<API*>(userp);
		api->response.append(static_cast<char*>(contents), realsize);
		return realsize;
	}

public:
	API() : curl(curl_easy_init()), headers(nullptr)//����ü ������
	{
		if (!curl) throw std::runtime_error("cURL �ʱ�ȭ ����");

		headers = curl_slist_append(headers, "Content-Type: application/json");
		headers = curl_slist_append(headers, ("X-Riot-Token: " + key).c_str());

		// cURL �⺻ ����
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, this);

	}

	~API()
	{
		if (curl) curl_easy_cleanup(curl);
		if (headers) curl_slist_free_all(headers);
	}

	void get_encoded(char **name, char **tag)
	{
		*name = encoded_name;
		*tag = encoded_tag;
		return;
	}

	bool encode()//�ѱ�� ���Ե� �г��Ӱ� ��Ʋ�±׸� ���ڵ��ϴ� �κ�
	{
		encoded_name = curl_easy_escape(curl, my_name.c_str(), my_name.length());
		encoded_tag = curl_easy_escape(curl, tag.c_str(), tag.length());

		//���ڵ� �������� ��ȯ
		if (encoded_name != NULL && encoded_tag != NULL)return true;
		else return false;
	}

	std::string getPUUID(char *target_name, char *target_tag)
	{
		std::string url = "https://asia.api.riotgames.com/riot/account/v1/accounts/by-riot-id/" + toStr(target_name) + "/" + toStr(target_tag) + "?api_key=" + key;
		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

		//��û ����
		response.clear();
		CURLcode res = curl_easy_perform(curl);
		if (res != CURLE_OK) throw std::runtime_error("���� : " + std::string(curl_easy_strerror(res)));

		//�����ڵ� Ȯ��
		long response_code;
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
		if (response_code != 200)
		{
			MessageBoxA(NULL, ("PUUID ���� �ڵ� : " + std::to_string(response_code)).c_str(), NULL, MB_OK);
			MessageBox(NULL, s2ws(url).c_str(), NULL, MB_OK);
			return "error";
		}
		
		// ���� �������� PUUID ����
		try
		{
			json parsed_response = json::parse(response); // JSON �Ľ�
			if (parsed_response.contains("puuid"))
			{
				return parsed_response["puuid"].get<std::string>();
			}
			else
			{
				throw std::runtime_error("puuid ���� ����");
			}
		}
		catch (const json::parse_error& e)
		{
			throw std::runtime_error(std::string("puuid �Ľ� ���� : ") + e.what());
		}
	}

	std::vector<std::string> getMatch_Id(std::string target_puuid, int n)//target puuid�� �ش��ϴ� nȸ�� ���� ������ ���´�
	{

		std::vector<std::string> Match_Ids;//���� ���̵���� ����Ǿ� ��ȯ �� ����

		std::string url = "https://asia.api.riotgames.com/lol/match/v5/matches/by-puuid/" + target_puuid + "/ids?start=0&count=" + std::to_string(n) + "&?api_key=" + key;
		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

		//��û ����
		response.clear();
		CURLcode res = curl_easy_perform(curl);
		if (res != CURLE_OK) throw std::runtime_error("���� : " + std::string(curl_easy_strerror(res)));

		//�����ڵ� Ȯ��
		long response_code;
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
		if (response_code != 200) throw std::runtime_error("��û ���� : " + std::to_string(response_code));

		// ���� �������� GAME ID ����
		try
		{
			json matchIdsJson = json::parse(response);
			for (const auto& matchId : matchIdsJson)
			{
				Match_Ids.push_back(matchId.get<std::string>());
			}
		}
		catch (const json::exception& e)
		{
			curl_slist_free_all(headers);
			throw std::runtime_error("��ġ ������ �Ľ� ���� : " + std::string(e.what()));
		}

		return Match_Ids;
	}

	std::vector<std::string> getTeamPlayers(std::string target_puuid, std::string target_match_id)//��ǥ�� �÷��̾��� �Ʊ� 4���� puuid�� ���´�.
	{
		std::vector<std::string> Player_List;

		std::string url = "https://asia.api.riotgames.com/lol/match/v5/matches/" + target_match_id + "?api_key=" + key;
		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

		response.clear();
		CURLcode res = curl_easy_perform(curl);
		if (res != CURLE_OK) throw std::runtime_error("���� : " + std::string(curl_easy_strerror(res)));

		//�����ڵ� Ȯ��
		long response_code;
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
		if (response_code != 200) throw std::runtime_error("��û ���� : " + std::to_string(response_code));

		try
		{
			json matchData = json::parse(response);

			// Ÿ�� PUUID�� �� ID ã��
			int target_team_id = -1;
			for (const auto& participant : matchData["info"]["participants"])
			{
				if (participant["puuid"] == target_puuid)
				{
					target_team_id = participant["teamId"].get<int>();
					break;
				}
			}

			if (target_team_id == -1) throw std::runtime_error("�־��� PUUID�� ���� ã�� ����");

			// ���� ���� PUUID ���͸�
			for (const auto& participant : matchData["info"]["participants"])
			{
				if (participant["teamId"] == target_team_id && participant["puuid"] != target_puuid)
				{
					std::string id = participant["puuid"].get<std::string>();
					id2name[id] = utf8_to_enkr(participant["summonerName"].get<std::string>());
					Player_List.push_back(id);
				}
			}

			if (Player_List.size() != 4) throw std::runtime_error("�Ʊ��� 4���� �ƴ� : " + std::to_string(Player_List.size()));
		}
		catch (const json::exception& e)
		{
			curl_slist_free_all(headers);
			curl_easy_cleanup(curl);
			throw std::runtime_error("�Ʊ� �÷��̾� �Ľ� ���� : " + std::string(e.what()));
		}

		return Player_List;
	}

	void Find_Ally(std::vector<std::string> players)//����ť�� ã�� �Լ�
	{
		ally.clear();
		for (auto &i : players)
		{
			std::vector<std::string> player_match = getMatch_Id(i, 3);
			ally.push_back(Player{ i, player_match[0], player_match[1], player_match[2]});
		}

		for (int i = 0; i < 3; i++)
		{
			for (int j = i + 1; j < 4; j++)
			{
				if (ally[i].match2 == ally[j].match2)//���� ���ӿ��� �Ʊ��̾���
				{
					//������ puuid�� �߰���.
					ally[i].before.push_back(ally[j].puuid);
					ally[j].before.push_back(ally[i].puuid);

					if (ally[i].match3 == ally[j].match3)//�� �� ���ӿ���'��' �Ʊ��̾���. ���� ���ӾƱ��� �ƴϸ� ����� �ƴϰ���.
					{
						ally[i].bebefore.push_back(ally[j].puuid);
						ally[j].bebefore.push_back(ally[i].puuid);
					}
				}
			}
		}

		return;
		
	}

	//id2name�� map���� ��������.

	std::string Id2Name(std::string id)
	{
		return id2name[id];
	}

	Player CallBackPlayer(int n)//n��° �÷��̾� �����͸� ��ȯ
	{
		for (int i=0;i<ally[n].before.size();i++)//���� ����
		{
			ally[n].before[i] = id2name[ally[n].before[i]];
		}

		for (int i = 0; i < ally[n].bebefore.size(); i++)//�� �� ����
		{
			ally[n].bebefore[i] = id2name[ally[n].bebefore[i]];
		}

		return ally[n];
	}


	void insert_key(std::string key_input, time_t time_input) //key�� ����Ⱓ�� �ֽ�ȭ�ϴ� �Լ�
	{
		key = key_input;
		expiration = time_input;

		return;
	}

	bool check_expiration()//���� ������ Ȯ���ϴ� �Լ�.
	{
		time_t now = time(NULL);
		if (expiration <= now) return false;
		else return true;
	}


	bool initializer()
	{
		my_name = ConvertToUTF8(my_name); //����
		tag = ConvertToUTF8(tag); //����

		//���ڵ� ���� ���� Ȯ��
		if (!encode())
		{
			MessageBox(NULL, TEXT("�г��� �� �±� ���ڵ� ����!"), NULL, MB_OK);
			return false;
		}
		//���ڵ� ������ puuid�ֽ�ȭ.
		puuid = getPUUID(encoded_name, encoded_tag);
	}


	bool AllInOne()//���ο� �Լ�.
	{
		auto now = std::chrono::steady_clock::now();
		auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - lastEnterTime).count();

		//10�� �ȿ� ������ ������
		if (elapsed < 10)
		{
			MessageBox(NULL, TEXT("�ʹ� ���� ��û"), NULL, MB_OK);
			return false;
		}
		init = true;
		lastEnterTime = now; // ������ �Է� �ð� ����

		std::string Last = getMatch_Id(puuid, 3)[0];//������ ���� ã��
		std::vector<std::string> team = getTeamPlayers(puuid, Last);//�� ������ ���� 4���� ã��

		Find_Ally(team);//�ش� �������� ����� ã��, ������θ� Ȯ��.

		return true;
	}

	bool initcheck()
	{
		return init;
	}

};

