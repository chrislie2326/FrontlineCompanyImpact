#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>

void GirlsFrontline()
{
    char *P[5] = {"M1911", "PPK", "Glock 17", "Colt Revolver", "Contender"};
    char *AR[5] = {"G3", "StG44", "HK33", "G36", "HK416"};
    char *SMG[5] = {"PPSh-41", "PP-2000", "PPS-43", "PP-19", "AK-74U"};
    char *SG[5] = {"M500", "M590", "SPAS-12", "M1014", "Saiga-12"};
    char *RF[5] = {"Simonov", "M14", "SV-98", "Springfield", "WA2000"};
    char *MG[5] = {"LWMMG", "M2HB", "M1919A4", "PK", "PKP"};
    char *Squad[5] = {NULL};
    int Gacha[5] = {0};
    int Choice;
    bool Repeat;

    printf("Select 5 Between P(1), AR(2), SMG(3), SG(4), RF(5), MG(6)\n");
    printf("Pistol\n");
    printf("Assault Rifle\n");
    printf("Submachine Gun\n");
    printf("Shotgun\n");
    printf("Sniper\n");
    printf("Machine gun\n\n");

    //Selection for Units
    for (int i = 0; i <= 4; i++)
    {
        printf("Selected Unit: ");
        scanf("%d", &Choice);

        while (Choice < 1 || Choice > 6)
        {
            printf("Please choose from 1 to 6: ");
            scanf("%d", &Choice);
        }

        Gacha[i] = Choice;
        printf("Slots Filled %d/5\n", i + 1);
    }

    //Gacha
    for (int i = 0; i <= 4; i++)
    {
        do
        {
            Repeat = false;

            if (Gacha[i] == 1)
            {
                Squad[i] = P[rand() % 5];
            }
            else if (Gacha[i] == 2)
            {
                Squad[i] = AR[rand() % 5];
            }
            else if (Gacha[i] == 3)
            {
                Squad[i] = SMG[rand() % 5];
            }
            else if (Gacha[i] == 4)
            {
                Squad[i] = SG[rand() % 5];
            }
            else if (Gacha[i] == 5)
            {
                Squad[i] = RF[rand() % 5];
            }
            else
            {
                Squad[i] = MG[rand() % 5];
            }

            for (int j = 0; j < i; j++)
            {
                if (strcmp(Squad[j], Squad[i]) == 0)
                {
                    Repeat = true;
                }
            }

        } while (Repeat);
    }

    //Show Gacha
    printf("\nCongratulations!\n");
    printf("Your Units are: ");

    for (int i = 0; i <= 4; i++)
    {
        if (i < 4)
        {
            printf("%s, ", Squad[i]);
        }
        else
        {
            printf("%s", Squad[i]);
        }
    }

    printf("\n");
}

const char *weightedPull(const char *pool[],
                         const int rates[],
                         int count)
{
    int totalWeight = 0;

    // Calculate the total probability weight.
    for (int i = 0; i < count; i++)
    {
        if (rates[i] > 0)
            totalWeight += rates[i];
    }

    if (totalWeight <= 0)
        return NULL;

    // Generate a random number within the total weight.
    int roll = rand() % totalWeight;

    // Determine which character corresponds to the roll.
    for (int i = 0; i < count; i++)
    {
        if (rates[i] <= 0)
            continue;

        if (roll < rates[i])
            return pool[i];

        roll -= rates[i];
    }

    return NULL;
}

#define TEAM_SIZE 4
#define MAX_HP 1000

typedef enum
{
    DPS = 1,
    SUB_DPS,
    SUPPORT,
    SUSTAIN
} Role;

typedef struct
{
    const char *name;
    Role role;
    int hp;
    bool skillUsed;
} Unit;

typedef struct
{
    const char *name;
    Unit units[TEAM_SIZE];
    int damageBuffAttacks;
} Player;


// Safely read an integer within a specified range.
static int readIntRange(const char *prompt, int min, int max)
{
    int value;
    int result;
    int ch;

    while (1)
    {
        printf("%s", prompt);
        result = scanf("%d", &value);

        if (result == EOF)
        {
            printf("\nInput ended. Exiting.\n");
            exit(EXIT_FAILURE);
        }

        // Clear the remaining input from the line.
        while ((ch = getchar()) != '\n' && ch != EOF)
        {
        }

        if (result == 1 && value >= min && value <= max)
            return value;

        printf("Invalid input. Enter a number from %d to %d.\n",
               min, max);
    }
}

// Return the name of a character's role.
static const char *getRoleName(Role role)
{
    switch (role)
    {
        case DPS:      return "DPS";
        case SUB_DPS:  return "Sub-DPS";
        case SUPPORT:  return "Support";
        case SUSTAIN:  return "Sustain";
        default:       return "Unknown";
    }
}

// Weighted character selection without duplicates within a team.
static const char *weightedPullUnique(
    const char *const pool[],
    const int weights[],
    int count,
    const Unit team[],
    int teamCount)
{
    int availableWeights[TEAM_SIZE];
    int totalWeight = 0;

    for (int i = 0; i < count; i++)
    {
        bool duplicate = false;

        for (int j = 0; j < teamCount; j++)
        {
            if (strcmp(pool[i], team[j].name) == 0)
            {
                duplicate = true;
                break;
            }
        }

        availableWeights[i] = duplicate ? 0 : weights[i];
        totalWeight += availableWeights[i];
    }

    if (totalWeight <= 0)
        return NULL;

    int roll = rand() % totalWeight;

    for (int i = 0; i < count; i++)
    {
        if (availableWeights[i] <= 0)
            continue;

        if (roll < availableWeights[i])
            return pool[i];

        roll -= availableWeights[i];
    }

    return NULL;
}

// Display a player's team.
static void printTeam(const Player *player)
{
    printf("\n========== %s ==========\n", player->name);

    for (int i = 0; i < TEAM_SIZE; i++)
    {
        printf("%d. %-14s | %-8s | HP: %4d/%d | Skill: %s\n",
               i + 1,
               player->units[i].name,
               getRoleName(player->units[i].role),
               player->units[i].hp,
               MAX_HP,
               player->units[i].skillUsed ? "USED" : "READY");
    }

    printf("Remaining damage buff attacks: %d\n",
           player->damageBuffAttacks);
}

// Check whether a player still has living characters.
static bool hasLivingUnits(const Player *player)
{
    for (int i = 0; i < TEAM_SIZE; i++)
    {
        if (player->units[i].hp > 0)
            return true;
    }

    return false;
}

// Select a living character from the active player's team.
static int chooseLivingUnit(Player *player)
{
    while (1)
    {
        printf("\nSelect one of your characters:\n");

        for (int i = 0; i < TEAM_SIZE; i++)
        {
            printf("%d. %s - HP: %d/%d",
                   i + 1,
                   player->units[i].name,
                   player->units[i].hp,
                   MAX_HP);

            if (player->units[i].hp <= 0)
                printf(" [DEFEATED]");

            printf("\n");
        }

        int choice = readIntRange("Choose unit: ", 1, TEAM_SIZE);

        if (player->units[choice - 1].hp > 0)
            return choice - 1;

        printf("That character has been defeated!\n");
    }
}

// Select a living enemy character.
static int chooseLivingTarget(Player *opponent)
{
    while (1)
    {
        printf("\nSelect an enemy target:\n");

        for (int i = 0; i < TEAM_SIZE; i++)
        {
            printf("%d. %s - HP: %d/%d",
                   i + 1,
                   opponent->units[i].name,
                   opponent->units[i].hp,
                   MAX_HP);

            if (opponent->units[i].hp <= 0)
                printf(" [DEFEATED]");

            printf("\n");
        }

        int choice = readIntRange("Choose target: ", 1, TEAM_SIZE);

        if (opponent->units[choice - 1].hp > 0)
            return choice - 1;

        printf("That character has already been defeated!\n");
    }
}

// Determine normal attack damage based on role.
static int getNormalDamage(Role role)
{
    switch (role)
    {
        case DPS:      return 200;
        case SUB_DPS:  return 150;
        case SUPPORT:  return 100;
        case SUSTAIN:  return 100;
        default:       return 0;
    }
}

// Calculate damage and apply it to the enemy.
static void dealDamage(
    Player *attacker,
    Player *defender,
    int targetIndex,
    int baseDamage)
{
    Unit *target = &defender->units[targetIndex];

    int damage = baseDamage;

    // Apply an active damage buff.
    if (attacker->damageBuffAttacks > 0)
    {
        damage = damage * 125 / 100;
        attacker->damageBuffAttacks--;

        printf("\nDamage buff activated! Increased damage: %d\n",
               damage);
    }

    // Prevent damage from exceeding remaining HP.
    if (damage > target->hp)
        damage = target->hp;

    target->hp -= damage;

    printf("\n%s deals %d damage to %s's %s!\n",
           attacker->name,
           damage,
           defender->name,
           target->name);

    printf("%s's remaining HP: %d/%d\n",
           target->name,
           target->hp,
           MAX_HP);

    if (target->hp <= 0)
    {
        printf("\n*** %s has been defeated! ***\n", target->name);
    }
}

// Restore HP to every living character on the player's team.
static void healTeam(Player *player, int healAmount)
{
    printf("\n%s activated Party Healing!\n", player->name);

    for (int i = 0; i < TEAM_SIZE; i++)
    {
        Unit *unit = &player->units[i];

        if (unit->hp <= 0)
            continue;

        int previousHP = unit->hp;

        unit->hp += healAmount;

        if (unit->hp > MAX_HP)
            unit->hp = MAX_HP;

        int actualHealing = unit->hp - previousHP;

        printf("%s restored %d HP. Current HP: %d/%d\n",
               unit->name,
               actualHealing,
               unit->hp,
               MAX_HP);
    }
}




void GenshinImpact()
{
    // Character pools: weakest to strongest.
    static const char *const DPS_POOL[4] = {
        "Hu Tao", "Wanderer", "Mavuika", "Flins"
    };

    static const char *const SUB_DPS_POOL[4] = {
        "Kaeya", "Yelan", "Ineffa", "Traveler"
    };

    static const char *const SUPPORT_POOL[4] = {
        "Bennett", "Nicole", "Durin", "Columbina"
    };

    static const char *const SUSTAIN_POOL[4] = {
        "Barbara", "Kokomi", "Zhongli", "Furina"
    };

    // Initial drop rates.
    const int dropRates[4] = {40, 30, 20, 10};

    // Initialize both players.
    Player players[2] = {0};

    players[0].name = "Player 1";
    players[1].name = "Player 2";

    // ==============================
    // CHARACTER ROLLING
    // ==============================

    for (int p = 0; p < 2; p++)
    {
        printf("\n====================================\n");
        printf("          %s GACHA\n", players[p].name);
        printf("====================================\n");

        for (int i = 0; i < TEAM_SIZE; i++)
        {
            printf("\nSelect character category:\n");
            printf("1. DPS\n");
            printf("2. Sub-DPS\n");
            printf("3. Support\n");
            printf("4. Sustain\n");

            int choice = readIntRange("Your choice: ", 1, 4);

            const char *const *pool = NULL;

            switch (choice)
            {
                case DPS:
                    pool = DPS_POOL;
                    break;

                case SUB_DPS:
                    pool = SUB_DPS_POOL;
                    break;

                case SUPPORT:
                    pool = SUPPORT_POOL;
                    break;

                case SUSTAIN:
                    pool = SUSTAIN_POOL;
                    break;
            }

            // Perform weighted character pull.
            Unit *newUnit = &players[p].units[i];

            newUnit->name = weightedPullUnique(
                pool,
                dropRates,
                TEAM_SIZE,
                players[p].units,
                i
            );

            if (newUnit->name == NULL)
            {
                printf("Unable to generate character.\n");
                return;
            }

            newUnit->role = (Role)choice;
            newUnit->hp = MAX_HP;
            newUnit->skillUsed = false;

            printf("\n%s pulled: %s!\n",
                   players[p].name,
                   newUnit->name);

            printf("Slots Filled: %d/%d\n",
                   i + 1,
                   TEAM_SIZE);
        }

        printf("\n%s has completed their team!\n",
               players[p].name);

        printTeam(&players[p]);
    }

    // ==============================
    // COIN FLIP
    // ==============================

    printf("\n====================================\n");
    printf("             COIN FLIP\n");
    printf("====================================\n");

    int currentPlayer = rand() % 2;

    if (currentPlayer == 0)
        printf("The coin landed on HEADS!\n");
    else
        printf("The coin landed on TAILS!\n");

    printf("%s will make the first move!\n",
           players[currentPlayer].name);

    // ==============================
    // COMBAT SYSTEM
    // ==============================

    int turnNumber = 1;

    while (hasLivingUnits(&players[0]) &&
           hasLivingUnits(&players[1]))
    {
        Player *attacker = &players[currentPlayer];
        Player *defender = &players[1 - currentPlayer];

        printf("\n\n====================================\n");
        printf("               TURN %d\n", turnNumber);
        printf("====================================\n");

        printf("%s's turn!\n", attacker->name);

        printTeam(attacker);

        // Select the active character.
        int unitIndex = chooseLivingUnit(attacker);

        Unit *unit = &attacker->units[unitIndex];

        printf("\nSelected: %s (%s)\n",
               unit->name,
               getRoleName(unit->role));

        // Choose an action.
        int action;

        if (unit->skillUsed)
        {
            printf("\nElemental Skill already used.\n");
            printf("1. Normal Attack\n");

            action = readIntRange("Choose move: ", 1, 1);
        }
        else
        {
            printf("\nAvailable Moves:\n");
            printf("1. Normal Attack\n");
            printf("2. Elemental Skill\n");

            action = readIntRange("Choose move: ", 1, 2);
        }

        // ==============================
        // NORMAL ATTACK
        // ==============================

        if (action == 1)
        {
            int targetIndex = chooseLivingTarget(defender);

            int damage = getNormalDamage(unit->role);

            dealDamage(
                attacker,
                defender,
                targetIndex,
                damage
            );
        }

        // ==============================
        // ELEMENTAL SKILL
        // ==============================

        else if (action == 2)
        {
            // Each character can use their skill once per match.
            unit->skillUsed = true;

            switch (unit->role)
            {
                case DPS:
                {
                    printf("\n%s uses their Elemental Skill!\n",
                           unit->name);

                    int targetIndex = chooseLivingTarget(defender);

                    dealDamage(
                        attacker,
                        defender,
                        targetIndex,
                        350
                    );

                    break;
                }

                case SUB_DPS:
                {
                    printf("\n%s uses their Elemental Skill!\n",
                           unit->name);

                    int targetIndex = chooseLivingTarget(defender);

                    dealDamage(
                        attacker,
                        defender,
                        targetIndex,
                        300
                    );

                    break;
                }

                case SUPPORT:
                {
                    printf("\n%s uses their Elemental Skill!\n",
                           unit->name);

                    attacker->damageBuffAttacks = 2;

                    printf("%s's party gained a 25%% damage buff!\n",
                           attacker->name);

                    printf("The next two damage-dealing attacks "
                           "will receive the buff.\n");

                    break;
                }

                case SUSTAIN:
                {
                    printf("\n%s uses their Elemental Skill!\n",
                           unit->name);

                    healTeam(attacker, 200);

                    break;
                }
            }
        }

        // ==============================
        // END OF TURN
        // ==============================

        printf("\n---------- END OF TURN ----------\n");

        printTeam(&players[0]);
        printTeam(&players[1]);

        // Check if either team has been defeated.
        if (!hasLivingUnits(defender))
            break;

        // Alternate players.
        currentPlayer = 1 - currentPlayer;
        turnNumber++;
    }

    // ==============================
    // MATCH RESULT
    // ==============================

    printf("\n\n====================================\n");
    printf("             MATCH OVER\n");
    printf("====================================\n");

    if (hasLivingUnits(&players[0]))
    {
        printf("VICTORY! PLAYER 1 WINS!\n");
        printf("Congratulations, Player 1!\n");
    }
    else
    {
        printf("VICTORY! PLAYER 2 WINS!\n");
        printf("Congratulations, Player 2!\n");
    }

    printf("\nFinal Team Status:\n");

    printTeam(&players[0]);
    printTeam(&players[1]);
}


void LimbusCompany()
{
    char *Sin[12] = {
        "Yi Sang", "Faust", "Don Quixote", "Ryoshu",
        "Meursault", "Gregor", "Hong Lu", "Ishamel",
        "Rodion", "Sinclair", "Outis", "Heathcliff"
    };

    char *Squad[7] = {NULL};
    int Choice;

    for (int i = 0; i <= 6; i++)
    {
        printf("Selected Unit: ");
        scanf("%d", &Choice);

        bool valid = false;

        while (!valid)
        {
            valid = true;

            if (Choice < 1 || Choice > 12)
            {
                printf("Please choose from 1 to 12: ");
                scanf("%d", &Choice);
                valid = false;
            }
            else
            {
                for (int j = 0; j < i; j++)
                {
                    if (strcmp(Squad[j], Sin[Choice - 1]) == 0)
                    {
                        printf("Already picked, choose another: ");
                        scanf("%d", &Choice);
                        valid = false;
                        break;
                    }
                }
            }
        }

        Squad[i] = Sin[Choice - 1];
        printf("Slots Filled %d/7\n", i + 1);
    }

    //Show Gacha
    printf("\nCongratulations!\n");
    printf("Your Units are: ");

    for (int i = 0; i <= 6; i++)
    {
        if (i < 6)
        {
            printf("%s, ", Squad[i]);
        }
        else
        {
            printf("%s", Squad[i]);
        }
    }

    printf("\n");
}

void ChooseGacha()
{
    int choice;

    while (true)
    {
        scanf("%d", &choice);

        if (choice == 1)
        {
            GirlsFrontline();
            break;
        }
        else if (choice == 2)
        {
            GenshinImpact();
            break;
        }
        else if (choice == 3)
        {
            LimbusCompany();
            break;
        }
        else
        {
            printf("Please select among the choices: ");
        }
    }
}

int main()
{
    srand(time(NULL));

    printf("Welcome to Frontline Company Impact\n");
    printf("A Gacha Coin Flip Based Game where every success is based on a 50/50\n");
    printf("The Three Factions\n");
    printf("1. Girls Frontline - All Units Attack on turn based on Class until Reload\n");
    printf("2. Genshin Impact - Only One Unit in a Squad of 4 per turn, but can attack multiple units\n");
    printf("3. Limbus Company - All Units can Attack Selectively per turn but can be Staggered\n");
    printf("Choose by Number: ");

    ChooseGacha();

    return 0;
}