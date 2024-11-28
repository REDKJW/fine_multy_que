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
        std::cout << "500 : ���� ����\n";
        break;

    case 403:
        std::cout << "403 : ���� ����\n";
        break;

    case 400:
        std::cout << "400 : Bad Request\n";
        break;

    case 429:
        std::cout << "429 : ȣ�ⷮ ����\n";
        break;

    case 503:
        std::cout << "503 : API ���� ��";
        break;

    default:
        std::cout << code << " : �ĺ����� ���� �����ڵ�. Ȯ�� �ʿ�\n";
        break;
    }
    return false;
}


//���ڵ� ���ڵ� �Լ� ����
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

    //std::cout << "������ Ȯ�� : " << size << " " << nmemb << " " << totalSize << " ��\n\n";
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

//���ڵ� ���ڵ� �Լ� ��



/*
//json ������ callback �Լ�
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* out)
{
    size_t totalSize = size * nmemb;
    out->append((char*)contents, totalSize);

    //std::cout << "������ Ȯ�� : " << size << " " << nmemb << " " << totalSize << " ��\n\n";
    return totalSize;
}
*/

struct API
{
    std::string key;
    bool init = false;


    //curl ���� ����
    CURL* curl;
    CURLcode res;
    std::string response;
    struct curl_slist* headers = nullptr;

    API()
    {
        if (!initialize())
        {
            std::cout << "API �ʱ�ȭ ����";
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


        //��û ����
        response.clear();
        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) throw std::runtime_error("���� : " + std::string(curl_easy_strerror(res)));

        long response_code;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

        if (!check_error_code(response_code))
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

    std::vector<std::pair<std::string, std::string>> getTeamPlayers(std::string target_puuid, std::string target_match_id)//��ǥ�� �÷��̾��� �Ʊ� 4���� puuid�� ���´�.
    {
        std::vector<std::pair<std::string, std::string>> Player_List;//puuid, name

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
                    std::string name = Utf8ToEuckr(participant["summonerName"].get<std::string>());
                    Player_List.push_back(std::make_pair(id, name));
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


    bool initialize()
    {
        if (init) return true;

        std::cout << "API key : ";
        std::cin >> key;


        curl = curl_easy_init();
        if (!curl)
        {
            std::cout << "curl �ʱ�ȭ ����";
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