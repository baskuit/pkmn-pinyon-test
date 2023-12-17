from fractions import Fraction

alpha = Fraction(714782643423,1451698946048)
beta = Fraction(414261/218103808)
gamma = Fraction(235407649/725849473024)

miss = Fraction(729, 65536)

total_prob = 2 * alpha + 2 * beta + gamma + miss

x = alpha + beta + gamma * Fraction(1, 2)
y = x / (Fraction(1) - miss)
print(x)
print(y)
print(Fraction(1) - miss)
print(total_prob)
print(total_prob.denominator - total_prob.numerator)
