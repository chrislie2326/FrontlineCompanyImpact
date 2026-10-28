#include <iostream>
#include <string>
#include <cstdlib>
#include <ctime>

void GirlsFrontline()
{
    std::string P[5] = {"M1911", "PPK", "Glock 17", "Colt Revolver", "Contender"};
    std::string AR[5] = {"G3","StG44" ,"HK33" ,"G36", "HK416" };
    std::string SMG[5] = {"PPSh-41", "PP-2000", "PPS-43", "PP-19", "AK-74U"};
    std::string SG [5] = {"M500", "M590", "SPAS-12", "M1014", "Saiga-12"};
    std::string RF [5] = {"Simonov", "M14", "SV-98","Springfield", "WA2000"};
    std::string MG [5] = {"LWMMG", "M2HB", "M1919A4", "PK", "PKP"};
    std::string Squad[5] = {};
    int Gacha[5] = {};
    int Choice;
    bool Repeat;
    std::cout << "Select 5 Between P(1), AR(2), SMG(3), SG(4), RF(5), MG(6)" << std::endl <<
    "Pistol" << std::endl <<
    "Assault Rifle" << std::endl <<
    "Submachine Gun" << std::endl <<
    "Shotgun" << std::endl <<
    "Sniper" << std::endl <<
    "Machine gun" << std::endl << std::endl;
    //Selection for Units
    for (int i = 0; i <= 4; i++)
    {
        std::cout << "Selected Unit: ";
        std::cin >> Choice;
            while (Choice < 1 || Choice > 6)
            {
                std::cout << "Please choose from 1 to 6: ";
                std:: cin >> Choice;
                std::cout;
            }
            Gacha[i] = Choice;
            std::cout << "Slots Filled " << i + 1 << "/5" << std::endl;
    }
    //Gacha
    for (int i = 0; i <= 4; i++)
    {
        do
        {
            Repeat = false;
            if(Gacha[i] == 1)
            {
                Squad[i] = P[rand() % 5];
            }
                else if(Gacha[i] == 2)
                {
                    Squad[i] = AR[rand() % 5];
                }
                else if(Gacha[i] == 3)
                {
                    Squad[i] = SMG[rand() % 5];
                }
                else if(Gacha[i] == 4)
                {
                    Squad[i] = SG[rand() % 5];
                }
                else if(Gacha[i] == 5)
                {
                    Squad[i] = RF[rand() % 5];
                }
                    else
                    {
                      Squad[i] = MG[rand() % 5];
                    }
            for (int j = 0; j < i; j++)
            {
                if (Squad[j] == Squad[i])
                {
                    Repeat = true;
                }
            }
        } while (Repeat);
    }
    //Show Gacha
    std::cout << std::endl<< "Congratulations!" << std::endl << "Your Units are: ";
    for (int i = 0; i <= 4; i++)
    {
        if(i < 4)
        {
            std::cout << Squad[i] << ", ";
        }
            else
            {
                std::cout << Squad[i];
            }
    }
};
void GenshinImpact()
{
    std::string DPS[4] = {"Hu Tao", "Wanderer", "Mavuika", "Flins"};
    std::string Sub_DPS[4] = {"Kaeya", "Yelan", "Ineffa", "Traveler"};
    std::string Sup[4] = {"Bennett", "Nicole", "Durin", "Columbina"};
    std::string Sus[4] = {"Barbara", "Kokomi", "Zhongli", "Furina"};
    std::string Squad[4] = {};
    int Gacha[4] = {};
    int Choice;
    bool Repeat;
    std::cout << "Select 4 Between DPS(1), Sub-DPS(2), Support(3), Sustain(4)" << std::endl <<
    "DPS" << std::endl <<
    "Sub-DPS" << std::endl <<
    "Support" << std::endl <<
    "Sustain" << std::endl;

    //Selection for Units
    for(int i = 0; i <= 3; i++)
    {
        std::cout << "Selected Unit: ";
        std::cin >> Choice;
            while (Choice < 1 || Choice > 4)
            {
                std::cout << "Please choose from 1 to 4: ";
                std:: cin >> Choice;
                std::cout;
            }
            Gacha[i] = Choice;
            std::cout << "Slots Filled " << i + 1 << "/4" << std::endl;
    }
    //Gacha
    for (int i = 0; i <= 3; i++)
    {
        do
        {
            Repeat = false;
            if(Gacha[i] == 1)
            {
                Squad[i] = DPS[rand() % 4];
            }
                else if(Gacha[i] == 2)
                {
                    Squad[i] = Sub_DPS[rand() % 4];
                }
                else if(Gacha[i] == 3)
                {
                    Squad[i] = Sup[rand() % 4];
                }
                    else
                    {
                        Squad[i] = Sus[rand() % 4];
                    }

            for (int j = 0; j < i; j++)
            {
                if (Squad[j] == Squad[i])
                {
                    Repeat = true;
                }
            }

        } while (Repeat);

    }
    //Show Gacha
    std::cout << std::endl<< "Congratulations!" << std::endl << "Your Units are: ";
    for (int i = 0; i <= 3; i++)
    {
        if(i < 4)
        {
            std::cout << Squad[i] << ", ";
        }
            else
            {
                std::cout << Squad[i];
            }
    }
};
void LimbusCompany()
{
    std::string Sin[12] = {"Yi Sang", "Faust", "Don Quixote", "Ryoshu", "Meursault", "Gregor", "Hong Lu", "Ishmael", "Rodion", "Sinclair", "Outis", "Heathcliff"};
    std::string Squad[7] = {};
    int Choice;

    for (int i = 0; i < 7; i++)
    {
        std::cout << "Selected Unit: ";
        std::cin >> Choice;

        bool valid = false;
        while (!valid)
        {
            valid = true;

            if (Choice < 1 || Choice > 12)
            {
                std::cout << "Please choose from 1 to 12: ";
                std::cin >> Choice;
                valid = false;
            }
            else
            {
                for (int j = 0; j < i; j++)
                {
                    if (Squad[j] == Sin[Choice - 1])
                    {
                        std::cout << "Already picked, choose another: ";
                        std::cin >> Choice;
                        valid = false;
                        break;
                    }
                }
            }
        }

        Squad[i] = Sin[Choice - 1];
        std::cout << "Slots Filled " << i + 1 << "/7" << std::endl;
    }

        //Show Gacha
    std::cout << std::endl<< "Congratulations!" << std::endl << "Your Units are: ";
    for (int i = 0; i < 7; i++)
    {
        if(i < 6)
        {
            std::cout << Squad[i] << ", ";
        }
            else
            {
                std::cout << Squad[i];
            }
    }
}
void ChooseGacha()
{
    int choice;
    while(true)
    {
    std:: cin >> choice;
    if (choice == 1)
    {
        //std::cout << "\033[2J" << std::endl;
        GirlsFrontline();
        break;
    }
        else if (choice == 2)
        {
            //std::cout << "\033[2J" << std::endl;
            GenshinImpact();
            break;
        }
            else if (choice == 3)
            {
                //std::cout << "\033[2J" << std::endl;
                LimbusCompany();
                break;
            }
                else
                {
                    std::cout << "Please select among the choices: ";
                }
    }
}

int main()
{
    srand(time(0));
    std::cout << "Welcome to Frontline Company Impact" << std::endl << 
    "A Gacha Coin Flip Based Game where every success is based on a 50/50" << std::endl << 
    "The Three Factions"<< std::endl << 
    "1. Girls Frontline - All Units Attack on turn based on Class until Reload" << std::endl << 
    "2. Genshin Impact - Only One Unit in a Squad of 4 per turn, but can attack mutliple units" << std::endl << 
    "3. Limbus Companty - All Units can Attack Selectively per turn but can be Staggered" << std::endl << 
    "Choose by Number: ";
    ChooseGacha();
    return 0;
}