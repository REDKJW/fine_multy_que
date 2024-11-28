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

//인코딩 변환 파트
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
	// 기존 문자열(ANSI) → Wide Char 변환
	int wide_size = MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, nullptr, 0);
	std::wstring wide_str(wide_size, 0);
	MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, &wide_str[0], wide_size);

	// Wide Char → UTF-8 변환
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
//인코딩 디코딩 파트 끝




struct Player
{
	std::string puuid;//해당 플레이어의 puuid

	std::string match1;//각 매치의 id변수
	std::string match2;//각 매치의 id변수
	std::string match3;//각 매치의 id변수

	std::vector<std::string> before; //직전게임에서 같은 팀이었던 puuid들
	std::vector<std::string> bebefore;//그 전 게임에서 같은 팀이었던 puuid들
};

struct API
{
private:
	std::chrono::steady_clock::time_point lastEnterTime;//마지막 입력 시간
	bool init = false;//초기화 여부


	std::string key = "RGAPI-32c02044-97cd-4fe9-a7cb-a9741d3595b2";
	time_t expiration = NULL; //api key의 만료 기간

	std::string puuid = "";//puuid
	std::string my_name = "춘햐추동편의점";//입력될 이름
	std::string tag = "KR1";//입력될 태그
	std::map<std::string, std::string> id2name; //id를 이름으로 변환할 map

	//인코딩 결과
	char* encoded_name = nullptr;//인코딩된 이름
	char* encoded_tag = nullptr;//인코딩된 태그


	std::vector<Player> ally; //아군과 게임 id를 저장할 변수



	//curl 관련 변수들
	CURL* curl = NULL;//curl 핸들
	struct curl_slist* headers;//헤더 리스트
	std::string response;//응답 데이터 저장

	static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp)//json 본문 처리 함수
	{
		size_t realsize = size * nmemb;
		API* api = static_cast<API*>(userp);
		api->response.append(static_cast<char*>(contents), realsize);
		return realsize;
	}

public:
	API() : curl(curl_easy_init()), headers(nullptr)//구조체 생성자
	{
		if (!curl) throw std::runtime_error("cURL 초기화 실패");

		headers = curl_slist_append(headers, "Content-Type: application/json");
		headers = curl_slist_append(headers, ("X-Riot-Token: " + key).c_str());

		// cURL 기본 설정
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

	bool encode()//한국어가 포함된 닉네임과 배틀태그를 인코딩하는 부분
	{
		encoded_name = curl_easy_escape(curl, my_name.c_str(), my_name.length());
		encoded_tag = curl_easy_escape(curl, tag.c_str(), tag.length());

		//인코딩 성공여부 반환
		if (encoded_name != NULL && encoded_tag != NULL)return true;
		else return false;
	}

	std::string getPUUID(char *target_name, char *target_tag)
	{
		std::string url = "https://asia.api.riotgames.com/riot/account/v1/accounts/by-riot-id/" + toStr(target_name) + "/" + toStr(target_tag) + "?api_key=" + key;
		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

		//요청 실행
		response.clear();
		CURLcode res = curl_easy_perform(curl);
		if (res != CURLE_OK) throw std::runtime_error("에러 : " + std::string(curl_easy_strerror(res)));

		//응답코드 확인
		long response_code;
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
		if (response_code != 200)
		{
			MessageBoxA(NULL, ("PUUID 실패 코드 : " + std::to_string(response_code)).c_str(), NULL, MB_OK);
			MessageBox(NULL, s2ws(url).c_str(), NULL, MB_OK);
			return "error";
		}
		
		// 응답 본문에서 PUUID 추출
		try
		{
			json parsed_response = json::parse(response); // JSON 파싱
			if (parsed_response.contains("puuid"))
			{
				return parsed_response["puuid"].get<std::string>();
			}
			else
			{
				throw std::runtime_error("puuid 추출 오류");
			}
		}
		catch (const json::parse_error& e)
		{
			throw std::runtime_error(std::string("puuid 파싱 오류 : ") + e.what());
		}
	}

	std::vector<std::string> getMatch_Id(std::string target_puuid, int n)//target puuid에 해당하는 n회의 게임 전적을 얻어온다
	{

		std::vector<std::string> Match_Ids;//게임 아이디들이 저장되어 반환 될 변수

		std::string url = "https://asia.api.riotgames.com/lol/match/v5/matches/by-puuid/" + target_puuid + "/ids?start=0&count=" + std::to_string(n) + "&?api_key=" + key;
		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

		//요청 실행
		response.clear();
		CURLcode res = curl_easy_perform(curl);
		if (res != CURLE_OK) throw std::runtime_error("에러 : " + std::string(curl_easy_strerror(res)));

		//응답코드 확인
		long response_code;
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
		if (response_code != 200) throw std::runtime_error("요청 실패 : " + std::to_string(response_code));

		// 응답 본문에서 GAME ID 추출
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
			throw std::runtime_error("매치 데이터 파싱 에러 : " + std::string(e.what()));
		}

		return Match_Ids;
	}

	std::vector<std::string> getTeamPlayers(std::string target_puuid, std::string target_match_id)//목표한 플레이어의 아군 4명의 puuid를 얻어온다.
	{
		std::vector<std::string> Player_List;

		std::string url = "https://asia.api.riotgames.com/lol/match/v5/matches/" + target_match_id + "?api_key=" + key;
		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

		response.clear();
		CURLcode res = curl_easy_perform(curl);
		if (res != CURLE_OK) throw std::runtime_error("에러 : " + std::string(curl_easy_strerror(res)));

		//응답코드 확인
		long response_code;
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
		if (response_code != 200) throw std::runtime_error("요청 실패 : " + std::to_string(response_code));

		try
		{
			json matchData = json::parse(response);

			// 타겟 PUUID의 팀 ID 찾기
			int target_team_id = -1;
			for (const auto& participant : matchData["info"]["participants"])
			{
				if (participant["puuid"] == target_puuid)
				{
					target_team_id = participant["teamId"].get<int>();
					break;
				}
			}

			if (target_team_id == -1) throw std::runtime_error("주어진 PUUID의 팀을 찾지 못함");

			// 같은 팀의 PUUID 필터링
			for (const auto& participant : matchData["info"]["participants"])
			{
				if (participant["teamId"] == target_team_id && participant["puuid"] != target_puuid)
				{
					std::string id = participant["puuid"].get<std::string>();
					id2name[id] = utf8_to_enkr(participant["summonerName"].get<std::string>());
					Player_List.push_back(id);
				}
			}

			if (Player_List.size() != 4) throw std::runtime_error("아군이 4명이 아님 : " + std::to_string(Player_List.size()));
		}
		catch (const json::exception& e)
		{
			curl_slist_free_all(headers);
			curl_easy_cleanup(curl);
			throw std::runtime_error("아군 플레이어 파싱 에러 : " + std::string(e.what()));
		}

		return Player_List;
	}

	void Find_Ally(std::vector<std::string> players)//다인큐를 찾는 함수
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
				if (ally[i].match2 == ally[j].match2)//직전 게임에서 아군이었음
				{
					//서로의 puuid를 추가함.
					ally[i].before.push_back(ally[j].puuid);
					ally[j].before.push_back(ally[i].puuid);

					if (ally[i].match3 == ally[j].match3)//그 전 게임에서'도' 아군이었음. 직전 게임아군이 아니면 듀오가 아니겠지.
					{
						ally[i].bebefore.push_back(ally[j].puuid);
						ally[j].bebefore.push_back(ally[i].puuid);
					}
				}
			}
		}

		return;
		
	}

	//id2name은 map으로 구현하자.

	std::string Id2Name(std::string id)
	{
		return id2name[id];
	}

	Player CallBackPlayer(int n)//n번째 플레이어 데이터를 반환
	{
		for (int i=0;i<ally[n].before.size();i++)//직전 게임
		{
			ally[n].before[i] = id2name[ally[n].before[i]];
		}

		for (int i = 0; i < ally[n].bebefore.size(); i++)//그 전 게임
		{
			ally[n].bebefore[i] = id2name[ally[n].bebefore[i]];
		}

		return ally[n];
	}


	void insert_key(std::string key_input, time_t time_input) //key와 만료기간을 최신화하는 함수
	{
		key = key_input;
		expiration = time_input;

		return;
	}

	bool check_expiration()//만료 기한을 확인하는 함수.
	{
		time_t now = time(NULL);
		if (expiration <= now) return false;
		else return true;
	}


	bool initializer()
	{
		my_name = ConvertToUTF8(my_name); //기존
		tag = ConvertToUTF8(tag); //기존

		//인코딩 성공 여부 확인
		if (!encode())
		{
			MessageBox(NULL, TEXT("닉네임 및 태그 인코딩 실패!"), NULL, MB_OK);
			return false;
		}
		//인코딩 성공시 puuid최신화.
		puuid = getPUUID(encoded_name, encoded_tag);
	}


	bool AllInOne()//올인원 함수.
	{
		auto now = std::chrono::steady_clock::now();
		auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - lastEnterTime).count();

		//10초 안에 누르면 무시함
		if (elapsed < 10)
		{
			MessageBox(NULL, TEXT("너무 잦은 요청"), NULL, MB_OK);
			return false;
		}
		init = true;
		lastEnterTime = now; // 마지막 입력 시간 갱신

		std::string Last = getMatch_Id(puuid, 3)[0];//마지막 게임 찾고
		std::vector<std::string> team = getTeamPlayers(puuid, Last);//그 게임의 팀원 4명을 찾음

		Find_Ally(team);//해당 팀원들의 결과를 찾아, 듀오여부를 확인.

		return true;
	}

	bool initcheck()
	{
		return init;
	}

};

