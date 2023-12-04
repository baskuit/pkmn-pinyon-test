from fractions import Fraction

# TODO problem is that blizzard 'procs' when bslam misses?

value_table = {}
move_table = {}

max_hp = 1
attacks_by_charging = [["bslam", "blizzard", "hyperbeam"], ["recharge"]]
crit = {0: Fraction(15, 16), 1: Fraction(1, 16)}

accs = {
    "bslam": {0: Fraction(1, 256), 1: Fraction(255, 256)},
    "blizzard": {0: Fraction(1, 256), 1: Fraction(255, 256)},
    "hyperbeam": {0: Fraction(1, 256), 1: Fraction(255, 256)},
}

procs = {
    "bslam": {0: Fraction(1, 1), 1: Fraction(0, 1)},
    "blizzard": {0: Fraction(9, 10), 1: Fraction(1, 10)},
    "hyperbeam": {0: Fraction(1, 1), 1: Fraction(0, 1)},
}

rolls = [
    {
        "bslam" : {95: 2, 96: 2, 97: 3, 98: 2, 99: 2, 100: 2, 101: 3, 102: 2, 103: 2, 104: 3, 105: 2, 106: 2, 107: 2, 108: 3, 109: 2, 110: 2, 111: 2, 112: 1},
        "blizzard" : {86: 1, 87: 2, 88: 3, 89: 2, 90: 3, 91: 2, 92: 3, 93: 2, 94: 3, 95: 2, 96: 3, 97: 2, 98: 3, 99: 2, 100: 3, 101: 2, 102: 1},
        "hyperbeam" : {166: 1, 167: 1, 168: 1, 169: 2, 170: 1, 171: 1, 172: 2, 173: 1, 174: 1, 175: 1, 176: 2, 177: 1, 178: 1, 179: 2, 180: 1, 181: 1, 182: 2, 183: 1, 184: 1, 185: 1, 186: 2, 187: 1, 188: 1, 189: 2, 190: 1, 191: 1, 192: 2, 193: 1, 194: 1, 195: 1, 196: 1},
    },
    {
        "bslam" : {184: 1, 185: 1, 186: 1, 187: 1, 188: 2, 189: 1, 190: 1, 191: 1, 192: 1, 193: 1, 194: 2, 195: 1, 196: 1, 197: 1, 198: 1, 199: 2, 200: 1, 201: 1, 202: 1, 203: 1, 204: 1, 205: 2, 206: 1, 207: 1, 208: 1, 209: 1, 210: 1, 211: 2, 212: 1, 213: 1, 214: 1, 215: 1, 216: 1, 217: 1},
        "blizzard" : {168: 1, 169: 1, 170: 2, 171: 1, 172: 1, 173: 2, 174: 1, 175: 1, 176: 1, 177: 2, 178: 1, 179: 1, 180: 2, 181: 1, 182: 1, 183: 1, 184: 2, 185: 1, 186: 1, 187: 2, 188: 1, 189: 1, 190: 1, 191: 2, 192: 1, 193: 1, 194: 2, 195: 1, 196: 1, 197: 1, 198: 1},
        "hyperbeam" : {324: 1, 325: 1, 327: 1, 328: 1, 330: 1, 331: 1, 333: 1, 334: 1, 336: 1, 337: 1, 339: 1, 340: 1, 342: 1, 343: 1, 345: 1, 346: 1, 348: 1, 349: 1, 351: 1, 352: 1, 354: 1, 355: 1, 357: 1, 358: 1, 360: 1, 361: 1, 363: 1, 364: 1, 366: 1, 367: 1, 369: 1, 370: 1, 372: 1, 373: 1, 375: 1, 376: 1, 378: 1, 379: 1, 381: 1},
    },
    {
        "bslam" : {0: 39},
        "blizzard" : {0: 39},
        "hyperbeam" : {0: 39},
    },
]

def solve(p1_hp, p2_hp, p1_recharge, p2_recharge):
    # flip
    if p2_hp > p1_hp:
        return Fraction(1, 1) - value_table[(p2_hp, p1_hp, p2_recharge, p1_recharge)]
    # otherwise just lookup
    return value_table[(p1_hp, p2_hp, p1_recharge, p2_recharge)]


def solve_q(p1_hp, p2_hp, p1_move, p2_move):
    p1_accs = accs[p1_move]
    p2_accs = accs[p2_move]
    p1_procs = procs[p1_move]
    p2_procs = procs[p2_move]

    expected_value = Fraction(0, 1)
    same_prob = Fraction(0, 1)

    count = 0
    total_prob = Fraction(0, 1)
    p1_win_prob = Fraction(0, 1)
    p2_win_prob = Fraction(0, 1)

    for speed_tie in range(2):

        for p1_acc in p1_accs:
            for p1_proc in p1_procs:
                for p1_crit in crit:
                    p1_roll_idx = 2 if (p1_proc or not p1_acc) else p1_crit
                    p1_rolls = rolls[p1_roll_idx][p1_move]
                    for p1_roll in p1_rolls:
                    
                        for p2_acc in p2_accs:
                            for p2_proc in p2_procs:
                                for p2_crit in crit:
                                    p2_roll_idx = 2 if (p2_proc or not p2_acc)  else p2_crit
                                    p2_rolls = rolls[p2_roll_idx][p2_move]
                                    for p2_roll in p2_rolls:

                                        prob = (
                                            crit[p1_crit]
                                            * p1_rolls[p1_roll]
                                            * p1_accs[p1_acc]
                                            * p1_procs[p1_proc]
                                            * crit[p2_crit]
                                            * p2_rolls[p2_roll]
                                            * p2_accs[p2_acc]
                                            * p2_procs[p2_proc]
                                            * Fraction(1, 2 * 39**2) #accounting for damage rolls and speed tie
                                        )
                                        total_prob += prob

                                        p1_win = (p1_roll >= p2_hp) or (p1_proc and p1_acc)
                                        p2_win = (p2_roll >= p1_hp) or (p2_proc and p2_acc)

                                        count += 1

                                        if p1_win:
                                            if p2_win:
                                                if speed_tie:
                                                    expected_value += prob  # p1 won
                                                    p1_win_prob += prob
                                                else:
                                                    p2_win_prob += prob
                                                    pass
                                            else:
                                                expected_value += prob
                                                p1_win_prob += prob
                                        elif p2_win:
                                            p2_win_prob += prob
                                            pass  # done, since we add 0 * prob
                                        else:
                                            # no win
                                            p1_recharge = (
                                                p1_move == "hyperbeam" and p1_acc
                                            )
                                            p2_recharge = (
                                                p2_move == "hyperbeam" and p2_acc
                                            )
                                            both_recharge = p1_recharge and p2_recharge

                                            if both_recharge or (not p1_acc and not p2_acc):
                                                same_prob += prob
                                            else:
                                                value = solve(
                                                    p1_hp - p2_acc * p2_roll,
                                                    p2_hp - p1_acc * p1_roll,
                                                    p1_recharge,
                                                    p2_recharge,
                                                )

                                                expected_value += value * prob
    
    expected_value = expected_value / (Fraction(1, 1) - same_prob)
    print(p1_hp, p2_hp, p1_move, p2_move, expected_value)
    print(expected_value)
    print("P1 WIN", p1_win_prob, float(p1_win_prob))
    print("P2 WIN", p2_win_prob, float(p2_win_prob))
    print("SAME", same_prob, float(same_prob))
    print("TOTAL", total_prob)

    return expected_value


if __name__ == "__main__":
    for p1_hp in range(1, max_hp + 1):
        for p2_hp in range(1, p1_hp + 1):
            print("MAIN HP LOOP:", p1_hp, p2_hp)
            # p2 always has <= p1 hp

            matrix = {}

            solve_q(p1_hp, p2_hp, "bslam", "blizzard")

            for p1_attack in attacks_by_charging[0]:
                for p2_attack in attacks_by_charging[0]:
                    value = solve_q(p1_hp, p2_hp, p1_attack, p2_attack)
                    matrix[(p1_attack, p2_attack)] = value

            # get optimal moves and value

            worst = []
            for p1_attack in attacks_by_charging[0]:
                worst_case = Fraction(1, 1)
                for p2_attack in attacks_by_charging[0]:
                    value = matrix[(p1_attack, p2_attack)]
                    if value < worst_case:
                        worst_case = value
                worst.append(worst_case)

            best = []
            for p2_attack in attacks_by_charging[0]:
                best_case = Fraction(0, 1)
                for p1_attack in attacks_by_charging[0]:
                    value = matrix[(p1_attack, p2_attack)]
                    if value > best_case:
                        best_case = value
                best.append(best_case)

            assert(max(worst) == min(best))
            value = max(worst)

            p1_best_moves = []
            for i, p1_attack in enumerate(attacks_by_charging[0]):
                if worst[i] == value:
                    p1_best_moves.append(p1_attack)
            p2_best_moves = []
            for i, p2_attack in enumerate(attacks_by_charging[0]):
                if best[i] == value:
                    p2_best_moves.append(p2_attack)

            value_table[(p1_hp, p2_hp, 0, 0)] = value
            move_table[(p1_hp, p2_hp, 0, 0)] = (p1_best_moves, p2_best_moves)

            print(p1_best_moves)
            print(p2_best_moves)

            for p1_attack in attacks_by_charging[0]:
                value = solve_q(p1_hp, p2_hp, p1_attack, "recharge")

            for p2_attack in attacks_by_charging[0]:
                value = solve_q(p1_hp, p2_hp, "recharge", p2_attack)
