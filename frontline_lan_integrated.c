/* Integrated POSIX/Linux LAN edition.
 * Host owns the complete original game state and combat logic.
 * Client receives printed game output and supplies requested choices.
 * No threads: every prompt/response is synchronous.
 */
#define _POSIX_C_SOURCE 200809L
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/socket.h>
#include <unistd.h>

static int create_server(unsigned short port) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) return -1;
    int reuse = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof reuse);
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(fd, (struct sockaddr *)&addr, sizeof addr) < 0 || listen(fd, 1) < 0) {
        close(fd); return -1;
    }
    printf("Host listening on port %u...\n", port);
    int peer = accept(fd, NULL, NULL);
    close(fd);
    return peer;
}

static int join_server(const char *ip, unsigned short port) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) return -1;
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip, &addr.sin_addr) != 1 ||
        connect(fd, (struct sockaddr *)&addr, sizeof addr) < 0) {
        close(fd); return -1;
    }
    return fd;
}



/* Framed TCP messages: 1-byte kind, 4-byte network-order payload length, payload. */
static int peer_fd = -1;
static int active_player = 0;
static void connection_lost(void) {
    fputs("Connection closed or local input ended.\n", stderr);
    if (peer_fd >= 0) close(peer_fd);
    exit(1);
}
static int send_bytes(int fd, const void *data, size_t size) {
    const unsigned char *p = data;
    while (size) {
        ssize_t n = send(fd, p, size, 0);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) return -1;
        p += n; size -= (size_t)n;
    }
    return 0;
}
static int recv_bytes(int fd, void *data, size_t size) {
    unsigned char *p = data;
    while (size) {
        ssize_t n = recv(fd, p, size, 0);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) return -1;
        p += n; size -= (size_t)n;
    }
    return 0;
}
static int send_packet(int fd, char kind, const char *payload) {
    size_t len = strlen(payload);
    if (len > 4095) return -1;
    uint32_t network_len = htonl((uint32_t)len);
    return send_bytes(fd, &kind, 1) || send_bytes(fd, &network_len, 4) ||
           send_bytes(fd, payload, len) ? -1 : 0;
}
static int recv_packet(int fd, char *payload, size_t capacity) {
    char kind;
    uint32_t network_len;
    if (recv_bytes(fd, &kind, 1) < 0 || recv_bytes(fd, &network_len, 4) < 0) return -1;
    uint32_t len = ntohl(network_len);
    if (len >= capacity || len > 4095) return -1;
    if (recv_bytes(fd, payload, len) < 0) return -1;
    payload[len] = 0;
    return kind;
}
static int mirrored_printf(const char *format, ...) {
    char buffer[4096];
    va_list ap;
    va_start(ap, format);
    int n = vsnprintf(buffer, sizeof buffer, format, ap);
    va_end(ap);
    if (n < 0 || (size_t)n >= sizeof buffer) connection_lost();
    fputs(buffer, stdout);
    fflush(stdout);
    if (send_packet(peer_fd, 'T', buffer) < 0) connection_lost();
    return n;
}
static int local_choice(const char *prompt, int min, int max) {
    char line[100], extra;
    int value;
    while (1) {
        fputs(prompt, stdout);
        fflush(stdout);
        if (!fgets(line, sizeof line, stdin)) return -1;
        if (sscanf(line, "%d %c", &value, &extra) == 1 && value >= min && value <= max) return value;
        fprintf(stdout, "Invalid. Enter %d to %d.\n", min, max);
    }
}

#define printf mirrored_printf

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
    if (active_player == 1) {
        char request[1024], reply[128], extra;
        int value;
        snprintf(request, sizeof request, "%d %d %s", min, max, prompt);
        for (;;) {
            if (send_packet(peer_fd, 'Q', request) < 0 || recv_packet(peer_fd, reply, sizeof reply) != 'N') connection_lost();
            if (sscanf(reply, "%d %c", &value, &extra) == 1 && value >= min && value <= max) return value;
            if (send_packet(peer_fd, 'T', "Invalid choice.\n") < 0) connection_lost();
        }
    }
    char line[100], extra;
    int value;
    while (1) {
        fputs(prompt, stdout);
        fflush(stdout);
        if (!fgets(line, sizeof line, stdin)) connection_lost();
        if (sscanf(line, "%d %c", &value, &extra) == 1 && value >= min && value <= max) return value;
        fprintf(stdout, "Invalid. Enter %d to %d.\n", min, max);
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

static int host_game(int fd)
{
    peer_fd = fd;
    srand((unsigned int)(getpid() ^ (unsigned int)time(NULL)));
    printf("====================================\n");
    printf("      FRONTLINE COMPANY IMPACT\n");
    printf("====================================\n");
    Player players[2] = {0};
    strcpy(players[0].name, "Player 1");
    strcpy(players[1].name, "Player 2");
    for (int i = 0; i < 2; i++) {
        active_player = i;
        printf("\n%s, choose your faction:\n", players[i].name);
        printf("1. Girls' Frontline\n2. Genshin Impact\n3. Limbus Company\n");
        players[i].faction = readNumber("Faction: ", 1, 3);
    }
    for (int i = 0; i < 2; i++) {
        active_player = i;
        printf("\n%s (%s), build your team.\n", players[i].name, factionName[players[i].faction]);
        setupTeam(&players[i]);
        showTeam(&players[i]);
    }
    int cur = rand() % 2;
    printf("\nCoin flip: %s goes first!\n", players[cur].name);
    for (int round = 1; livingCount(&players[0]) && livingCount(&players[1]); round++) {
        active_player = cur;
        Player *a = &players[cur], *d = &players[1 - cur];
        printf("\n========== ROUND %d: %s ==========\n", round, a->name);
        if (a->faction != GI) showTeam(a);
        showTeam(d);
        if (a->faction == GI) turnGI(a, d);
        else turn(a, d);
        cur = 1 - cur;
    }
    printf("\n========== MATCH OVER ==========\n");
    printf("%s WINS!\n", livingCount(&players[0]) ? players[0].name : players[1].name);
    if (send_packet(fd, 'E', "Match complete") < 0) connection_lost();
    return 0;
}

static int client_game(int fd)
{
    char message[4096];
    for (;;) {
        int kind = recv_packet(fd, message, sizeof message);
        if (kind == 'T') { fputs(message, stdout); fflush(stdout); }
        else if (kind == 'Q') {
            int low, high, offset = 0;
            if (sscanf(message, "%d %d %n", &low, &high, &offset) != 2 || offset == 0 || low > high) return -1;
            int choice = local_choice(message + offset, low, high);
            if (choice < 0) return -1;
            char answer[64];
            snprintf(answer, sizeof answer, "%d", choice);
            if (send_packet(fd, 'N', answer) < 0) return -1;
        } else if (kind == 'E') return 0;
        else return -1;
    }
}

int main(int argc, char **argv)
{
    if (argc != 3 && argc != 4) { // Checks if  the user correctly enters 3 or 4 arguments
        fprintf(stderr, "Usage: %s host PORT | %s join HOST_IP PORT\n", argv[0], argv[0]);
        return 1;
    }
    char *end; // An integral variable for the succeeding strtol() function
    const char *port_text = argc == 3 ? argv[2] : argv[3]; // First storage for the inputted port number, inside a string literal.
    long port = strtol(port_text, &end, 10); // This is where the inputted port number is then stored in the long variable `port`

    if (!*port_text || *end || port < 1 || port > 65535) { // Checks if inputted port is valid. Refer to Canvas Documentation for details.
        fputs("Invalid port.\n", stderr); return 1;
    }

    int is_host = argc == 3 && strcmp(argv[1], "host") == 0; // Assigns `1` if command executed is valid and entered by Host. Otherwise, `0`.
    int is_client = argc == 4 && strcmp(argv[1], "join") == 0; // Assigns `1` if command executed is valid and entered by Client. Otherwise, `0`.

    if (!is_host && !is_client) { // Checks if command is invalid
        fputs("Expected host or join mode.\n", stderr); return 1;
    }

    signal(SIGPIPE, SIG_IGN); // Ignore SIGPIPE signal by Operating System
    int fd = is_host ? create_server((unsigned short)port) : join_server(argv[2], (unsigned short)port); // Identifier for socket within the process. Depends on whether user is Host or Client.

    if (fd < 0) { perror("Connection setup failed"); return 1; } // Return error if fd is invalid

    puts("Connected!");
    int result = is_host ? host_game(fd) : client_game(fd);

    if (result < 0) fputs("Game stopped: disconnect, invalid message, or input ended.\n", stderr);

    close(fd);
    return result < 0 ? 1 : 0;
}
