#pragma once

#include <curl.h>
#include <boost/locale.hpp>
#include "json.hpp"
#include <string>
#include <codecvt>
#include <iostream>
#include <vector>

using json = nlohmann::json;

bool check_error_code(long code)
{
    if (code == 200)return true;

    switch (code)
    {
    case 500:
        std::cout << "500 : 서버 오류\n";
        break;

    case 403:
        std::cout << "403 : 권한 없음\n";
        break;

    case 400:
        std::cout << "400 : Bad Request\n";
        break;

    case 429:
        std::cout << "429 : 호출량 과다\n";
        break;

    case 503:
        std::cout << "503 : API 점검 중";
        break;

    default:
        std::cout << code << " : 식별되지 않은 오류코드. 확인 필요\n";
        break;
    }
    return false;
}


//인코딩 디코딩 함수 시작
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

std::string toStr(char* chr)
{
    if (!chr)return std::string();
    if (strlen(chr) >= 3 && strcmp(chr + strlen(chr) - 3, "%00") == 0)
    {
        chr[strlen(chr) - 3] = '\0';
    }
    return std::string(chr, strlen(chr));

}


static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* out)
{
    size_t totalSize = size * nmemb;
    out->append((char*)contents, totalSize);

    //std::cout << "사이즈 확인 : " << size << " " << nmemb << " " << totalSize << " 끝\n\n";
    return totalSize;
}

std::string Utf8ToEuckr(const std::string& euc_str)
{
    // Convert from EUC-KR to UTF-8 using boost::locale::conv::between
    return boost::locale::conv::between(euc_str, "EUC-KR", "UTF-8");
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

//인코딩 디코딩 함수 끝



/*
//json 데이터 callback 함수
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* out)
{
    size_t totalSize = size * nmemb;
    out->append((char*)contents, totalSize);

    //std::cout << "사이즈 확인 : " << size << " " << nmemb << " " << totalSize << " 끝\n\n";
    return totalSize;
}
*/

struct API
{
    std::string key;
    bool init = false;


    //curl 관련 변수
    CURL* curl;
    CURLcode res;
    std::string response;
    struct curl_slist* headers = nullptr;

    API()
    {
        if (!initialize())
        {
            std::cout << "API 초기화 실패";
            delete this;
            return;
        }

    }

    ~API()
    {
        if (curl) curl_easy_cleanup(curl);
        if (headers) curl_slist_free_all(headers);
    }

    std::string getPUUID(std::string target_name, std::string target_tag)
    {
        if (!init) return "Default";

        std::string utf_name =  ConvertToUTF8(target_name);
        std::string utf_tag = ConvertToUTF8(target_tag);

        char* encoded_name = curl_easy_escape(curl, utf_name.c_str(), utf_name.length());
        char* encoded_tag = curl_easy_escape(curl, utf_tag.c_str(), utf_tag.length());

        std::string url = "https://asia.api.riotgames.com/riot/account/v1/accounts/by-riot-id/" + toStr(encoded_name) + "/" + toStr(encoded_tag) + "?api_key=" + key;
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());


        //요청 실행
        response.clear();
        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) throw std::runtime_error("에러 : " + std::string(curl_easy_strerror(res)));

        long response_code;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

        if (!check_error_code(response_code))
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

    std::vector<std::pair<std::string, std::string>> getTeamPlayers(std::string target_puuid, std::string target_match_id)//목표한 플레이어의 아군 4명의 puuid를 얻어온다.
    {
        std::vector<std::pair<std::string, std::string>> Player_List;//puuid, name

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
                    std::string name = Utf8ToEuckr(participant["summonerName"].get<std::string>());
                    Player_List.push_back(std::make_pair(id, name));
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


    bool initialize()
    {
        if (init) return true;

        std::cout << "API key : ";
        std::cin >> key;


        curl = curl_easy_init();
        if (!curl)
        {
            std::cout << "curl 초기화 실패";
            return false;
        }
        headers = curl_slist_append(headers, "Content-Type: application/json");
        headers = curl_slist_append(headers, ("X-Riot-Token: " + key).c_str());

        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

        return init = true;
    }

};