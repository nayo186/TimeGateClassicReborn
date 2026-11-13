///////////////////////////////////////////////////////////////////////////////
// Time Gate: Classic Reborn
// Based on FITD by yaz0r and AloneInTheDarkReHaunted, released under GPL
//
// Time Gate's way finder for the objects that chase another (see
// timeGatePath.h). Written from TIMEGATE.EXE: the addresses are those of its
// routines and variables
///////////////////////////////////////////////////////////////////////////////

#include "common.h"
#include "timeGatePath.h"
#include "consoleLog.h"

#include <cstdlib>
#include <vector>

namespace
{
    struct Box { int x1, z1, x2, z2; };
    struct Node { s16 x, z; u8 corner; };
    struct Leg { u16 from, to; };

    const int kMargin = 3;              // 0x61832 and 0x61834
    const u32 kLongestLeg = 3000;       // 0x61838, in the length's units (4 per grid square)
    const int kMaxNodes = 300;          // the size of its tables
    const int kMaxLegs = 16;            // 0x8AB20
    const u32 kNoLength = 0xFFFFFF;
    const int kZoneLinks = 16;

    std::vector<Box> s_obstacles;       // 0x8AB50: the floor's boxes, a little bigger (2F720)
    std::vector<Box> s_zones;           // 0x8AB58
    s8 s_links[kZoneLinks][kZoneLinks]; // 0x7E92C: the zone that joins two others, plus 1
    std::vector<Node> s_forbidden;      // 0x8A538

    std::vector<Node> s_nodes;          // 0x8AB5C: the corners, then the start and the goal
    int s_startZone = -1, s_goalZone = -1;        // 0x8B2EA, 0x8B2E4
    int s_nodesStartZone = -2, s_nodesGoalZone = -2;  // 0x8B2E6, 0x8B2F0
    bool s_newCorners = true;           // 0x6183C
    int s_tableNodes = 0;               // 0x61982: nodes in the tables below
    std::vector<u8> s_hidden;           // 0x8AB48: whether a leg crosses an obstacle
    std::vector<u32> s_length;          // 0x8AB1C: a leg's length

    // The search (40664)
    int s_goal = 0;                     // 0x8AB2C
    u32 s_weight = 32;                  // 0x8A534
    u32 s_best = 0;                     // 0x8AB28
    int s_bestLegs = 0;                 // 0x8B2EC
    Leg s_legs[kMaxLegs];               // 0x7EA6C
    Leg s_bestWay[kMaxLegs];            // 0x7EA2C

    bool inside(const Box& box, int x, int z)
    {
        return x >= box.x1 && x <= box.x2 && z >= box.z1 && z <= box.z2;
    }

    // 30300: the first zone that has the point, -1 if none
    int zoneOf(int x, int z)
    {
        for (int i = 0; i < (int)s_zones.size(); i++)
            if (inside(s_zones[i], x, z))
                return i;
        return -1;
    }

    bool insideZone(int zone, int x, int z)
    {
        return zone >= 0 && zone < (int)s_zones.size() && inside(s_zones[zone], x, z);
    }

    int zoneLink(int from, int to)
    {
        if (from < 0 || from >= kZoneLinks || to < 0 || to >= kZoneLinks)
            return 0;
        return s_links[from][to];
    }

    // 2F4F0: whether a corner may not be a node: inside an obstacle, or the
    // point next to it towards its box is, or it is a forbidden point, or it
    // is in none of the zones of the way
    bool cornerRefused(int x, int z, int corner)
    {
        const int nearX = (corner == 1 || corner == 4) ? x + kMargin + 1 : x - kMargin - 1;
        const int nearZ = (corner == 1 || corner == 2) ? z + kMargin + 1 : z - kMargin - 1;
        for (const Box& box : s_obstacles)
            if (inside(box, x, z) || inside(box, nearX, z) || inside(box, x, nearZ))
                return true;
        for (const Node& point : s_forbidden)
            if (point.x == x && point.z == z)
                return true;
        if (insideZone(s_startZone, x, z) || insideZone(s_goalZone, x, z))
            return false;
        const int link = zoneLink(s_startZone, s_goalZone);
        return !(link > 0 && insideZone(link - 1, x, z));
    }

    // 2FC60 and 2FD90: whether the leg from (x1, z1) to (x2, z2), x1 <= x2,
    // crosses a side of the box
    bool crosses(const Box& box, int x1, int z1, int x2, int z2)
    {
        const int dx = x2 - x1;
        const int dz = z2 - z1;
        const int zLow = (std::min)(z1, z2);
        const int zHigh = (std::max)(z1, z2);
        for (int side = 0; side < 2; side++)
        {
            const int x = side ? box.x2 : box.x1;
            if (x < x1 || x > x2)
                continue;
            int z = z1;
            if (dx && dz)
                z += (x - x1) * dz / dx;
            if (z >= box.z1 && z <= box.z2)
                return true;
        }
        for (int side = 0; side < 2; side++)
        {
            const int z = side ? box.z2 : box.z1;
            if (z < zLow || z > zHigh)
                continue;
            int x = x1;
            if (dx && dz)
                x += (z - z1) * dx / dz;
            if (x >= box.x1 && x <= box.x2)
                return true;
        }
        return false;
    }

    // 2FEC0: whether the leg crosses an obstacle
    bool legHidden(const Node& a, const Node& b)
    {
        const int zLow = (std::min)(a.z, b.z);
        const int zHigh = (std::max)(a.z, b.z);
        for (const Box& box : s_obstacles)
            if (box.x1 <= b.x && box.x2 >= a.x && box.z1 <= zHigh && box.z2 >= zLow && crosses(box, a.x, a.z, b.x, b.z))
                return true;
        return false;
    }

    // 4062C: the square root of n, times 4 (as the original works it out)
    u32 lengthOf(u32 n)
    {
        if (!n)
            return 0;
        u32 root = n * 8;
        const u32 sixteen = n * 16;
        for (int i = 0; i < 32; i++)
        {
            const u32 last = root;
            root = (u32)((sixteen / root + root) >> 1);
            if (root == last)
                break;
        }
        return root;
    }

    // One leg in the tables (2FFA0 and 30170): the node more to the left
    // first, `first` on a tie
    void measureLeg(int first, int other)
    {
        const Node& a = s_nodes[first];
        const Node& b = s_nodes[other];
        const bool hidden = (a.x <= b.x) ? legHidden(a, b) : legHidden(b, a);
        const int one = first * kMaxNodes + other;
        const int two = other * kMaxNodes + first;
        s_hidden[one] = s_hidden[two] = hidden ? 1 : 0;
        if (hidden)
            return;
        const int dx = a.x - b.x;
        const int dz = a.z - b.z;
        const u32 length = lengthOf((u32)(dx * dx + dz * dz));
        if (length > kLongestLeg)
            s_hidden[one] = s_hidden[two] = 1;
        else
            s_length[one] = s_length[two] = length;
    }

    void noLegToItself(int node)
    {
        s_hidden[node * kMaxNodes + node] = 1;
        s_length[node * kMaxNodes + node] = kNoLength;
    }

    // 2F7A0: the nodes, the corners again only when the zones of the way
    // change; then the start and the goal
    void makeNodes(int startX, int startZ, int goalX, int goalZ)
    {
        if (s_startZone != s_nodesStartZone || s_goalZone != s_nodesGoalZone)
        {
            s_nodesStartZone = s_startZone;
            s_nodesGoalZone = s_goalZone;
            s_newCorners = true;
            s_tableNodes = 0;
        }
        if (s_newCorners)
        {
            s_newCorners = false;
            s_nodes.clear();
            for (const Box& box : s_obstacles)
            {
                const int corners[4][2] = {
                    { box.x1 - kMargin, box.z1 - kMargin }, { box.x2 + kMargin, box.z1 - kMargin },
                    { box.x2 + kMargin, box.z2 + kMargin }, { box.x1 - kMargin, box.z2 + kMargin } };
                for (int corner = 0; corner < 4; corner++)
                {
                    const int x = corners[corner][0];
                    const int z = corners[corner][1];
                    if (cornerRefused(x, z, corner + 1))
                        continue;
                    bool again = false;
                    for (const Node& node : s_nodes)
                        again = again || (node.x == x && node.z == z);
                    if (!again && (int)s_nodes.size() < kMaxNodes - 2)
                        s_nodes.push_back({ (s16)x, (s16)z, (u8)(corner + 1) });
                }
            }
        }
        else
            s_nodes.resize(s_nodes.size() - 2);
        s_nodes.push_back({ (s16)startX, (s16)startZ, 0 });
        s_nodes.push_back({ (s16)goalX, (s16)goalZ, 0 });
    }

    // 40910: the nodes of the list that one sees (no obstacle between) and
    // those it does not
    void split(int node, const std::vector<u16>& list, std::vector<u16>& seen, std::vector<u16>& unseen)
    {
        for (u16 other : list)
        {
            if (other == node)
                continue;
            if (s_hidden[node * kMaxNodes + other])
                unseen.push_back(other);
            else
                seen.push_back(other);
        }
    }

    // 40664 and 407AB: from a node, the goal at once if it is seen; else
    // each node seen, then on with the ones it did not see. A way longer than
    // the best (weighed by the zones) is left. True when the goal is seen
    // from this node, its length added to `length`
    bool search(int node, int depth, u32& length, const std::vector<u16>& list)
    {
        std::vector<u16> seen, unseen;
        split(node, list, seen, unseen);
        if (seen.empty())
            return false;
        for (u16 other : seen)
            if (other == s_goal)
            {
                length += s_length[node * kMaxNodes + s_goal];
                s_legs[depth] = { (u16)node, (u16)s_goal };
                if (depth == 0)
                {
                    s_bestWay[0] = s_legs[0];
                    s_bestLegs = 1;
                    s_best = length;
                }
                return true;
            }
        if (depth + 1 == kMaxLegs)
            return false;
        for (u16 next : seen)
        {
            u32 nextLength = s_length[node * kMaxNodes + next] + length;
            if ((u32)(nextLength * s_weight) >= (u32)(s_best << 5))
                continue;
            s_legs[depth] = { (u16)node, next };
            if (!search(next, depth + 1, nextLength, unseen) || nextLength >= s_best)
                continue;
            s_best = nextLength;
            s_bestLegs = depth + 2;
            for (int i = 0; i < s_bestLegs; i++)
                s_bestWay[i] = s_legs[i];
        }
        return false;
    }
}

void timeGatePathLoadFloor(int floor)
{
    s_obstacles.clear();
    s_zones.clear();
    s_forbidden.clear();
    memset(s_links, 0, sizeof(s_links));
    s_nodes.clear();
    s_nodesStartZone = s_nodesGoalZone = -2;
    s_newCorners = true;
    s_tableNodes = 0;

    const int size = getPakSize("PURSUIT", floor);
    char* data = size > 12 ? loadPak("PURSUIT", floor) : nullptr;
    if (!data)
    {
        printf(MAIN_WARN "Time Gate: no ways for the chasers of floor %d (PURSUIT.PAK)" CON_RESET "\n", floor);
        return;
    }
    u8* p = (u8*)data;
    const int boxes = (int)READ_LE_U32(p);
    const int forbidden = READ_LE_U16(p + 4);
    const int zones = READ_LE_U16(p + 6);
    const int needed = 12 + boxes * 10 + zones * 10 + kZoneLinks * kZoneLinks + forbidden * 5;
    if (boxes < 0 || needed > size)
    {
        printf(MAIN_WARN "Time Gate: the ways of floor %d are shorter than they say" CON_RESET "\n", floor);
        free(data);
        return;
    }
    p += 12;
    for (int i = 0; i < boxes; i++, p += 10)
    {
        // 2F720: a little bigger; 2F7A0 puts the lower z first
        Box box = { READ_LE_S16(p) - kMargin + 1, READ_LE_S16(p + 2) - kMargin + 1,
                    READ_LE_S16(p + 4) + kMargin - 1, READ_LE_S16(p + 6) + kMargin - 1 };
        if (box.z1 > box.z2)
            std::swap(box.z1, box.z2);
        s_obstacles.push_back(box);
    }
    for (int i = 0; i < zones; i++, p += 10)
        s_zones.push_back({ READ_LE_S16(p), READ_LE_S16(p + 2), READ_LE_S16(p + 4), READ_LE_S16(p + 6) });
    memcpy(s_links, p, sizeof(s_links));
    p += sizeof(s_links);
    for (int i = 0; i < forbidden; i++, p += 5)
        s_forbidden.push_back({ READ_LE_S16(p), READ_LE_S16(p + 2), p[4] });
    free(data);

    s_hidden.assign(kMaxNodes * kMaxNodes, 1);
    s_length.assign(kMaxNodes * kMaxNodes, kNoLength);
}

int timeGateFindPath(int startX, int startZ, int goalX, int goalZ, int& nextX, int& nextZ)
{
    s_bestLegs = 0;
    if (s_obstacles.empty() && s_zones.empty())
        return 0;

    // 30350
    s_startZone = zoneOf(startX, startZ);
    s_goalZone = zoneOf(goalX, goalZ);
    s_weight = 32 + (s_startZone != s_goalZone ? 1 : 0) + (zoneLink(s_startZone, s_goalZone) > 0 ? 1 : 0);
    makeNodes(startX, startZ, goalX, goalZ);
    const int count = (int)s_nodes.size();
    const int start = count - 2;
    const int goal = count - 1;
    if (count != s_tableNodes)
    {
        // 2FFA0: every leg
        s_tableNodes = count;
        for (int i = 0; i < count; i++)
        {
            noLegToItself(i);
            for (int j = i + 1; j < count; j++)
                measureLeg(i, j);
        }
    }
    else
    {
        // 30170: only those of the start and the goal
        for (int node : { start, goal })
        {
            noLegToItself(node);
            for (int other = 0; other < count; other++)
                if (other != node)
                    measureLeg(node, other);
        }
    }

    // From the end that sees fewer nodes (0x950FC)
    std::vector<u16> all(count);
    for (int i = 0; i < count; i++)
        all[i] = (u16)i;
    std::vector<u16> seen, unseen;
    split(start, all, seen, unseen);
    const size_t seenFromStart = seen.size();
    seen.clear();
    unseen.clear();
    split(goal, all, seen, unseen);
    const bool fromGoal = seenFromStart > seen.size();

    // 304B0
    s_best = 0x3FFFFFF;
    s_goal = fromGoal ? start : goal;
    u32 length = 0;
    search(fromGoal ? goal : start, 0, length, all);
    if (s_bestLegs == 0)
        return 0;
    const Leg first = fromGoal ? Leg{ s_bestWay[s_bestLegs - 1].to, s_bestWay[s_bestLegs - 1].from } : s_bestWay[0];
    nextX = s_nodes[first.to].x;
    nextZ = s_nodes[first.to].z;
    return s_bestLegs;
}
