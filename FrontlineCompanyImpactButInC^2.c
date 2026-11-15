#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define HP 1000
#define MAX_UNITS 7
#define NAME_SIZE 32

enum { GFL = 1, GI = 2, LC = 3 };

typedef struct {
    char name[NAME_SIZE];
    int hp, ammo, type, skillUsed, staggered;
} Unit;

typedef struct {
    char name[20];
    Unit units[MAX_UNITS];
    int size, defending, buff;   /* buff = attacks left with +100 damage */
    int faction;                 /* GFL, GI or LC */
} Player;

const char *factionName[4] = {"", "Girls' Frontline", "Genshin Impact", "Limbus Company"};

const char *giNames[4][4] = {
    {"Hu Tao", "Wanderer", "Mavuika", "Flins"},
    {"Kaeya", "Yelan", "Ineffa", "Traveler"},
    {"Bennett", "Nicole", "Durin", "Columbina"},
    {"Barbara", "Kokomi", "Zhongli", "Furina"}
};
const char *giRole[4] = {"DPS", "Sub-DPS", "Support", "Sustain"};

const char *gflNames[6][5] = {
    {"M1911", "PPK", "Glock 17", "Colt Revolver", "Contender"},
    {"PPSh-41", "PP-2000", "PPS-43", "PP-19", "AK-74U"},
    {"G3", "StG44", "HK33", "G36", "HK416"},
    {"LWMMG", "M2HB", "M1919A4", "PK", "PKP"},
    {"Simonov", "M14", "SV-98", "Springfield", "WA2000"},
    {"M500", "M590", "SPAS-12", "M1014", "Saiga-12"}
};
const char *gflClass[6] = {"Pistol", "SMG", "AR", "MG", "Rifle", "Shotgun"};
const int gflDamage[6] = {100, 150, 200, 250, 300, 350};
const int gflAmmo[6] = {2, 4, 5, 3, 3, 3};   /* magazine size per class */

const char *sinnerNames[12] = {
    "Yi Sang", "Faust", "Don Quixote", "Ryoshu", "Meursault", "Gregor",
    "Hong Lu", "Ishmael", "Rodion", "Sinclair", "Outis", "Heathcliff"
};

/* ---------- helpers ---------- */

int readNumber(const char *prompt, int min, int max)
{
    char line[100], extra;
    int value;
    while (1) {
        printf("%s", prompt);
        if (!fgets(line, sizeof line, stdin)) exit(0);
        if (sscanf(line, "%d %c", &value, &extra) == 1 && value >= min && value <= max)
            return value;
        printf("Invalid. Enter %d to %d.\n", min, max);
    }
}

int livingCount(const Player *p)
{
    int n = 0;
    for (int i = 0; i < p->size; i++) if (p->units[i].hp > 0) n++;
    return n;
}

void showTeam(const Player *p)
{
    printf("\n--- %s (%s) ---\n", p->name, factionName[p->faction]);
    for (int i = 0; i < p->size; i++) {
        const Unit *u = &p->units[i];
        printf("%d. %-14s HP:%4d", i + 1, u->name, u->hp);
        if (p->faction == GFL) printf(" Ammo:%d/%d", u->ammo, gflAmmo[u->type]);
        if (u->staggered) printf(" [STAGGERED]");
        if (u->hp <= 0) printf(" [DEFEATED]");
        printf("\n");
    }
}

int coinMatch(void)
{
    int mine = readNumber("Pick coin (1=Tails, 2=Heads): ", 1, 2) - 1;
    int cpu = rand() % 2;
    printf("CPU coin: %s -> ", cpu ? "Heads" : "Tails");
    if (mine == cpu) { printf("MATCH!\n"); return 1; }
    printf("no match.\n");
    return 0;
}

/* Applies damage with defend (halved) and stagger (+50%) modifiers. */
void hit(Player *d, int i, int dmg)
{
    Unit *t = &d->units[i];
    int before = t->hp;
    if (d->defending) dmg /= 2;
    if (t->staggered) dmg = dmg * 3 / 2;
    if (dmg > t->hp) dmg = t->hp;
    t->hp -= dmg;
    printf("  -> %s takes %d damage (HP %d)\n", t->name, dmg, t->hp);
    if (d->faction == LC && before > 500 && t->hp > 0 && t->hp <= 500) {
        t->staggered = 1;
        printf("  -> %s is STAGGERED! (takes extra damage, misses next action)\n", t->name);
    }
}

/* Attack damage including support buff. */
int withBuff(Player *a, int dmg)
{
    if (a->buff > 0) { a->buff--; return dmg + 100; }
    return dmg;
}

int chooseTarget(Player *d)
{
    while (1) {
        showTeam(d);
        int c = readNumber("Choose enemy target: ", 1, d->size) - 1;
        if (d->units[c].hp > 0) return c;
        printf("That target is defeated.\n");
    }
}

/* needSkill=1 -> must still have its ability available */
int chooseUnit(Player *p, int needSkill)
{
    while (1) {
        showTeam(p);
        int c = readNumber("Choose your unit: ", 1, p->size) - 1;
        if (p->units[c].hp <= 0) printf("That unit is defeated.\n");
        else if (needSkill && p->units[c].skillUsed) printf("Ability already used.\n");
        else return c;
    }
}

/* ---------- gacha ---------- */

int isDuplicate(const Player *p, int count, const char *name)
{
    for (int j = 0; j < count; j++)
        if (strcmp(p->units[j].name, name) == 0) return 1;
    return 0;
}

void setupTeam(Player *p)
{
    Unit *u;
    int faction = p->faction;
    if (faction == GI) p->size = 4;
    else if (faction == GFL) p->size = 5;
    else p->size = 7;

    for (int i = 0; i < p->size; i++) {
        u = &p->units[i];
        memset(u, 0, sizeof *u);
        u->hp = HP;
        u->ammo = -1;

        if (faction == GI) {
            int role = readNumber("\nRole: 1 DPS, 2 Sub-DPS, 3 Support, 4 Sustain: ", 1, 4) - 1;
            int pick;
            do {
                int r = rand() % 100;   /* 40/30/20/10 weighted pull */
                pick = r < 40 ? 0 : r < 70 ? 1 : r < 90 ? 2 : 3;
            } while (isDuplicate(p, i, giNames[role][pick]));
            snprintf(u->name, NAME_SIZE, "%s", giNames[role][pick]);
            u->type = role;
            printf("%s pulled %s (%s)!\n", p->name, u->name, giRole[role]);
        } else if (faction == GFL) {
            int type = readNumber("\nClass: 1 Pistol, 2 SMG, 3 AR, 4 MG, 5 Rifle, 6 Shotgun: ", 1, 6) - 1;
            int pick;
            do pick = rand() % 5;
            while (isDuplicate(p, i, gflNames[type][pick]));
            snprintf(u->name, NAME_SIZE, "%s", gflNames[type][pick]);
            u->type = type;
            u->ammo = gflAmmo[type];
            printf("%s pulled %s (%s)!\n", p->name, u->name, gflClass[type]);
        } else {
            printf("\nAvailable Sinners:\n");
            for (int j = 0; j < 12; j++) printf("%2d. %s\n", j + 1, sinnerNames[j]);
            int pick;
            while (1) {
                pick = readNumber("Select Sinner: ", 1, 12) - 1;
                if (!isDuplicate(p, i, sinnerNames[pick])) break;
                printf("Already selected.\n");
            }
            snprintf(u->name, NAME_SIZE, "%s", sinnerNames[pick]);
        }
    }
}

/* ---------- GFL / LC abilities and attacks ---------- */

void useAbility(Player *a, Player *d)
{
    int any = 0;
    for (int i = 0; i < a->size; i++)
        if (a->units[i].hp > 0 && !a->units[i].skillUsed) any = 1;
    if (!any) { printf("No abilities left.\n"); return; }

    Unit *u = &a->units[chooseUnit(a, 1)];
    u->skillUsed = 1;
    printf("%s uses an ability!\n", u->name);
    hit(d, chooseTarget(d), a->faction == GFL ? 300 : 250);
}

void attackGFL(Player *a, Player *d)
{
    printf("\nYour T-Dolls fire automatically by class.\n");
    int success = coinMatch();   /* one flip for the whole team */

    for (int i = 0; i < a->size && livingCount(d) > 0; i++) {
        Unit *u = &a->units[i];
        if (u->hp <= 0) continue;

        if (u->ammo == 0) {
            printf("%s is out of ammo and reloads.\n", u->name);
            u->ammo = gflAmmo[u->type];
            continue;
        }

        int t = -1;   /* same class first, otherwise first living enemy */
        for (int j = 0; j < d->size && t < 0; j++)
            if (d->faction == GFL && d->units[j].hp > 0 && d->units[j].type == u->type) t = j;
        for (int j = 0; j < d->size && t < 0; j++)
            if (d->units[j].hp > 0) t = j;

        u->ammo--;
        printf("\n%s fires at %s.\n", u->name, d->units[t].name);
        if (success) hit(d, t, withBuff(a, gflDamage[u->type]));
        else printf("  -> Missed.\n");
    }
}


void attackLC(Player *a, Player *d)
{
    printf("\nYour Sinners attack together.\n");
    int success = coinMatch();

    for (int i = 0; i < a->size && livingCount(d) > 0; i++) {
        Unit *u = &a->units[i];

        if (u->hp <= 0) continue;

        if (u->staggered) {
            printf("%s is staggered and misses this turn.\n", u->name);
            u->staggered = 0;
            continue;
        }

        printf("\n%s attacks:\n", u->name);

        if (success) {
            int t = chooseTarget(d);
            hit(d, t, withBuff(a, 150));
        } else {
            printf("  -> Missed. No target selected.\n");
        }
    }
}


/* ---------- Genshin ---------- */

void attackGI(Player *a, Player *d, Unit *u)
{
    int base = u->type == 0 ? 200 : u->type == 1 ? 150 : 100;
    printf("\n%s attacks every enemy!\n", u->name);
    int success = coinMatch();   /* one flip for the whole attack */
    for (int t = 0; t < d->size; t++) {
        if (d->units[t].hp <= 0) continue;
        printf("Target: %s\n", d->units[t].name);
        if (success) hit(d, t, withBuff(a, base));
        else printf("  -> Missed.\n");
    }
}

void abilityGI(Player *a, Player *d, Unit *u)
{
    u->skillUsed = 1;
    if (u->type <= 1) {
        int dmg = u->type == 0 ? 350 : 300;
        int n = readNumber("How many enemy targets? ", 1, livingCount(d));
        for (int k = 0; k < n && livingCount(d) > 0; k++)
            hit(d, chooseTarget(d), dmg);
    } else if (u->type == 2) {
        a->buff = 3;
        printf("%s's team gets +100 damage on the next 3 hits!\n", u->name);
    } else {
        for (int i = 0; i < a->size; i++)
            if (a->units[i].hp > 0) {
                a->units[i].hp += 200;
                if (a->units[i].hp > HP) a->units[i].hp = HP;
            }
        printf("Party healed by up to 200 HP.\n");
    }
}

void turnGI(Player *a, Player *d)
{
    a->defending = 0;
    Unit *u = &a->units[chooseUnit(a, 0)];   /* pick the character first */
    printf("\n%s takes the field.\n", u->name);

    while (1) {
        int c = readNumber("1 Attack, 2 Defend, 3 Ability (free): ", 1, 3);
        if (c == 3) {
            if (u->skillUsed) { printf("Ability already used.\n"); continue; }
            abilityGI(a, d, u);
            if (livingCount(d) == 0) return;
            continue;                         /* abilities don't end the round */
        }
        if (c == 2) {
            if (coinMatch()) { a->defending = 1; printf("Defending! Incoming damage halved.\n"); }
            else printf("Defense failed.\n");
            return;
        }
        attackGI(a, d, u);
        return;
    }
}

/* ---------- GFL / LC turn ---------- */

void turn(Player *a, Player *d)
{
    a->defending = 0;   /* defend lasts until the owner's next turn */
    while (1) {
        int c = readNumber("\n1 Attack, 2 Defend, 3 Ability (free): ", 1, 3);
        if (c == 3) {
            useAbility(a, d);
            if (livingCount(d) == 0) return;
            continue;           /* abilities don't end the round */
        }
        if (c == 2) {
            if (coinMatch()) { a->defending = 1; printf("Defending! Incoming damage halved.\n"); }
            else printf("Defense failed.\n");
            return;
        }
        if (a->faction == GFL) attackGFL(a, d);
        else attackLC(a, d);
        return;
    }
}

/* ---------- main ---------- */

int main(void)
{
    srand((unsigned)time(NULL));
    printf("====================================\n");
    printf("      FRONTLINE COMPANY IMPACT\n");
    printf("====================================\n");

    Player players[2] = {0};
    strcpy(players[0].name, "Player 1");
    strcpy(players[1].name, "Player 2");

    for (int p = 0; p < 2; p++) {
        printf("\n%s, choose your faction:\n", players[p].name);
        printf("1. Girls' Frontline\n2. Genshin Impact\n3. Limbus Company\n");
        players[p].faction = readNumber("Faction: ", 1, 3);
    }

    for (int p = 0; p < 2; p++) {
        printf("\n%s (%s), build your team.\n", players[p].name, factionName[players[p].faction]);
        setupTeam(&players[p]);
        showTeam(&players[p]);
    }

    int cur = rand() % 2;
    printf("\nCoin flip: %s goes first!\n", players[cur].name);

    for (int round = 1; livingCount(&players[0]) && livingCount(&players[1]); round++) {
        Player *a = &players[cur], *d = &players[1 - cur];
        printf("\n========== ROUND %d: %s ==========\n", round, a->name);
        if (a->faction != GI) showTeam(a);   /* GI shows its team in chooseUnit */
        showTeam(d);
        if (a->faction == GI) turnGI(a, d);
        else turn(a, d);
        cur = 1 - cur;
    }

    printf("\n========== MATCH OVER ==========\n");
    printf("%s WINS!\n", livingCount(&players[0]) ? players[0].name : players[1].name);
    return 0;
}