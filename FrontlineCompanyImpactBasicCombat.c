#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>

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


#define GFL_TEAM_SIZE 5
#define LIMBUS_TEAM_SIZE 7

typedef enum
{
    HG = 1, SMG, AR, MG, RF, SG
} GFLClass;

typedef struct
{
    const char *name;
    GFLClass weaponClass;
    int hp;
    int ammo;
} T_Doll;

typedef struct
{
    const char *name;
    T_Doll units[GFL_TEAM_SIZE];
} GFLPlayer;

typedef struct
{
    const char *name;
    int hp;
    bool skillUsed;
    int staggerTurns;
} Sinner;

typedef struct
{
    const char *name;
    Sinner units[LIMBUS_TEAM_SIZE];
} LimbusPlayer;


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

// Return a readable weapon-class name.
static const char *getGFLClassName(GFLClass weaponClass)
{
    switch (weaponClass)
    {
        case HG:  return "Pistol";
        case SMG: return "SMG";
        case AR:  return "Assault Rifle";
        case MG:  return "Machine Gun";
        case RF:  return "Rifle";
        case SG:  return "Shotgun";
        default:  return "Unknown";
    }
}

static int getGFLDamage(GFLClass weaponClass)
{
    switch (weaponClass)
    {
        case HG:  return 100;
        case SMG: return 150;
        case AR:  return 200;
        case MG:  return 250;
        case RF:  return 300;
        case SG:  return 350;
        default:  return 0;
    }
}

static void printGFLTeam(const GFLPlayer *player)
{
    printf("\n========== %s ==========\n", player->name);

    for (int i = 0; i < GFL_TEAM_SIZE; i++)
    {
        const T_Doll *unit = &player->units[i];
        printf("%d. %-16s | %-13s | HP: %4d/%d | Ammo: %d/3\n",
               i + 1, unit->name, getGFLClassName(unit->weaponClass),
               unit->hp, MAX_HP, unit->ammo);
    }
}

static bool gflHasLivingUnits(const GFLPlayer *player)
{
    for (int i = 0; i < GFL_TEAM_SIZE; i++)
        if (player->units[i].hp > 0)
            return true;

    return false;
}

static int chooseGFLUnit(GFLPlayer *player)
{
    while (1)
    {
        printf("\nSelect one of your T-Dolls:\n");

        for (int i = 0; i < GFL_TEAM_SIZE; i++)
        {
            T_Doll *unit = &player->units[i];
            printf("%d. %-16s HP: %4d | Ammo: %d/3",
                   i + 1, unit->name, unit->hp, unit->ammo);

            if (unit->hp <= 0)
                printf(" [DEFEATED]");
            else if (unit->ammo == 0)
                printf(" [EMPTY - MUST RELOAD]");

            printf("\n");
        }

        int choice = readIntRange("Choose T-Doll: ", 1, GFL_TEAM_SIZE) - 1;

        if (player->units[choice].hp > 0)
            return choice;

        printf("That T-Doll has been defeated.\n");
    }
}

static int chooseGFLTarget(GFLPlayer *player)
{
    while (1)
    {
        printf("\nSelect an enemy target:\n");

        for (int i = 0; i < GFL_TEAM_SIZE; i++)
        {
            printf("%d. %-16s HP: %4d/%d%s\n",
                   i + 1, player->units[i].name,
                   player->units[i].hp, MAX_HP,
                   player->units[i].hp <= 0 ? " [DEFEATED]" : "");
        }

        int choice = readIntRange("Choose target: ", 1, GFL_TEAM_SIZE) - 1;

        if (player->units[choice].hp > 0)
            return choice;

        printf("That T-Doll has already been defeated.\n");
    }
}

static void gflCombat(GFLPlayer players[2])
{
    int currentPlayer = rand() % 2;
    int turn = 1;

    printf("\n====================================\n");
    printf("             COIN FLIP\n");
    printf("====================================\n");
    printf("The coin landed on %s!\n",
           currentPlayer == 0 ? "HEADS" : "TAILS");
    printf("%s goes first!\n", players[currentPlayer].name);

    while (gflHasLivingUnits(&players[0]) &&
           gflHasLivingUnits(&players[1]))
    {
        GFLPlayer *attacker = &players[currentPlayer];
        GFLPlayer *defender = &players[1 - currentPlayer];

        printf("\n========== TURN %d: %s ==========\n",
               turn, attacker->name);
        printGFLTeam(attacker);

        int unitIndex = chooseGFLUnit(attacker);
        T_Doll *unit = &attacker->units[unitIndex];

        if (unit->ammo == 0)
        {
            printf("\n%s is out of ammunition and must reload.\n",
                   unit->name);
            unit->ammo = 3;
            printf("%s reloaded! Ammunition: 3/3.\n", unit->name);
        }
        else
        {
            int targetIndex = chooseGFLTarget(defender);
            T_Doll *target = &defender->units[targetIndex];
            int damage = getGFLDamage(unit->weaponClass);

            if (damage > target->hp)
                damage = target->hp;

            target->hp -= damage;
            unit->ammo--;

            printf("\n%s attacks %s for %d damage!\n",
                   unit->name, target->name, damage);
            printf("%s HP: %d/%d | %s ammo: %d/3\n",
                   target->name, target->hp, MAX_HP,
                   unit->name, unit->ammo);

            if (target->hp == 0)
                printf("*** %s has been eliminated! ***\n", target->name);
        }

        printGFLTeam(&players[0]);
        printGFLTeam(&players[1]);

        if (!gflHasLivingUnits(defender))
            break;

        currentPlayer = 1 - currentPlayer;
        turn++;
    }

    printf("\n========== MATCH OVER ==========\n");
    printf("%s WINS!\n",
           gflHasLivingUnits(&players[0]) ? players[0].name : players[1].name);
}

// Each player chooses five classes; one distinct T-Doll is rolled
// from the selected class pool for each slot.
void GirlsFrontline()
{
    static const char *const pools[6][5] = {
        {"M1911", "PPK", "Glock 17", "Colt Revolver", "Contender"},
        {"G3", "StG44", "HK33", "G36", "HK416"},
        {"PPSh-41", "PP-2000", "PPS-43", "PP-19", "AK-74U"},
        {"M500", "M590", "SPAS-12", "M1014", "Saiga-12"},
        {"Simonov", "M14", "SV-98", "Springfield", "WA2000"},
        {"LWMMG", "M2HB", "M1919A4", "PK", "PKP"}
    };

    const GFLClass classes[6] = {HG, AR, SMG, SG, RF, MG};
    GFLPlayer players[2] = {0};

    players[0].name = "Player 1";
    players[1].name = "Player 2";

    for (int p = 0; p < 2; p++)
    {
        printf("\n========== %s GFL TEAM ROLL ==========\n",
               players[p].name);

        for (int i = 0; i < GFL_TEAM_SIZE; i++)
        {
            printf("\nChoose a T-Doll class:\n");
            printf("1. Pistol (HG)\n");
            printf("2. Submachine Gun (SMG)\n");
            printf("3. Assault Rifle (AR)\n");
            printf("4. Machine Gun (MG)\n");
            printf("5. Rifle (RF)\n");
            printf("6. Shotgun (SG)\n");

            int choice = readIntRange("Your choice: ", 1, 6) - 1;
            int roll;

            // Avoid duplicate T-Dolls within the same player's squad.
            bool duplicate;
            do
            {
                duplicate = false;
                roll = rand() % 5;

                for (int j = 0; j < i; j++)
                {
                    if (strcmp(players[p].units[j].name,
                               pools[choice][roll]) == 0)
                    {
                        duplicate = true;
                        break;
                    }
                }
            } while (duplicate);

            T_Doll *unit = &players[p].units[i];
            unit->name = pools[choice][roll];
            unit->weaponClass = classes[choice];
            unit->hp = MAX_HP;
            unit->ammo = 3;

            printf("%s pulled: %s (%s)!\n",
                   players[p].name, unit->name,
                   getGFLClassName(unit->weaponClass));
            printf("Slots Filled: %d/%d\n", i + 1, GFL_TEAM_SIZE);
        }

        printGFLTeam(&players[p]);
    }

    gflCombat(players);
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


static void printLimbusTeam(const LimbusPlayer *player)
{
    printf("\n========== %s ==========\n", player->name);

    for (int i = 0; i < LIMBUS_TEAM_SIZE; i++)
    {
        const Sinner *unit = &player->units[i];
        printf("%d. %-16s | HP: %4d/%d | Skill: %-5s | %s\n",
               i + 1, unit->name, unit->hp, MAX_HP,
               unit->skillUsed ? "USED" : "READY",
               unit->staggerTurns > 0 ? "STAGGERED" : "READY");
    }
}

static bool limbusHasLivingUnits(const LimbusPlayer *player)
{
    for (int i = 0; i < LIMBUS_TEAM_SIZE; i++)
        if (player->units[i].hp > 0)
            return true;

    return false;
}

static int chooseLimbusUnit(LimbusPlayer *player)
{
    while (1)
    {
        bool hasAvailableUnit = false;

        printf("\nSelect one of your Sinners:\n");

        for (int i = 0; i < LIMBUS_TEAM_SIZE; i++)
        {
            Sinner *unit = &player->units[i];

            printf("%d. %-16s HP: %4d/%d",
                   i + 1, unit->name, unit->hp, MAX_HP);

            if (unit->hp <= 0)
                printf(" [DEFEATED]");
            else if (unit->staggerTurns > 0)
                printf(" [STAGGERED - CANNOT ACT]");
            else
                hasAvailableUnit = true;

            printf("\n");
        }

        if (!hasAvailableUnit)
            return -1;

        int choice = readIntRange("Choose Sinner: ", 1,
                                  LIMBUS_TEAM_SIZE) - 1;

        if (player->units[choice].hp <= 0)
            printf("That Sinner has been defeated.\n");
        else if (player->units[choice].staggerTurns > 0)
            printf("That Sinner is staggered and cannot act yet.\n");
        else
            return choice;
    }
}

static int chooseLimbusTarget(LimbusPlayer *player)
{
    while (1)
    {
        printf("\nSelect an enemy target:\n");

        for (int i = 0; i < LIMBUS_TEAM_SIZE; i++)
        {
            printf("%d. %-16s HP: %4d/%d%s\n",
                   i + 1, player->units[i].name,
                   player->units[i].hp, MAX_HP,
                   player->units[i].hp <= 0 ? " [DEFEATED]" : "");
        }

        int choice = readIntRange("Choose target: ", 1,
                                  LIMBUS_TEAM_SIZE) - 1;

        if (player->units[choice].hp > 0)
            return choice;

        printf("That Sinner has already been defeated.\n");
    }
}

// A staggered Sinner misses one of their team's turns, then recovers.
static void recoverLimbusStagger(LimbusPlayer *player)
{
    for (int i = 0; i < LIMBUS_TEAM_SIZE; i++)
    {
        if (player->units[i].staggerTurns > 0)
        {
            player->units[i].staggerTurns--;

            if (player->units[i].staggerTurns == 0)
                printf("%s recovered from Stagger!\n",
                       player->units[i].name);
        }
    }
}

static void limbusCombat(LimbusPlayer players[2])
{
    int currentPlayer = rand() % 2;
    int turn = 1;

    printf("\n====================================\n");
    printf("             COIN FLIP\n");
    printf("====================================\n");
    printf("The coin landed on %s!\n",
           currentPlayer == 0 ? "HEADS" : "TAILS");
    printf("%s goes first!\n", players[currentPlayer].name);

    while (limbusHasLivingUnits(&players[0]) &&
           limbusHasLivingUnits(&players[1]))
    {
        LimbusPlayer *attacker = &players[currentPlayer];
        LimbusPlayer *defender = &players[1 - currentPlayer];

        printf("\n========== TURN %d: %s ==========\n",
               turn, attacker->name);
        printLimbusTeam(attacker);

        int unitIndex = chooseLimbusUnit(attacker);

        if (unitIndex == -1)
        {
            printf("\nAll available Sinners are staggered. "
                   "%s loses this turn!\n", attacker->name);
        }
        else
        {
            Sinner *unit = &attacker->units[unitIndex];

            printf("\nSelected: %s\n", unit->name);
            printf("1. Normal Attack (150 damage)\n");

            if (unit->skillUsed)
                printf("2. Skill Attack [ALREADY USED]\n");
            else
                printf("2. Skill Attack (250 damage)\n");

            int action = unit->skillUsed
                ? readIntRange("Choose move: ", 1, 1)
                : readIntRange("Choose move: ", 1, 2);

            int damage = 150;

            if (action == 2)
            {
                damage = 250;
                unit->skillUsed = true;
                printf("\n%s uses their Skill Attack!\n", unit->name);
            }
            else
            {
                printf("\n%s performs a Normal Attack!\n", unit->name);
            }

            int targetIndex = chooseLimbusTarget(defender);
            Sinner *target = &defender->units[targetIndex];
            int previousHP = target->hp;

            if (damage > target->hp)
                damage = target->hp;

            target->hp -= damage;

            printf("\n%s deals %d damage to %s!\n",
                   unit->name, damage, target->name);
            printf("%s HP: %d/%d\n",
                   target->name, target->hp, MAX_HP);

            if (target->hp == 0)
            {
                printf("*** %s has been defeated! ***\n", target->name);
            }
            else if (previousHP > 500 && target->hp <= 500)
            {
                target->staggerTurns = 1;
                printf("*** %s has been STAGGERED! "
                       "They will miss their next team turn. ***\n",
                       target->name);
            }
        }

        // Stagger lasts through the affected player's next turn.
        recoverLimbusStagger(attacker);

        printLimbusTeam(&players[0]);
        printLimbusTeam(&players[1]);

        if (!limbusHasLivingUnits(defender))
            break;

        currentPlayer = 1 - currentPlayer;
        turn++;
    }

    printf("\n========== MATCH OVER ==========\n");
    printf("%s WINS!\n",
           limbusHasLivingUnits(&players[0])
               ? players[0].name : players[1].name);
}

void LimbusCompany()
{
    static const char *const sinners[LIMBUS_TEAM_SIZE + 5] = {
        "Yi Sang", "Faust", "Don Quixote", "Ryoshu",
        "Meursault", "Gregor", "Hong Lu", "Ishmael",
        "Rodion", "Sinclair", "Outis", "Heathcliff"
    };

    LimbusPlayer players[2] = {0};
    players[0].name = "Player 1";
    players[1].name = "Player 2";

    for (int p = 0; p < 2; p++)
    {
        printf("\n========== %s LIMBUS TEAM SELECTION ==========\n",
               players[p].name);

        for (int i = 0; i < LIMBUS_TEAM_SIZE; i++)
        {
            printf("\nAvailable Sinners:\n");

            for (int j = 0; j < 12; j++)
                printf("%2d. %s%s\n", j + 1, sinners[j],
                       (j == 5 || j == 11) ? "" : "");

            int choice;

            while (1)
            {
                choice = readIntRange("Select Sinner: ", 1, 12) - 1;

                bool duplicate = false;
                for (int j = 0; j < i; j++)
                {
                    if (strcmp(players[p].units[j].name,
                               sinners[choice]) == 0)
                    {
                        duplicate = true;
                        break;
                    }
                }

                if (!duplicate)
                    break;

                printf("Already selected. Choose another Sinner.\n");
            }

            players[p].units[i].name = sinners[choice];
            players[p].units[i].hp = MAX_HP;
            players[p].units[i].skillUsed = false;
            players[p].units[i].staggerTurns = 0;

            printf("%s selected: %s\n",
                   players[p].name, sinners[choice]);
            printf("Slots Filled: %d/%d\n",
                   i + 1, LIMBUS_TEAM_SIZE);
        }

        printLimbusTeam(&players[p]);
    }

    limbusCombat(players);
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
