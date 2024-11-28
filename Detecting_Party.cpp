// Detecting_Party.cpp : 이 파일에는 'main' 함수가 포함됩니다. 거기서 프로그램 실행이 시작되고 종료됩니다.
//

#include <iostream>
#include "Roit_Api.h"
#include <algorithm>
#include <map>

int main()
{
    API api;
    std::string my_name;
    std::string my_tag;

    std::cout << "닉네임 : ";
    std::cin >> my_name;
    std::cout << "배틀 태그 (#은 제외) : ";
    std::cin >> my_tag;

    std::string my_puuid = api.getPUUID(my_name, my_tag);
    std::cout << "PUUID : " << my_puuid << "\n";
    if (my_puuid.compare("Default") == 0)
    {
        std::cout << "PUUID 얻어오기 실패하였음\n";
        return 0;
    }

    int n;
    std::cout << "최근 n회의 게임의 MATCH ID를 얻어옵니다 : ";
    std::cin >> n;
    std::string last_ally = api.getMatch_Id(my_puuid, 1)[0]; //내 최근 전적의 매치 아이디
    std::vector<std::pair<std::string, std::string>> team = api.getTeamPlayers(my_puuid, last_ally);//그 게임의 팀원 4명의 아이디와 닉네임
    
    std::map<std::string, std::string> id2name;

    for (auto& i : team)
    {
        id2name[i.first] = i.second;
    }


    
    std::vector<std::vector<std::string>> match_ids;//[player][n번째판] = 매치 아이디

    for (int i = 0; i < 4; i++)
    {
        match_ids.push_back(api.getMatch_Id(team[i].first, n));//team[i]의 puuid에 따른 n판의 전적
    }
    
    int ally[4][4] = { 0, };

    for (int i = 0; i < 3; i++)
    {
        for (int j = i + 1; j < 4; j++)
        {
            for (int k = 1; k < n; k++)
            {
                if (match_ids[i][k].compare(match_ids[j][k]) == 0)//match id가 같은 팀의 경우 1씩 올려가며 탐색
                {
                    ally[i][j] += 1;
                    ally[j][i] += 1;
                }
                else break;
            }
        }
    }

    for (int i = 0; i < 4; i++)//팀원 순서
    {
        bool check = true;
        std::cout << team[i].second << " : ";
        for (int j = 0; j < 4; j++)
        {
            if (ally[i][j] != 0)
            {
                check = false;
                std::cout << team[i].second << " : " << ally[i][j] << "회, ";
            }
        }
        if (check) std::cout << "다인큐가 아닙니다.";
        std::cout << std::endl;

    }


    return 0;
}